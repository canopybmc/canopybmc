// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH

#include "../src/smbios_writer.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <fstream>

using namespace chif;

class SmbiosWriterTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Use a temporary directory for test output
        testDir_ = std::filesystem::temp_directory_path() / "chif_test_smbios";
        std::filesystem::create_directories(testDir_);
        writer_ = std::make_unique<SmbiosWriter>(testDir_);
    }

    void TearDown() override
    {
        std::filesystem::remove_all(testDir_);
    }

    std::filesystem::path testDir_;
    std::unique_ptr<SmbiosWriter> writer_;

    // Create a minimal SMBIOS Type 1 record (System Information)
    // with proper double-null terminator
    static std::vector<uint8_t> makeType1Record()
    {
        // Minimal Type 1: 4-byte header (type, length, handle) +
        // enough data to be valid + double-null string terminator
        std::vector<uint8_t> rec = {
            0x01,       // type = 1 (System Information)
            0x1B,       // length = 27 (min for type 1 v2.4)
            0x01, 0x00, // handle = 0x0001
        };
        // Fill formatted area with zeros to reach length
        rec.resize(0x1B, 0x00);
        // String table: manufacturer string + double null
        rec.push_back('T');
        rec.push_back('e');
        rec.push_back('s');
        rec.push_back('t');
        rec.push_back(0x00); // end of string
        rec.push_back(0x00); // end of string table (double null)
        return rec;
    }

    // Create a minimal SMBIOS Type 17 record (Memory Device)
    static std::vector<uint8_t> makeType17Record()
    {
        std::vector<uint8_t> rec = {
            0x11,       // type = 17 (Memory Device)
            0x54,       // length = 84 (v3.3+)
            0x11, 0x00, // handle = 0x0011
        };
        rec.resize(0x54, 0x00);
        // String: part number
        rec.push_back('D');
        rec.push_back('I');
        rec.push_back('M');
        rec.push_back('M');
        rec.push_back(0x00);
        rec.push_back(0x00);
        return rec;
    }

    // Wrap raw SMBIOS records in HPE ROM blob format for addRecord().
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
};

TEST_F(SmbiosWriterTest, EmptyFinalize)
{
    writer_->begin();
    // Finalize with no records should fail
    EXPECT_FALSE(writer_->finalize());
}

TEST_F(SmbiosWriterTest, SingleRecord)
{
    writer_->begin();
    auto rec = makeType1Record();
    auto blob = wrapInBlob({rec});
    writer_->addRecord(blob);
    EXPECT_EQ(writer_->dataSize(), rec.size());

    ASSERT_TRUE(writer_->finalize());

    // Verify the output file exists
    auto path = writer_->outputPath();
    ASSERT_TRUE(std::filesystem::exists(path));

    // Read back the file
    std::ifstream in(path, std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)),
                              std::istreambuf_iterator<char>());

    // File = MDR header (10) + SMBIOS 3.0 EP (24) + record data
    ASSERT_EQ(data.size(), sizeof(MdrHeader) + sizeof(Smbios3EntryPoint) +
                               rec.size());
}

TEST_F(SmbiosWriterTest, MdrHeaderFields)
{
    writer_->begin();
    auto rec = makeType1Record();
    auto blob = wrapInBlob({rec});
    writer_->addRecord(blob);
    ASSERT_TRUE(writer_->finalize());

    std::ifstream in(writer_->outputPath(), std::ios::binary);
    MdrHeader mdr{};
    in.read(reinterpret_cast<char*>(&mdr), sizeof(mdr));

    EXPECT_EQ(mdr.dirVer, 1u); // first finalize
    EXPECT_EQ(mdr.mdrType, 2u);
    EXPECT_GT(mdr.timestamp, 0u);
    EXPECT_EQ(mdr.dataSize,
              static_cast<uint32_t>(sizeof(Smbios3EntryPoint) + rec.size()));
}

TEST_F(SmbiosWriterTest, Smbios3EntryPointAnchor)
{
    writer_->begin();
    auto rec = makeType1Record();
    auto blob = wrapInBlob({rec});
    writer_->addRecord(blob);
    ASSERT_TRUE(writer_->finalize());

    std::ifstream in(writer_->outputPath(), std::ios::binary);
    // Skip MDR header
    in.seekg(sizeof(MdrHeader));

    Smbios3EntryPoint ep{};
    in.read(reinterpret_cast<char*>(&ep), sizeof(ep));

    // Verify anchor string "_SM3_"
    EXPECT_EQ(ep.anchorString[0], '_');
    EXPECT_EQ(ep.anchorString[1], 'S');
    EXPECT_EQ(ep.anchorString[2], 'M');
    EXPECT_EQ(ep.anchorString[3], '3');
    EXPECT_EQ(ep.anchorString[4], '_');

    // Version 3.3
    EXPECT_EQ(ep.smbiosMajor, 3);
    EXPECT_EQ(ep.smbiosMinor, 3);

    // Entry point length
    EXPECT_EQ(ep.entryPointLength, sizeof(Smbios3EntryPoint));

    // Entry point revision
    EXPECT_EQ(ep.entryPointRevision, 0x01);

    // Structure table max size = record data size
    EXPECT_EQ(ep.structureTableMaxSize, static_cast<uint32_t>(rec.size()));
}

