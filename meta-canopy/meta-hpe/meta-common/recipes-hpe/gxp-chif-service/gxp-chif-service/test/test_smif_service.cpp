// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH

#include "../src/ev_storage.hpp"
#include "../src/platform_info.hpp"
#include "../src/smif_service.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>

using namespace chif;

class SmifServiceTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        testDir_ = std::filesystem::temp_directory_path() / "chif_test_smif";
        std::filesystem::create_directories(testDir_);
        evPath_ = testDir_ / "evs.dat";
        storage_ = std::make_unique<EvStorage>(evPath_);
        storage_->load();

        // Create fake os-release for platform info tests
        osReleasePath_ = testDir_ / "os-release";
        {
            std::ofstream out(osReleasePath_);
            out << "ID=\"openbmc-phosphor\"\n";
            out << "NAME=\"Canopy OpenBMC\"\n";
            out << "VERSION=\"3.0.0-dev\"\n";
            out << "VERSION_ID=3.0.0-dev\n";
            out << "BUILD_ID=\"2026-03-19\"\n";
        }

        // Create fake serial-number (device-tree style: NUL-terminated)
        serialPath_ = testDir_ / "serial-number";
        {
            std::ofstream out(serialPath_, std::ios::binary);
            std::string serial = "CZUD2102YJ";
            out.write(serial.c_str(), serial.size() + 1); // include NUL
        }

        service_ = std::make_unique<SmifService>(
            *storage_, nullptr, osReleasePath_.string(),
            serialPath_.string());
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

    // Parse ErrorCode from response payload
    uint32_t getErrorCode(std::span<const uint8_t> resp)
    {
        if (resp.size() < sizeof(ChifPktHeader) + 4)
        {
            return 0xFFFFFFFF;
        }
        uint32_t ec;
        std::memcpy(&ec, resp.data() + sizeof(ChifPktHeader), sizeof(ec));
        return ec;
    }

    // Build a Set EV request payload
    std::vector<uint8_t> makeSetEvPayload(const std::string& name,
                                          std::span<const uint8_t> data)
    {
        std::vector<uint8_t> pl;
        // flags(1) + reserved(3)
        pl.push_back(evFlagSet);
        pl.push_back(0);
        pl.push_back(0);
        pl.push_back(0);

        // name(32)
        char nameBuf[maxEvNameLen] = {};
        std::strncpy(nameBuf, name.c_str(), sizeof(nameBuf) - 1);
        pl.insert(pl.end(), nameBuf, nameBuf + sizeof(nameBuf));

        // sz_ev(2)
        uint16_t sz = static_cast<uint16_t>(data.size());
        pl.insert(pl.end(), reinterpret_cast<uint8_t*>(&sz),
                  reinterpret_cast<uint8_t*>(&sz) + 2);

        // data
        pl.insert(pl.end(), data.begin(), data.end());
        return pl;
    }

    std::filesystem::path testDir_;
    std::filesystem::path evPath_;
    std::filesystem::path osReleasePath_;
    std::filesystem::path serialPath_;
    std::unique_ptr<EvStorage> storage_;
    std::unique_ptr<SmifService> service_;
    std::array<uint8_t, maxPacketSize> respBuf_{};
};

TEST_F(SmifServiceTest, ServiceId)
{
    EXPECT_EQ(service_->serviceId(), smifServiceId);
}

TEST_F(SmifServiceTest, BiosReadyReturnsSuccess)
{
    auto req = makeRequest(smifCmdBiosReady);
    int n = service_->handle(req, respBuf_);
    ASSERT_GT(n, 0);
    EXPECT_EQ(getErrorCode(respBuf_), 0u);

    auto hdr = parseHeader(respBuf_);
    EXPECT_EQ(hdr.command,
              static_cast<uint16_t>(smifCmdBiosReady | responseBit));
}

TEST_F(SmifServiceTest, GetEvByIndexEmpty)
{
    std::vector<uint8_t> pl(8, 0); // idx=0, pad=0
    auto req = makeRequest(smifCmdGetEvByIndex, pl);
    int n = service_->handle(req, respBuf_);
    ASSERT_GT(n, 0);
    EXPECT_EQ(getErrorCode(respBuf_), 1u);
}

