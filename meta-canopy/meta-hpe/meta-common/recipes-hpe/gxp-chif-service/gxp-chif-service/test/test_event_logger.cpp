// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH

#include "../src/ev_storage.hpp"
#include "../src/event_logger.hpp"
#include "../src/smif_service.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <filesystem>

using namespace chif;

// --- EventLogger severity mapping tests ---

TEST(EventLoggerSeverity, InfoMapsToInformational)
{
    const char* level = EventLogger::mapSeverity(2);
    EXPECT_STREQ(level,
                 "xyz.openbmc_project.Logging.Entry.Level.Informational");
}

TEST(EventLoggerSeverity, RepairedMapsToInformational)
{
    const char* level = EventLogger::mapSeverity(6);
    EXPECT_STREQ(level,
                 "xyz.openbmc_project.Logging.Entry.Level.Informational");
}

TEST(EventLoggerSeverity, CautionMapsToWarning)
{
    const char* level = EventLogger::mapSeverity(9);
    EXPECT_STREQ(level, "xyz.openbmc_project.Logging.Entry.Level.Warning");
}

TEST(EventLoggerSeverity, CriticalMapsToCritical)
{
    const char* level = EventLogger::mapSeverity(15);
    EXPECT_STREQ(level, "xyz.openbmc_project.Logging.Entry.Level.Critical");
}

TEST(EventLoggerSeverity, UnknownDefaultsToWarning)
{
    const char* level = EventLogger::mapSeverity(0);
    EXPECT_STREQ(level, "xyz.openbmc_project.Logging.Entry.Level.Warning");

    level = EventLogger::mapSeverity(99);
    EXPECT_STREQ(level, "xyz.openbmc_project.Logging.Entry.Level.Warning");
}

// --- SmifService Quick Event Log (0x0146) tests ---

class QuickEventLogTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        testDir_ = std::filesystem::temp_directory_path() /
                   "chif_test_evtlog";
        std::filesystem::create_directories(testDir_);
        evPath_ = testDir_ / "evs.dat";
        storage_ = std::make_unique<EvStorage>(evPath_);
        storage_->load();
    }

    void TearDown() override
    {
        std::filesystem::remove_all(testDir_);
    }

    // Build a SMIF request packet
    std::vector<uint8_t> makeRequest(uint16_t command,
                                     std::span<const uint8_t> reqPayload = {},
                                     uint16_t seq = 1)
    {
        std::vector<uint8_t> pkt(sizeof(ChifPktHeader) + reqPayload.size());
        ChifPktHeader hdr{};
        hdr.pktSize = static_cast<uint16_t>(pkt.size());
        hdr.sequence = seq;
        hdr.command = command;
        hdr.serviceId = smifServiceId;
        hdr.version = 0x01;
        std::memcpy(pkt.data(), &hdr, sizeof(hdr));
        if (!reqPayload.empty())
        {
            std::memcpy(pkt.data() + sizeof(hdr), reqPayload.data(),
                        reqPayload.size());
        }
        return pkt;
    }

    // Parse uint32_t ErrorCode from response payload (for buildSimpleResponse)
    uint32_t getErrorCode(std::span<const uint8_t> resp)
    {
        if (resp.size() < sizeof(ChifPktHeader) + 4)
        {
            return 0xFFFFFFFF;
        }
        uint32_t ec = 0;
        std::memcpy(&ec, resp.data() + sizeof(ChifPktHeader), sizeof(ec));
        return ec;
    }

    // Parse uint8_t errorCode from 0x0146 response (pkt_8146 format)
    uint8_t getEventErrorCode(std::span<const uint8_t> resp)
    {
        if (resp.size() < sizeof(ChifPktHeader) + 1)
        {
            return 0xFF;
        }
        return resp[sizeof(ChifPktHeader)];
    }

    // Build a Quick Event Log (0x0146) request payload
    // HPE pkt_0146 layout:
    //   evtType(1) + matchCode(2) + evtClass(2) + evtCode(2) +
    //   severity(1) + evtVarLen(2) + varData(var)
    std::vector<uint8_t> makeEventPayload(uint8_t evtType, uint16_t matchCode,
                                          uint16_t evtClass, uint16_t evtCode,
                                          uint8_t severity,
                                          std::span<const uint8_t> varData = {})
    {
        std::vector<uint8_t> pl;
        pl.push_back(evtType);

        // matchCode (2 bytes LE)
        pl.push_back(static_cast<uint8_t>(matchCode & 0xFF));
        pl.push_back(static_cast<uint8_t>(matchCode >> 8));

        // evtClass (2 bytes LE)
        pl.push_back(static_cast<uint8_t>(evtClass & 0xFF));
        pl.push_back(static_cast<uint8_t>(evtClass >> 8));

        // evtCode (2 bytes LE)
        pl.push_back(static_cast<uint8_t>(evtCode & 0xFF));
        pl.push_back(static_cast<uint8_t>(evtCode >> 8));

        pl.push_back(severity);

        // evtVarLen (2 bytes LE)
        uint16_t varLen = static_cast<uint16_t>(varData.size());
        pl.push_back(static_cast<uint8_t>(varLen & 0xFF));
        pl.push_back(static_cast<uint8_t>(varLen >> 8));

        // variable data
        pl.insert(pl.end(), varData.begin(), varData.end());
        return pl;
    }

    std::filesystem::path testDir_;
    std::filesystem::path evPath_;
    std::unique_ptr<EvStorage> storage_;
    std::array<uint8_t, maxPacketSize> respBuf_{};
};

