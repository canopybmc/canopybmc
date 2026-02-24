// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH

#include "../src/rom_service.hpp"
#include "../src/smbios_writer.hpp"
#include "mock_channel.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <fstream>

using namespace chif;

class RomServiceTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        testDir_ = std::filesystem::temp_directory_path() / "chif_test_rom";
        std::filesystem::create_directories(testDir_);
        writer_ = std::make_unique<SmbiosWriter>(testDir_);
        // nullptr MdrBridge — no D-Bus in tests
        service_ = std::make_unique<RomService>(*writer_, nullptr);
    }

    void TearDown() override
    {
        std::filesystem::remove_all(testDir_);
    }

    // Build a raw request packet
    std::vector<uint8_t> makeRequest(uint16_t command,
                                     std::span<const uint8_t> payload = {},
                                     uint16_t seq = 1)
    {
        std::vector<uint8_t> pkt(sizeof(ChifPktHeader) + payload.size());
        ChifPktHeader hdr{};
        hdr.pktSize = static_cast<uint16_t>(pkt.size());
        hdr.sequence = seq;
        hdr.command = command;
        hdr.serviceId = romServiceId;
        hdr.version = 0x01;
        std::memcpy(pkt.data(), &hdr, sizeof(hdr));
        if (!payload.empty())
        {
            std::memcpy(pkt.data() + sizeof(hdr), payload.data(),
                        payload.size());
        }
        return pkt;
    }

    // Wrap raw SMBIOS records in HPE ROM blob format.
    // Format: uint32_t count, then count x (uint16_t size + raw record).
    static std::vector<uint8_t> wrapInBlob(
        const std::vector<std::vector<uint8_t>>& records)
    {
        std::vector<uint8_t> blob;
        uint32_t count = static_cast<uint32_t>(records.size());
        blob.insert(blob.end(), reinterpret_cast<const uint8_t*>(&count),
                    reinterpret_cast<const uint8_t*>(&count) + sizeof(count));
        for (const auto& rec : records)
        {
            uint16_t size = static_cast<uint16_t>(rec.size());
            blob.insert(
                blob.end(), reinterpret_cast<const uint8_t*>(&size),
                reinterpret_cast<const uint8_t*>(&size) + sizeof(size));
            blob.insert(blob.end(), rec.begin(), rec.end());
        }
        return blob;
    }

    std::filesystem::path testDir_;
    std::unique_ptr<SmbiosWriter> writer_;
    std::unique_ptr<RomService> service_;
    std::array<uint8_t, maxPacketSize> respBuf_{};
};

TEST_F(RomServiceTest, ServiceId)
{
    EXPECT_EQ(service_->serviceId(), romServiceId);
}

TEST_F(RomServiceTest, BeginCommand)
{
    auto req = makeRequest(romCmdBegin);
    int n = service_->handle(req, respBuf_);

    // ROM responses are header-only (8 bytes) with no response bit
    ASSERT_EQ(static_cast<size_t>(n), sizeof(ChifPktHeader));

    auto rspHdr = parseHeader(
        std::span<const uint8_t>(respBuf_.data(), static_cast<size_t>(n)));
    EXPECT_EQ(rspHdr.command, romCmdBegin); // no response bit for ROM
    EXPECT_EQ(rspHdr.sequence, 1);
}

TEST_F(RomServiceTest, RecordCommand)
{
    // Begin first
    auto beginReq = makeRequest(romCmdBegin);
    service_->handle(beginReq, respBuf_);

    // Send a record wrapped in HPE blob format
    std::vector<uint8_t> rawRecord = {
        0x01, 0x1B, 0x01, 0x00, // Type 1, length 27, handle 1
    };
    rawRecord.resize(0x1B, 0x00);
    rawRecord.push_back(0x00);
    rawRecord.push_back(0x00); // double-null terminator

    auto blob = wrapInBlob({rawRecord});
    auto req = makeRequest(romCmdRecord, blob, 2);
    int n = service_->handle(req, respBuf_);

    // ROM responses are header-only (8 bytes)
    ASSERT_EQ(static_cast<size_t>(n), sizeof(ChifPktHeader));

    auto rspHdr = parseHeader(
        std::span<const uint8_t>(respBuf_.data(), static_cast<size_t>(n)));
    EXPECT_EQ(rspHdr.command, romCmdRecord); // no response bit for ROM
    EXPECT_EQ(rspHdr.sequence, 2);

    // Verify data was accumulated (raw record size, not blob size)
    EXPECT_EQ(writer_->dataSize(), rawRecord.size());
}

TEST_F(RomServiceTest, EmptyRecordHandledGracefully)
{
    auto beginReq = makeRequest(romCmdBegin);
    service_->handle(beginReq, respBuf_);

    // Send header-only (no payload)
    auto req = makeRequest(romCmdRecord);
    int n = service_->handle(req, respBuf_);

    // Still get a header-only response
    ASSERT_EQ(static_cast<size_t>(n), sizeof(ChifPktHeader));

    // No data should have been accumulated
    EXPECT_EQ(writer_->dataSize(), 0u);
}