TEST_F(SmifServiceTest, GetEvByIndexSuccess)
{
    std::vector<uint8_t> data = {0xAA, 0xBB, 0xCC};
    storage_->set("TESTEV", data);

    std::vector<uint8_t> pl(8, 0);
    auto req = makeRequest(smifCmdGetEvByIndex, pl);
    int n = service_->handle(req, respBuf_);

    ASSERT_GT(n, static_cast<int>(sizeof(ChifPktHeader) + 4));
    EXPECT_EQ(getErrorCode(respBuf_), 0u);

    // Check name field
    char name[maxEvNameLen] = {};
    std::memcpy(name, respBuf_.data() + sizeof(ChifPktHeader) + 4,
                maxEvNameLen);
    EXPECT_STREQ(name, "TESTEV");

    // Check data size
    uint16_t szEv = 0;
    std::memcpy(&szEv, respBuf_.data() + sizeof(ChifPktHeader) + 36,
                sizeof(szEv));
    EXPECT_EQ(szEv, 3u);

    // Check data bytes
    EXPECT_EQ(respBuf_[sizeof(ChifPktHeader) + 38], 0xAA);
    EXPECT_EQ(respBuf_[sizeof(ChifPktHeader) + 39], 0xBB);
    EXPECT_EQ(respBuf_[sizeof(ChifPktHeader) + 40], 0xCC);
}

TEST_F(SmifServiceTest, SetEvCommand)
{
    std::vector<uint8_t> data = {0x11, 0x22, 0x33, 0x44};
    auto pl = makeSetEvPayload("MYEV", data);

    auto req = makeRequest(smifCmdSetDeleteEv, pl);
    int n = service_->handle(req, respBuf_);
    ASSERT_GT(n, 0);
    EXPECT_EQ(getErrorCode(respBuf_), 0u);

    auto ev = storage_->getByName("MYEV");
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->data, data);
}

TEST_F(SmifServiceTest, DeleteEvCommand)
{
    std::vector<uint8_t> data = {0x01};
    storage_->set("TODEL", data);
    EXPECT_EQ(storage_->count(), 1u);

    // Build Delete request
    std::vector<uint8_t> pl;
    pl.push_back(evFlagDelete);
    pl.push_back(0);
    pl.push_back(0);
    pl.push_back(0);

    char name[maxEvNameLen] = {};
    std::strncpy(name, "TODEL", sizeof(name) - 1);
    pl.insert(pl.end(), name, name + sizeof(name));

    auto req = makeRequest(smifCmdSetDeleteEv, pl);
    int n = service_->handle(req, respBuf_);
    ASSERT_GT(n, 0);
    EXPECT_EQ(getErrorCode(respBuf_), 0u);
    EXPECT_EQ(storage_->count(), 0u);
}

TEST_F(SmifServiceTest, DeleteAllEvCommand)
{
    std::vector<uint8_t> d1 = {0x01};
    std::vector<uint8_t> d2 = {0x02};
    storage_->set("EV1", d1);
    storage_->set("EV2", d2);
    EXPECT_EQ(storage_->count(), 2u);

    std::vector<uint8_t> pl;
    pl.push_back(evFlagDeleteAll);
    pl.push_back(0);
    pl.push_back(0);
    pl.push_back(0);

    auto req = makeRequest(smifCmdSetDeleteEv, pl);
    int n = service_->handle(req, respBuf_);
    ASSERT_GT(n, 0);
    EXPECT_EQ(getErrorCode(respBuf_), 0u);
    EXPECT_EQ(storage_->count(), 0u);
}

TEST_F(SmifServiceTest, GetEvByNameNotFound)
{
    char name[maxEvNameLen] = {};
    std::strncpy(name, "NOPE", sizeof(name) - 1);
    std::vector<uint8_t> pl(name, name + sizeof(name));

    auto req = makeRequest(smifCmdGetEvByName, pl);
    int n = service_->handle(req, respBuf_);
    ASSERT_GT(n, 0);
    EXPECT_EQ(getErrorCode(respBuf_), 1u);
}

TEST_F(SmifServiceTest, GetEvByNameSuccess)
{
    std::vector<uint8_t> data = {0xDE, 0xAD};
    storage_->set("FOUND", data);

    char name[maxEvNameLen] = {};
    std::strncpy(name, "FOUND", sizeof(name) - 1);
    std::vector<uint8_t> pl(name, name + sizeof(name));

    auto req = makeRequest(smifCmdGetEvByName, pl);
    int n = service_->handle(req, respBuf_);
    ASSERT_GT(n, 0);
    EXPECT_EQ(getErrorCode(respBuf_), 0u);

    uint16_t szEv = 0;
    std::memcpy(&szEv, respBuf_.data() + sizeof(ChifPktHeader) + 36,
                sizeof(szEv));
    EXPECT_EQ(szEv, 2u);

    EXPECT_EQ(respBuf_[sizeof(ChifPktHeader) + 38], 0xDE);
    EXPECT_EQ(respBuf_[sizeof(ChifPktHeader) + 39], 0xAD);
}

