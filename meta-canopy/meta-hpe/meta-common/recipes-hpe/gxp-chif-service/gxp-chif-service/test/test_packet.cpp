// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH

#include "../src/packet.hpp"
#include "mock_channel.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstring>

using namespace chif;

TEST(PacketTest, HeaderSize)
{
    EXPECT_EQ(sizeof(ChifPktHeader), 8u);
}

TEST(PacketTest, ParseHeader)
{
    // Construct a raw header: pktSize=0x0010, seq=0x0042, cmd=0x0003,
    // service=0x02, version=0x01
    std::array<uint8_t, 8> raw = {
        0x10, 0x00, // pktSize = 16
        0x42, 0x00, // sequence = 0x42
        0x03, 0x00, // command = 0x03
        0x02,       // serviceId = 0x02 (ROM)
        0x01,       // version = 1
    };

    auto hdr = parseHeader(raw);
    EXPECT_EQ(hdr.pktSize, 0x0010);
    EXPECT_EQ(hdr.sequence, 0x0042);
    EXPECT_EQ(hdr.command, 0x0003);
    EXPECT_EQ(hdr.serviceId, 0x02);
    EXPECT_EQ(hdr.version, 0x01);
}

TEST(PacketTest, ResponseCommandBit)
{
    // Response command should be request | 0x8000
    EXPECT_EQ(uint16_t(0x0003 | responseBit), 0x8003);
    EXPECT_EQ(uint16_t(0x0072 | responseBit), 0x8072);
    EXPECT_EQ(uint16_t(0x0002 | responseBit), 0x8002);
}

TEST(PacketTest, InitResponse)
{
    ChifPktHeader reqHdr{};
    reqHdr.pktSize = 8;
    reqHdr.sequence = 0x1234;
    reqHdr.command = 0x0003;
    reqHdr.serviceId = 0x02;
    reqHdr.version = 0x01;

    std::array<uint8_t, 16> resp{};
    initResponse(resp, reqHdr, 12);

    auto rspHdr = parseHeader(resp);
    EXPECT_EQ(rspHdr.pktSize, 12);
    EXPECT_EQ(rspHdr.sequence, 0x1234);
    EXPECT_EQ(rspHdr.command, 0x8003);
    EXPECT_EQ(rspHdr.serviceId, 0x02);
    EXPECT_EQ(rspHdr.version, 0x01);
}

TEST(PacketTest, PayloadExtraction)
{
    std::array<uint8_t, 12> pkt{};
    // Fill header
    ChifPktHeader hdr{};
    hdr.pktSize = 12;
    std::memcpy(pkt.data(), &hdr, sizeof(hdr));
    // Fill payload with known bytes
    pkt[8] = 0xAA;
    pkt[9] = 0xBB;
    pkt[10] = 0xCC;
    pkt[11] = 0xDD;

    auto p = payload(std::span<const uint8_t>(pkt));
    ASSERT_EQ(p.size(), 4u);
    EXPECT_EQ(p[0], 0xAA);
    EXPECT_EQ(p[1], 0xBB);
    EXPECT_EQ(p[2], 0xCC);
    EXPECT_EQ(p[3], 0xDD);
}

TEST(PacketTest, EmptyPayload)
{
    // A header-only packet has no payload
    std::array<uint8_t, 8> pkt{};
    auto p = payload(std::span<const uint8_t>(pkt));
    EXPECT_TRUE(p.empty());
}

TEST(PacketTest, MockChannelRoundtrip)
{
    test::MockChannel ch;

    // Enqueue a packet
    std::array<uint8_t, 4> data = {0x01, 0x02, 0x03, 0x04};
    ch.enqueuePacket(0x0003, 0x02, 1, data);

    // Read it back
    std::array<uint8_t, maxPacketSize> buf{};
    auto n = ch.read(buf);
    ASSERT_EQ(n, static_cast<ssize_t>(sizeof(ChifPktHeader) + 4));

    auto hdr = parseHeader(std::span<const uint8_t>(buf.data(), n));
    EXPECT_EQ(hdr.command, 0x0003);
    EXPECT_EQ(hdr.serviceId, 0x02);

    // Write a response
    std::array<uint8_t, 12> resp{};
    initResponse(resp, hdr, 12);
    ch.write(resp);

    EXPECT_EQ(ch.writtenPackets().size(), 1u);
    auto rspHdr = ch.lastResponseHeader();
    EXPECT_EQ(rspHdr.command, 0x8003);
}
