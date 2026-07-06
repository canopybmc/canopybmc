// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH

#include "../src/bios_config_service.hpp"

#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

using namespace chif;

namespace
{

void appendLe16(std::vector<uint8_t>& out, uint16_t value)
{
    out.push_back(static_cast<uint8_t>(value & 0xff));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xff));
}

// Build a CQSBOOT#### EV payload (evBoot) whose UEFI variable holds `name`
// as a NUL-terminated UTF-16LE string, with `structuredBootStrLen` bytes of
// filler in the structured-boot-string region.
std::vector<uint8_t> makeEvBoot(const std::string& name,
                                uint16_t structuredBootStrLen = 4)
{
    std::vector<uint8_t> data;
    appendLe16(data, 1);                                 // version
    appendLe16(data, 0);                                 // bootOptionIdLen
    data.insert(data.end(), 10, 0);                      // bootOptionData[10]
    appendLe16(data, structuredBootStrLen);              // structuredBootStrLen
    data.insert(data.end(), structuredBootStrLen, 0xAB); // structuredBootStr
    data.insert(data.end(), 6, 0);                       // 6 reserved bytes
    appendLe16(data, static_cast<uint16_t>((name.size() + 1) * 2)); // uefi len
    for (char c : name)
    {
        appendLe16(data, static_cast<uint16_t>(c));
    }
    appendLe16(data, 0); // UTF-16 NUL terminator
    return data;
}

} // namespace

TEST(BiosConfigParse, ExtractsUtf16DisplayName)
{
    auto data = makeEvBoot("UEFI Shell");
    auto name = BiosConfigService::parseBootOptionName(data);
    ASSERT_TRUE(name.has_value());
    EXPECT_EQ(*name, "UEFI Shell");
}

TEST(BiosConfigParse, HandlesVaryingStructuredBootStrLen)
{
    auto data = makeEvBoot("Windows Boot Manager", 32);
    auto name = BiosConfigService::parseBootOptionName(data);
    ASSERT_TRUE(name.has_value());
    EXPECT_EQ(*name, "Windows Boot Manager");
}

TEST(BiosConfigParse, RejectsTooShortPayload)
{
    std::vector<uint8_t> shortData(8, 0);
    EXPECT_FALSE(BiosConfigService::parseBootOptionName(shortData).has_value());
}

TEST(BiosConfigParse, RejectsEmptyName)
{
    auto data = makeEvBoot("");
    EXPECT_FALSE(BiosConfigService::parseBootOptionName(data).has_value());
}

TEST(BiosConfigId, ReturnsSuffixForBootEv)
{
    auto id = BiosConfigService::bootOptionId("CQSBOOT0001");
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(*id, "01");
}

TEST(BiosConfigId, RejectsNonBootEv)
{
    EXPECT_FALSE(BiosConfigService::bootOptionId("CQHBOOTORDER").has_value());
    EXPECT_FALSE(BiosConfigService::bootOptionId("CQSBOOT").has_value());
    EXPECT_FALSE(BiosConfigService::bootOptionId("SOMETHING").has_value());
}