TEST_F(SmifServiceTest, EvStatistics)
{
    std::vector<uint8_t> data = {0x01, 0x02, 0x03};
    storage_->set("EV1", data);

    auto req = makeRequest(smifCmdEvStats);
    int n = service_->handle(req, respBuf_);

    ASSERT_EQ(n, 24);
    EXPECT_EQ(getErrorCode(respBuf_), 0u);

    uint32_t presentEvs = 0;
    std::memcpy(&presentEvs,
                respBuf_.data() + sizeof(ChifPktHeader) + 8,
                sizeof(presentEvs));
    EXPECT_EQ(presentEvs, 1u);

    uint32_t maxSz = 0;
    std::memcpy(&maxSz,
                respBuf_.data() + sizeof(ChifPktHeader) + 12,
                sizeof(maxSz));
    EXPECT_EQ(maxSz, static_cast<uint32_t>(EvStorage::maxSize()));
}

TEST_F(SmifServiceTest, EvState)
{
    auto req = makeRequest(smifCmdEvState);
    int n = service_->handle(req, respBuf_);
    ASSERT_GT(n, 0);
    EXPECT_EQ(getErrorCode(respBuf_), 0u);
}

TEST_F(SmifServiceTest, ResponseBitSet)
{
    auto req = makeRequest(smifCmdEvStats);
    int n = service_->handle(req, respBuf_);
    ASSERT_GT(n, 0);

    auto hdr = parseHeader(respBuf_);
    EXPECT_EQ(hdr.command,
              static_cast<uint16_t>(smifCmdEvStats | responseBit));
}

TEST_F(SmifServiceTest, SequenceEchoed)
{
    auto req = makeRequest(smifCmdEvStats, {}, 0xBEEF);
    int n = service_->handle(req, respBuf_);
    ASSERT_GT(n, 0);

    auto hdr = parseHeader(respBuf_);
    EXPECT_EQ(hdr.sequence, 0xBEEF);
}

TEST_F(SmifServiceTest, AuthStatusReturnsNotImplemented)
{
    auto req = makeRequest(smifCmdGetEvAuthStatus);
    int n = service_->handle(req, respBuf_);
    ASSERT_GT(n, 0);
    EXPECT_EQ(getErrorCode(respBuf_), 1u);
}

TEST_F(SmifServiceTest, UnknownCommandReturnsSuccess)
{
    auto req = makeRequest(0x9999);
    int n = service_->handle(req, respBuf_);
    ASSERT_GT(n, 0);
    EXPECT_EQ(getErrorCode(respBuf_), 0u);
}

TEST_F(SmifServiceTest, SetThenGetByIndex)
{
    std::vector<uint8_t> data = {0xCA, 0xFE};
    auto setPayload = makeSetEvPayload("ROUNDTRIP", data);
    auto setReq = makeRequest(smifCmdSetDeleteEv, setPayload);
    service_->handle(setReq, respBuf_);

    std::vector<uint8_t> getPayload(8, 0);
    auto getReq = makeRequest(smifCmdGetEvByIndex, getPayload, 2);
    int n = service_->handle(getReq, respBuf_);
    ASSERT_GT(n, 0);
    EXPECT_EQ(getErrorCode(respBuf_), 0u);

    char name[maxEvNameLen] = {};
    std::memcpy(name, respBuf_.data() + sizeof(ChifPktHeader) + 4,
                maxEvNameLen);
    EXPECT_STREQ(name, "ROUNDTRIP");
}

TEST_F(SmifServiceTest, SetEvShortPayload)
{
    std::vector<uint8_t> pl = {evFlagSet, 0, 0, 0};
    auto req = makeRequest(smifCmdSetDeleteEv, pl);
    int n = service_->handle(req, respBuf_);
    ASSERT_GT(n, 0);
    EXPECT_EQ(getErrorCode(respBuf_), 2u);
}