TEST_F(RomServiceTest, EndCommandFinalizesFile)
{
    auto beginReq = makeRequest(romCmdBegin);
    service_->handle(beginReq, respBuf_);

    // Add a record in blob format
    std::vector<uint8_t> rec = {0x01, 0x04, 0x01, 0x00, 0x00, 0x00};
    auto blob = wrapInBlob({rec});
    auto recReq = makeRequest(romCmdRecord, blob, 2);
    service_->handle(recReq, respBuf_);

    // End
    auto endReq = makeRequest(romCmdEnd, {}, 3);
    int n = service_->handle(endReq, respBuf_);

    ASSERT_EQ(static_cast<size_t>(n), sizeof(ChifPktHeader));
    auto rspHdr = parseHeader(
        std::span<const uint8_t>(respBuf_.data(), static_cast<size_t>(n)));
    EXPECT_EQ(rspHdr.command, romCmdEnd); // no response bit for ROM
    EXPECT_EQ(rspHdr.sequence, 3);

    // Verify the smbios2 file was created
    EXPECT_TRUE(std::filesystem::exists(writer_->outputPath()));
}

TEST_F(RomServiceTest, FullSequence)
{
    // Simulate the full BIOS sequence: begin -> record -> record -> end

    // Begin
    auto beginReq = makeRequest(romCmdBegin, {}, 1);
    int n = service_->handle(beginReq, respBuf_);
    ASSERT_EQ(static_cast<size_t>(n), sizeof(ChifPktHeader));

    // Record 1: Type 1 (System Information)
    std::vector<uint8_t> rec1 = {0x01, 0x1B, 0x01, 0x00};
    rec1.resize(0x1B, 0x00);
    rec1.push_back('H');
    rec1.push_back('P');
    rec1.push_back('E');
    rec1.push_back(0x00);
    rec1.push_back(0x00);

    auto blob1 = wrapInBlob({rec1});
    auto recReq1 = makeRequest(romCmdRecord, blob1, 2);
    n = service_->handle(recReq1, respBuf_);
    ASSERT_EQ(static_cast<size_t>(n), sizeof(ChifPktHeader));

    // Record 2: Type 17 (Memory Device)
    std::vector<uint8_t> rec17 = {0x11, 0x54, 0x11, 0x00};
    rec17.resize(0x54, 0x00);
    rec17.push_back('D');
    rec17.push_back('I');
    rec17.push_back('M');
    rec17.push_back('M');
    rec17.push_back(0x00);
    rec17.push_back(0x00);

    auto blob17 = wrapInBlob({rec17});
    auto recReq2 = makeRequest(romCmdRecord, blob17, 3);
    n = service_->handle(recReq2, respBuf_);
    ASSERT_EQ(static_cast<size_t>(n), sizeof(ChifPktHeader));

    // End
    auto endReq = makeRequest(romCmdEnd, {}, 4);
    n = service_->handle(endReq, respBuf_);
    ASSERT_EQ(static_cast<size_t>(n), sizeof(ChifPktHeader));

    // Verify output file
    ASSERT_TRUE(std::filesystem::exists(writer_->outputPath()));

    // Read and verify the SMBIOS 3.0 EP
    std::ifstream in(writer_->outputPath(), std::ios::binary);
    in.seekg(sizeof(MdrHeader));

    Smbios3EntryPoint ep{};
    in.read(reinterpret_cast<char*>(&ep), sizeof(ep));

    EXPECT_EQ(ep.anchorString[0], '_');
    EXPECT_EQ(ep.anchorString[1], 'S');
    EXPECT_EQ(ep.anchorString[2], 'M');
    EXPECT_EQ(ep.anchorString[3], '3');
    EXPECT_EQ(ep.anchorString[4], '_');
    EXPECT_EQ(ep.smbiosMajor, 3);
    EXPECT_EQ(ep.smbiosMinor, 3);
    EXPECT_EQ(ep.structureTableMaxSize,
              static_cast<uint32_t>(rec1.size() + rec17.size()));
}

TEST_F(RomServiceTest, BlobCommand)
{
    // Begin
    auto beginReq = makeRequest(romCmdBegin, {}, 1);
    service_->handle(beginReq, respBuf_);

    // Blob: multiple records in a single blob-formatted payload
    std::vector<uint8_t> r1 = {0x01, 0x04, 0x01, 0x00, 0x00, 0x00};
    std::vector<uint8_t> r4 = {0x04, 0x04, 0x04, 0x00, 0x00, 0x00};
    auto blob = wrapInBlob({r1, r4});

    auto blobReq = makeRequest(romCmdBlob, blob, 2);
    int n = service_->handle(blobReq, respBuf_);
    ASSERT_EQ(static_cast<size_t>(n), sizeof(ChifPktHeader));

    // Verify data size matches raw records (blob wrapper stripped)
    EXPECT_EQ(writer_->dataSize(), r1.size() + r4.size());

    // End
    auto endReq = makeRequest(romCmdEnd, {}, 3);
    n = service_->handle(endReq, respBuf_);
    ASSERT_EQ(static_cast<size_t>(n), sizeof(ChifPktHeader));

    EXPECT_TRUE(std::filesystem::exists(writer_->outputPath()));
}

TEST_F(RomServiceTest, UnknownCommandDropped)
{
    // Unknown ROM command should return -1 (no response)
    auto req = makeRequest(0x00FF);
    int n = service_->handle(req, respBuf_);
    EXPECT_EQ(n, -1);
}

TEST_F(RomServiceTest, ResponseEchoesSequence)
{
    // Verify response echoes the request's sequence number
    auto req = makeRequest(romCmdBegin, {}, 0xABCD);
    int n = service_->handle(req, respBuf_);

    ASSERT_GT(n, 0);
    auto rspHdr = parseHeader(
        std::span<const uint8_t>(respBuf_.data(), static_cast<size_t>(n)));
    EXPECT_EQ(rspHdr.sequence, 0xABCD);
}
