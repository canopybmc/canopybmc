// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH

#include "../src/boot_service.hpp"

#include <gtest/gtest.h>

using namespace chif;
using Sources = BootService::Sources;
using Modes = BootService::Modes;

TEST(BootServiceOptionNumber, ParsesHexSuffix)
{
    EXPECT_EQ(BootService::optionNumber("CQSBOOT0000"),
              std::optional<uint16_t>(0x0000));
    EXPECT_EQ(BootService::optionNumber("CQSBOOT0012"),
              std::optional<uint16_t>(0x0012));
    EXPECT_EQ(BootService::optionNumber("CQSBOOT000F"),
              std::optional<uint16_t>(0x000F));
    EXPECT_EQ(BootService::optionNumber("CQSBOOT00a3"),
              std::optional<uint16_t>(0x00A3));
}

TEST(BootServiceOptionNumber, RejectsNonOptionNames)
{
    EXPECT_FALSE(BootService::optionNumber("CQHBOOTORDER").has_value());
    EXPECT_FALSE(BootService::optionNumber("CQTBOOTNEXT").has_value());
    EXPECT_FALSE(BootService::optionNumber("CQSBOOT01").has_value()); // short
    EXPECT_FALSE(
        BootService::optionNumber("CQSBOOT000G").has_value());        // non-hex
    EXPECT_FALSE(BootService::optionNumber("CQSBOOT00012").has_value()); // long
}

TEST(BootServiceKeywords, SetupModeWins)
{
    auto kw = BootService::targetKeywords(Sources::Network, Modes::Setup);
    ASSERT_FALSE(kw.empty());
    EXPECT_EQ(kw.front(), "System Utilities");
}

TEST(BootServiceKeywords, DefaultSourceYieldsNoOverride)
{
    EXPECT_TRUE(
        BootService::targetKeywords(Sources::Default, Modes::Regular).empty());
}

TEST(BootServiceMatch, PrefersHigherPriorityKeyword)
{
    std::vector<std::pair<uint16_t, std::string>> options = {
        {0x0009, "PXE Boot"},
        {0x0006, "Network Boot"},
        {0x0011, "OCP Slot 15 Port 1 : Broadcom NetXtreme (PXE IPv4)"},
    };
    auto kw = BootService::targetKeywords(Sources::Network, Modes::Regular);
    EXPECT_EQ(BootService::matchOption(options, kw),
              std::optional<uint16_t>(0x0006));
}

TEST(BootServiceMatch, ReturnsNulloptWhenNothingMatches)
{
    std::vector<std::pair<uint16_t, std::string>> options = {
        {0x0000, "System Utilities"},
        {0x0006, "Network Boot"},
    };
    auto kw = BootService::targetKeywords(Sources::Disk, Modes::Regular);
    EXPECT_FALSE(BootService::matchOption(options, kw).has_value());
}

TEST(BootServiceReorder, MovesExistingOptionToFront)
{
    // Order [00,01,02,05,06] with option 06 requested.
    std::vector<uint8_t> current = {0x00, 0x00, 0x01, 0x00, 0x02,
                                    0x00, 0x05, 0x00, 0x06, 0x00};
    auto out = BootService::reorderBootOrder(current, 0x0006);
    EXPECT_EQ(out, (std::vector<uint8_t>{0x06, 0x00, 0x00, 0x00, 0x01, 0x00,
                                         0x02, 0x00, 0x05, 0x00}));
}

TEST(BootServiceReorder, InsertsUnknownOptionAtFront)
{
    std::vector<uint8_t> current = {0x00, 0x00, 0x01, 0x00};
    auto out = BootService::reorderBootOrder(current, 0x0007);
    EXPECT_EQ(out, (std::vector<uint8_t>{0x07, 0x00, 0x00, 0x00, 0x01, 0x00}));
}

TEST(BootServiceEncode, EncodesLittleEndianUint16)
{
    EXPECT_EQ(BootService::encodeOption(0x0006),
              (std::vector<uint8_t>{0x06, 0x00}));
    EXPECT_EQ(BootService::encodeOption(0x0112),
              (std::vector<uint8_t>{0x12, 0x01}));
}

static std::vector<std::pair<uint16_t, std::string>> realOptions()
{
    return {
        {0x0005, "Boot Menu"},
        {0x0000, "System Utilities"},
        {0x0001, "Embedded UEFI Shell"},
        {0x0002, "Non-bootable Hotkey"},
        {0x0003, "Embedded iPXE"},
        {0x0004, "Diagnose Error"},
        {0x0006, "Network Boot"},
        {0x0007, "View BIOS Event Log"},
        {0x0008, "HTTP Boot"},
        {0x0009, "PXE Boot"},
        {0x000A, "Embedded Diagnostics"},
        {0x000B, "Generic USB Boot"},
        {0x000C, "Rear USB 1 : PiKVM PiKVM Composite Device"},
        {0x000D, "OCP Slot 14 : HPE MR408i-o Gen11 - Box 1, Bay 1"},
        {0x000E, "OCP Slot 14 : HPE MR408i-o Gen11 - Box 1, Bay 2"},
        {0x0011, "OCP Slot 15 Port 1 : Broadcom NetXtreme Gigabit Ethernet - "
                 "NIC (HTTP(S) IPv4)"},
        {0x0012, "OCP Slot 15 Port 1 : Broadcom NetXtreme Gigabit Ethernet - "
                 "NIC (PXE IPv4)"},
        {0x000F, "OCP Slot 15 Port 1 : Broadcom NetXtreme Gigabit Ethernet - "
                 "NIC (PXE IPv6)"},
        {0x0010, "OCP Slot 15 Port 1 : Broadcom NetXtreme Gigabit Ethernet - "
                 "NIC (HTTP(S) IPv6)"},
    };
}

static std::optional<uint16_t> resolve(Sources source, Modes mode)
{
    return BootService::matchOption(realOptions(),
                                    BootService::targetKeywords(source, mode));
}

TEST(BootServiceRealDump, ResolvesCommonTargets)
{
    // Generic entries win over the per-NIC PXE/HTTP options thanks to keyword
    // priority ("Network Boot"/"HTTP Boot" before "PXE"/"HTTP").
    EXPECT_EQ(resolve(Sources::Network, Modes::Regular),
              std::optional<uint16_t>(0x0006));
    EXPECT_EQ(resolve(Sources::HTTP, Modes::Regular),
              std::optional<uint16_t>(0x0008));
    // "Generic USB Boot" precedes "Rear USB ..." in storage order.
    EXPECT_EQ(resolve(Sources::RemovableMedia, Modes::Regular),
              std::optional<uint16_t>(0x000B));
    // "Bay" distinguishes the storage controller from the NIC "Slot" entries.
    EXPECT_EQ(resolve(Sources::Disk, Modes::Regular),
              std::optional<uint16_t>(0x000D));
    // Setup is requested via Boot.Mode regardless of source.
    EXPECT_EQ(resolve(Sources::Default, Modes::Setup),
              std::optional<uint16_t>(0x0000));
}

TEST(BootServiceRealDump, ExternalMediaIsDeviceSpecificAndUnresolved)
{
    EXPECT_FALSE(resolve(Sources::ExternalMedia, Modes::Regular).has_value());
}

TEST(BootServiceRealDump, DefaultIsNoOverride)
{
    EXPECT_FALSE(resolve(Sources::Default, Modes::Regular).has_value());
}