TEST_F(SmifServiceTest, DeleteNonexistentEv)
{
    std::vector<uint8_t> pl;
    pl.push_back(evFlagDelete);
    pl.push_back(0);
    pl.push_back(0);
    pl.push_back(0);

    char name[maxEvNameLen] = {};
    std::strncpy(name, "NONEXISTENT", sizeof(name) - 1);
    pl.insert(pl.end(), name, name + sizeof(name));

    auto req = makeRequest(smifCmdSetDeleteEv, pl);
    int n = service_->handle(req, respBuf_);
    ASSERT_GT(n, 0);
    EXPECT_EQ(getErrorCode(respBuf_), 1u);
}

TEST_F(SmifServiceTest, GetEvByNameShortPayload)
{
    std::vector<uint8_t> pl(10, 0);
    auto req = makeRequest(smifCmdGetEvByName, pl);
    int n = service_->handle(req, respBuf_);
    ASSERT_GT(n, 0);
    EXPECT_EQ(getErrorCode(respBuf_), 2u);
}

// --- Platform info command tests ---

TEST_F(SmifServiceTest, HwRevisionResponseSize)
{
    // 0x0002 response: header(8) + payload(95) = 103 bytes
    auto req = makeRequest(smifCmdHwRevision);
    int n = service_->handle(req, respBuf_);
    ASSERT_EQ(n, static_cast<int>(sizeof(ChifPktHeader) + 95));
}

TEST_F(SmifServiceTest, HwRevisionErrorCodeZero)
{
    auto req = makeRequest(smifCmdHwRevision);
    int n = service_->handle(req, respBuf_);
    ASSERT_GT(n, 0);
    EXPECT_EQ(getErrorCode(respBuf_), 0u);
}

TEST_F(SmifServiceTest, HwRevisionResponseBit)
{
    auto req = makeRequest(smifCmdHwRevision);
    int n = service_->handle(req, respBuf_);
    ASSERT_GT(n, 0);

    auto hdr = parseHeader(respBuf_);
    EXPECT_EQ(hdr.command,
              static_cast<uint16_t>(smifCmdHwRevision | responseBit));
}

TEST_F(SmifServiceTest, HwRevisionSequenceEchoed)
{
    auto req = makeRequest(smifCmdHwRevision, {}, 0x1234);
    int n = service_->handle(req, respBuf_);
    ASSERT_GT(n, 0);

    auto hdr = parseHeader(respBuf_);
    EXPECT_EQ(hdr.sequence, 0x1234);
}

TEST_F(SmifServiceTest, HwRevisionMaxUsers)
{
    auto req = makeRequest(smifCmdHwRevision);
    service_->handle(req, respBuf_);

    auto resp = respBuf_.data() + sizeof(ChifPktHeader);
    uint16_t maxUsers = 0;
    std::memcpy(&maxUsers, resp + 0x06, sizeof(maxUsers));
    EXPECT_EQ(maxUsers, 12u);
}

TEST_F(SmifServiceTest, HwRevisionFirmwareVersion)
{
    auto req = makeRequest(smifCmdHwRevision);
    service_->handle(req, respBuf_);

    auto resp = respBuf_.data() + sizeof(ChifPktHeader);
    uint16_t fwVer = 0;
    std::memcpy(&fwVer, resp + 0x08, sizeof(fwVer));
    // "3.0.0-dev" -> major=3, minor=0 -> 0x0300
    EXPECT_EQ(fwVer, 0x0300);
}

TEST_F(SmifServiceTest, HwRevisionHwRevGxp)
{
    auto req = makeRequest(smifCmdHwRevision);
    service_->handle(req, respBuf_);

    auto resp = respBuf_.data() + sizeof(ChifPktHeader);
    uint32_t hwRev = 0;
    std::memcpy(&hwRev, resp + 0x16, sizeof(hwRev));
    EXPECT_EQ(hwRev, 0x06u);
}

TEST_F(SmifServiceTest, HwRevisionBoardSerial)
{
    auto req = makeRequest(smifCmdHwRevision);
    service_->handle(req, respBuf_);

    auto resp = respBuf_.data() + sizeof(ChifPktHeader);
    char serial[20] = {};
    std::memcpy(serial, resp + 0x1A, sizeof(serial));
    EXPECT_STREQ(serial, "CZUD2102YJ");
}