TEST_F(QuickEventLogTest, NullLoggerReturnsSuccess)
{
    // SmifService with nullptr eventLogger — should still succeed
    SmifService service(*storage_, nullptr);

    auto pl = makeEventPayload(1, 0x1234, 5, 0x0010, 2);
    auto req = makeRequest(smifCmdQuickEventLog, pl);

    int n = service.handle(req, respBuf_);
    ASSERT_GT(n, 0);
    EXPECT_EQ(getEventErrorCode(respBuf_), 0u);

    // Response should have command = 0x8146
    auto hdr = parseHeader(respBuf_);
    EXPECT_EQ(hdr.command,
              static_cast<uint16_t>(smifCmdQuickEventLog | responseBit));
}

TEST_F(QuickEventLogTest, ResponseHasCorrectFormat)
{
    SmifService service(*storage_, nullptr);

    uint8_t evtType = 2; // IEL
    auto pl = makeEventPayload(evtType, 0x5678, 10, 0x0020, 9);
    auto req = makeRequest(smifCmdQuickEventLog, pl);

    int n = service.handle(req, respBuf_);

    // Total response: header(8) + errorCode(1) + evtType(1) + evtNum(4) = 14
    ASSERT_EQ(n, 14);
    EXPECT_EQ(getEventErrorCode(respBuf_), 0u);

    // evtType echoed at offset 1 in payload
    EXPECT_EQ(respBuf_[sizeof(ChifPktHeader) + 1], evtType);

    // evtNum at offset 2 in payload — should be 0 with null logger
    uint32_t evtNum = 0;
    std::memcpy(&evtNum, respBuf_.data() + sizeof(ChifPktHeader) + 2,
                sizeof(evtNum));
    EXPECT_EQ(evtNum, 0u);
}

TEST_F(QuickEventLogTest, ShortPayloadRejected)
{
    SmifService service(*storage_, nullptr);

    // Only 5 bytes — less than the required 10
    std::vector<uint8_t> shortPl = {1, 2, 3, 4, 5};
    auto req = makeRequest(smifCmdQuickEventLog, shortPl);

    int n = service.handle(req, respBuf_);
    ASSERT_GT(n, 0);
    // buildSimpleResponse uses uint32_t errorCode
    EXPECT_EQ(getErrorCode(respBuf_), 2u);
}

TEST_F(QuickEventLogTest, EmptyPayloadRejected)
{
    SmifService service(*storage_, nullptr);

    auto req = makeRequest(smifCmdQuickEventLog);
    int n = service.handle(req, respBuf_);
    ASSERT_GT(n, 0);
    EXPECT_EQ(getErrorCode(respBuf_), 2u);
}

TEST_F(QuickEventLogTest, VariableDataTruncatedGracefully)
{
    SmifService service(*storage_, nullptr);

    // Claim 100 bytes of var data but only provide 5
    std::vector<uint8_t> pl;
    pl.push_back(1);    // evtType
    pl.push_back(0x00); // matchCode lo
    pl.push_back(0x00); // matchCode hi
    pl.push_back(3);    // evtClass lo
    pl.push_back(0);    // evtClass hi
    pl.push_back(0x01); // evtCode lo
    pl.push_back(0x00); // evtCode hi
    pl.push_back(2);    // severity
    pl.push_back(100);  // evtVarLen lo (claim 100)
    pl.push_back(0);    // evtVarLen hi
    // Only 5 bytes of actual var data
    pl.insert(pl.end(), {0xAA, 0xBB, 0xCC, 0xDD, 0xEE});

    auto req = makeRequest(smifCmdQuickEventLog, pl);
    int n = service.handle(req, respBuf_);
    ASSERT_GT(n, 0);
    EXPECT_EQ(getEventErrorCode(respBuf_), 0u);
}

TEST_F(QuickEventLogTest, SequenceEchoed)
{
    SmifService service(*storage_, nullptr);

    auto pl = makeEventPayload(1, 0, 0, 0, 2);
    auto req = makeRequest(smifCmdQuickEventLog, pl, 0xCAFE);

    int n = service.handle(req, respBuf_);
    ASSERT_GT(n, 0);

    auto hdr = parseHeader(respBuf_);
    EXPECT_EQ(hdr.sequence, 0xCAFE);
}

TEST_F(QuickEventLogTest, NineBytePayloadRejected)
{
    // 9 bytes is one short of the required 10-byte minimum
    SmifService service(*storage_, nullptr);

    std::vector<uint8_t> pl = {1, 0, 0, 0, 0, 0, 0, 2, 0};
    auto req = makeRequest(smifCmdQuickEventLog, pl);

    int n = service.handle(req, respBuf_);
    ASSERT_GT(n, 0);
    EXPECT_EQ(getErrorCode(respBuf_), 2u);
}

TEST_F(QuickEventLogTest, TenBytePayloadAccepted)
{
    // Exactly 10 bytes — minimum valid payload (0 bytes var data)
    SmifService service(*storage_, nullptr);

    auto pl = makeEventPayload(1, 0, 0, 0, 2);
    auto req = makeRequest(smifCmdQuickEventLog, pl);

    int n = service.handle(req, respBuf_);
    ASSERT_GT(n, 0);
    EXPECT_EQ(getEventErrorCode(respBuf_), 0u);
}