TEST_F(SmbiosWriterTest, Smbios3Checksum)
{
    writer_->begin();
    auto blob = wrapInBlob({makeType1Record()});
    writer_->addRecord(blob);
    ASSERT_TRUE(writer_->finalize());

    std::ifstream in(writer_->outputPath(), std::ios::binary);
    in.seekg(sizeof(MdrHeader));

    Smbios3EntryPoint ep{};
    in.read(reinterpret_cast<char*>(&ep), sizeof(ep));

    // Verify checksum: all bytes of EP must sum to zero
    const auto* bytes = reinterpret_cast<const uint8_t*>(&ep);
    uint8_t sum = 0;
    for (size_t i = 0; i < sizeof(ep); i++)
    {
        sum += bytes[i];
    }
    EXPECT_EQ(sum, 0u) << "SMBIOS 3.0 EP checksum verification failed";
}

TEST_F(SmbiosWriterTest, MultipleRecords)
{
    writer_->begin();
    auto rec1 = makeType1Record();
    auto rec17 = makeType17Record();
    auto blob1 = wrapInBlob({rec1});
    auto blob17 = wrapInBlob({rec17});
    writer_->addRecord(blob1);
    writer_->addRecord(blob17);

    EXPECT_EQ(writer_->dataSize(), rec1.size() + rec17.size());
    ASSERT_TRUE(writer_->finalize());

    std::ifstream in(writer_->outputPath(), std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)),
                              std::istreambuf_iterator<char>());

    size_t expected = sizeof(MdrHeader) + sizeof(Smbios3EntryPoint) +
                      rec1.size() + rec17.size();
    EXPECT_EQ(data.size(), expected);
}

TEST_F(SmbiosWriterTest, DirectoryVersionIncrements)
{
    // First write
    writer_->begin();
    auto blob1 = wrapInBlob({makeType1Record()});
    writer_->addRecord(blob1);
    ASSERT_TRUE(writer_->finalize());

    std::ifstream in1(writer_->outputPath(), std::ios::binary);
    MdrHeader mdr1{};
    in1.read(reinterpret_cast<char*>(&mdr1), sizeof(mdr1));
    in1.close();

    // Second write
    writer_->begin();
    auto blob2 = wrapInBlob({makeType17Record()});
    writer_->addRecord(blob2);
    ASSERT_TRUE(writer_->finalize());

    std::ifstream in2(writer_->outputPath(), std::ios::binary);
    MdrHeader mdr2{};
    in2.read(reinterpret_cast<char*>(&mdr2), sizeof(mdr2));

    EXPECT_EQ(mdr2.dirVer, mdr1.dirVer + 1);
}

TEST_F(SmbiosWriterTest, AtomicWrite)
{
    writer_->begin();
    auto blob = wrapInBlob({makeType1Record()});
    writer_->addRecord(blob);
    ASSERT_TRUE(writer_->finalize());

    // The temp file should not exist after finalize
    auto tempPath = testDir_ / "smbios_temp";
    EXPECT_FALSE(std::filesystem::exists(tempPath));

    // The final file should exist
    EXPECT_TRUE(std::filesystem::exists(writer_->outputPath()));
}

TEST_F(SmbiosWriterTest, RecordDataPreserved)
{
    writer_->begin();
    auto rec = makeType1Record();
    auto blob = wrapInBlob({rec});
    writer_->addRecord(blob);
    ASSERT_TRUE(writer_->finalize());

    std::ifstream in(writer_->outputPath(), std::ios::binary);

    // Skip MDR header + EP
    in.seekg(sizeof(MdrHeader) + sizeof(Smbios3EntryPoint));

    std::vector<uint8_t> readBack(rec.size());
    in.read(reinterpret_cast<char*>(readBack.data()),
            static_cast<std::streamsize>(rec.size()));

    EXPECT_EQ(readBack, rec) << "Record data not preserved in output file";
}