TEST_F(SmifServiceTest, HwRevisionTimestamp)
{
    auto before = static_cast<uint32_t>(std::time(nullptr));

    auto req = makeRequest(smifCmdHwRevision);
    service_->handle(req, respBuf_);

    auto after = static_cast<uint32_t>(std::time(nullptr));

    auto resp = respBuf_.data() + sizeof(ChifPktHeader);
    uint32_t ts = 0;
    std::memcpy(&ts, resp + 0x12, sizeof(ts));
    EXPECT_GE(ts, before);
    EXPECT_LE(ts, after);
}

TEST_F(SmifServiceTest, HwRevisionClassAndApp)
{
    auto req = makeRequest(smifCmdHwRevision);
    service_->handle(req, respBuf_);

    auto resp = respBuf_.data() + sizeof(ChifPktHeader);
    EXPECT_EQ(resp[0x3C], 7u);   // class_ = 7
    EXPECT_EQ(resp[0x3F], 5u);   // application = 5
    EXPECT_EQ(resp[0x40], 3u);   // security_state = PRODUCTION
}

TEST_F(SmifServiceTest, DateTimeResponseSize)
{
    // 0x0055 response: header(8) + payload(12) = 20 bytes
    auto req = makeRequest(smifCmdDateTime);
    int n = service_->handle(req, respBuf_);
    ASSERT_EQ(n, static_cast<int>(sizeof(ChifPktHeader) + 12));
}

TEST_F(SmifServiceTest, DateTimeErrorCodeZero)
{
    auto req = makeRequest(smifCmdDateTime);
    int n = service_->handle(req, respBuf_);
    ASSERT_GT(n, 0);
    EXPECT_EQ(getErrorCode(respBuf_), 0u);
}

TEST_F(SmifServiceTest, DateTimeResponseBit)
{
    auto req = makeRequest(smifCmdDateTime);
    int n = service_->handle(req, respBuf_);
    ASSERT_GT(n, 0);

    auto hdr = parseHeader(respBuf_);
    EXPECT_EQ(hdr.command,
              static_cast<uint16_t>(smifCmdDateTime | responseBit));
}

TEST_F(SmifServiceTest, DateTimeTimestamp)
{
    auto before = static_cast<uint32_t>(std::time(nullptr));

    auto req = makeRequest(smifCmdDateTime);
    service_->handle(req, respBuf_);

    auto after = static_cast<uint32_t>(std::time(nullptr));

    auto resp = respBuf_.data() + sizeof(ChifPktHeader);
    uint32_t ts = 0;
    std::memcpy(&ts, resp + 4, sizeof(ts));
    EXPECT_GE(ts, before);
    EXPECT_LE(ts, after);
}

TEST_F(SmifServiceTest, DateTimeUtcOffset)
{
    auto req = makeRequest(smifCmdDateTime);
    service_->handle(req, respBuf_);

    auto resp = respBuf_.data() + sizeof(ChifPktHeader);
    int16_t tzOffset = 0;
    std::memcpy(&tzOffset, resp + 8, sizeof(tzOffset));
    EXPECT_EQ(tzOffset, 0); // UTC

    EXPECT_EQ(resp[10], 0u); // daylight = 0
    EXPECT_EQ(resp[11], 0u); // pad = 0
}

TEST_F(SmifServiceTest, FlashInfoResponseSize)
{
    // 0x0050 response: header(8) + payload(181) = 189 bytes
    auto req = makeRequest(smifCmdFlashInfo);
    int n = service_->handle(req, respBuf_);
    ASSERT_EQ(n, static_cast<int>(sizeof(ChifPktHeader) + 181));
}

TEST_F(SmifServiceTest, FlashInfoErrorCodeZero)
{
    auto req = makeRequest(smifCmdFlashInfo);
    int n = service_->handle(req, respBuf_);
    ASSERT_GT(n, 0);
    EXPECT_EQ(getErrorCode(respBuf_), 0u);
}

TEST_F(SmifServiceTest, FlashInfoResponseBit)
{
    auto req = makeRequest(smifCmdFlashInfo);
    int n = service_->handle(req, respBuf_);
    ASSERT_GT(n, 0);

    auto hdr = parseHeader(respBuf_);
    EXPECT_EQ(hdr.command,
              static_cast<uint16_t>(smifCmdFlashInfo | responseBit));
}

TEST_F(SmifServiceTest, FlashInfoSectorSize)
{
    auto req = makeRequest(smifCmdFlashInfo);
    service_->handle(req, respBuf_);

    auto resp = respBuf_.data() + sizeof(ChifPktHeader);
    uint32_t sectorSize = 0;
    std::memcpy(&sectorSize, resp + 0x04, sizeof(sectorSize));
    EXPECT_EQ(sectorSize, 0x10000u);
}

TEST_F(SmifServiceTest, FlashInfoFirmwareDate)
{
    auto req = makeRequest(smifCmdFlashInfo);
    service_->handle(req, respBuf_);

    auto resp = respBuf_.data() + sizeof(ChifPktHeader);
    char date[20] = {};
    std::memcpy(date, resp + 0x08, sizeof(date));
    EXPECT_STREQ(date, "2026-03-19");
}

TEST_F(SmifServiceTest, FlashInfoFirmwareNumber)
{
    auto req = makeRequest(smifCmdFlashInfo);
    service_->handle(req, respBuf_);

    auto resp = respBuf_.data() + sizeof(ChifPktHeader);
    char fwNum[10] = {};
    std::memcpy(fwNum, resp + 0x30, sizeof(fwNum));
    EXPECT_STREQ(fwNum, "3.0.0-dev");
}

TEST_F(SmifServiceTest, FlashInfoFirmwareName)
{
    auto req = makeRequest(smifCmdFlashInfo);
    service_->handle(req, respBuf_);

    auto resp = respBuf_.data() + sizeof(ChifPktHeader);
    char fwName[20] = {};
    std::memcpy(fwName, resp + 0x44, sizeof(fwName));
    EXPECT_STREQ(fwName, "Canopy OpenBMC");
}

TEST_F(SmifServiceTest, FlashInfoAsicValues)
{
    auto req = makeRequest(smifCmdFlashInfo);
    service_->handle(req, respBuf_);

    auto resp = respBuf_.data() + sizeof(ChifPktHeader);
    EXPECT_EQ(resp[0x58], 0x06u); // AsicMajor = GXP
    EXPECT_EQ(resp[0x59], 0x02u); // AsicMinor
    EXPECT_EQ(resp[0x5A], 0x09u); // CPLDVersion

    uint32_t asicRtl = 0;
    std::memcpy(&asicRtl, resp + 0x5C, sizeof(asicRtl));
    EXPECT_EQ(asicRtl, 0x00060202u);
}

TEST_F(SmifServiceTest, FlashInfoBootleg)
{
    auto req = makeRequest(smifCmdFlashInfo);
    service_->handle(req, respBuf_);

    auto resp = respBuf_.data() + sizeof(ChifPktHeader);
    char bootleg[80] = {};
    std::memcpy(bootleg, resp + 0x60, sizeof(bootleg));
    EXPECT_STREQ(bootleg, "GXP OpenBMC");
}

// --- Platform info helper tests ---

TEST_F(SmifServiceTest, ParseFirmwareVersionMajorMinor)
{
    EXPECT_EQ(parseFirmwareVersion("3.0.0-dev"), 0x0300);
    EXPECT_EQ(parseFirmwareVersion("3.1.0"), 0x0301);
    EXPECT_EQ(parseFirmwareVersion("1.2"), 0x0102);
    EXPECT_EQ(parseFirmwareVersion("255.255.0"), 0xFFFF);
    EXPECT_EQ(parseFirmwareVersion("0.0.0"), 0x0000);
}

TEST_F(SmifServiceTest, ReadOsReleaseFieldFromTestFile)
{
    auto ver = readOsReleaseField("VERSION_ID", osReleasePath_.string());
    ASSERT_TRUE(ver.has_value());
    EXPECT_EQ(*ver, "3.0.0-dev");

    auto build = readOsReleaseField("BUILD_ID", osReleasePath_.string());
    ASSERT_TRUE(ver.has_value());
    EXPECT_EQ(*build, "2026-03-19");

    auto missing = readOsReleaseField("NONEXISTENT", osReleasePath_.string());
    EXPECT_FALSE(missing.has_value());
}

TEST_F(SmifServiceTest, ReadDeviceTreeStringFromTestFile)
{
    auto serial = readDeviceTreeString(serialPath_.string());
    EXPECT_EQ(serial, "CZUD2102YJ");
}

TEST_F(SmifServiceTest, ReadDeviceTreeStringMissingFile)
{
    auto result = readDeviceTreeString("/nonexistent/path");
    EXPECT_TRUE(result.empty());
}
