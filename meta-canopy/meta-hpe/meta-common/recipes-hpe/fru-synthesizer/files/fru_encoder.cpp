// SPDX-License-Identifier: Apache-2.0
#include "fru_encoder.hpp"

namespace fru
{

namespace
{

constexpr uint8_t fruVersion = 0x01;
constexpr uint8_t endOfFields = 0xC1;
// Type/length code: type = 0b11 (ASCII + Latin 1), length in the low 6 bits.
constexpr uint8_t typeAscii = 0xC0;
constexpr size_t maxFieldLen = 63;
constexpr size_t areaAlign = 8;

void appendField(std::vector<uint8_t>& out, const std::optional<std::string>& v)
{
    if (!v || v->empty())
    {
        // Zero-length ASCII field.
        out.push_back(typeAscii);
        return;
    }
    std::string s = *v;
    if (s.size() > maxFieldLen)
    {
        s.resize(maxFieldLen);
    }
    out.push_back(static_cast<uint8_t>(typeAscii | s.size()));
    out.insert(out.end(), s.begin(), s.end());
}

// Two's-complement "zero" checksum over [begin, end): the byte that makes the
// running sum of all bytes (including the checksum) equal zero mod 256.
uint8_t zeroChecksum(const std::vector<uint8_t>& data, size_t begin, size_t end)
{
    uint8_t sum = 0;
    for (size_t i = begin; i < end; i++)
    {
        sum = static_cast<uint8_t>(sum + data[i]);
    }
    return static_cast<uint8_t>(0U - sum);
}

// Append the end-of-fields marker, pad to a multiple of 8 (leaving room for the
// checksum), write the area length, and append the checksum.
void finalizeArea(std::vector<uint8_t>& area)
{
    area.push_back(endOfFields);
    while (((area.size() + 1) % areaAlign) != 0)
    {
        area.push_back(0x00);
    }
    // Length field counts 8-byte blocks of the whole area, including checksum.
    area[1] = static_cast<uint8_t>((area.size() + 1) / areaAlign);
    area.push_back(zeroChecksum(area, 0, area.size()));
}

std::vector<uint8_t> buildChassisArea(const ChassisInfo& c)
{
    std::vector<uint8_t> a;
    a.push_back(fruVersion);
    a.push_back(0x00);    // length placeholder
    a.push_back(c.type);  // chassis type (SMBIOS enclosure type)
    appendField(a, c.partNumber);
    appendField(a, c.serialNumber);
    for (const auto& f : c.custom)
    {
        appendField(a, f);
    }
    finalizeArea(a);
    return a;
}

std::vector<uint8_t> buildBoardArea(const BoardInfo& b)
{
    std::vector<uint8_t> a;
    a.push_back(fruVersion);
    a.push_back(0x00); // length placeholder
    a.push_back(0x00); // language code: English
    // Mfg date/time, 3 bytes, minutes since 1996-01-01; 0 = unspecified.
    a.push_back(0x00);
    a.push_back(0x00);
    a.push_back(0x00);
    appendField(a, b.manufacturer);
    appendField(a, b.productName);
    appendField(a, b.serialNumber);
    appendField(a, b.partNumber);
    appendField(a, b.fruFileId);
    for (const auto& c : b.custom)
    {
        appendField(a, c);
    }
    finalizeArea(a);
    return a;
}

std::vector<uint8_t> buildProductArea(const ProductInfo& p)
{
    std::vector<uint8_t> a;
    a.push_back(fruVersion);
    a.push_back(0x00); // length placeholder
    a.push_back(0x00); // language code: English
    appendField(a, p.manufacturer);
    appendField(a, p.productName);
    appendField(a, p.partNumber);
    appendField(a, p.version);
    appendField(a, p.serialNumber);
    appendField(a, p.assetTag);
    appendField(a, p.fruFileId);
    for (const auto& c : p.custom)
    {
        appendField(a, c);
    }
    finalizeArea(a);
    return a;
}

} // namespace

std::vector<uint8_t> build(const std::optional<ChassisInfo>& chassis,
                           const std::optional<BoardInfo>& board,
                           const std::optional<ProductInfo>& product)
{
    std::vector<uint8_t> chassisArea;
    std::vector<uint8_t> boardArea;
    std::vector<uint8_t> productArea;
    if (chassis)
    {
        chassisArea = buildChassisArea(*chassis);
    }
    if (board)
    {
        boardArea = buildBoardArea(*board);
    }
    if (product)
    {
        productArea = buildProductArea(*product);
    }

    std::vector<uint8_t> out(areaAlign, 0x00); // common header (8 bytes)
    out[0] = fruVersion;

    size_t blockOffset = 1; // header occupies the first 8-byte block
    if (!chassisArea.empty())
    {
        out[2] = static_cast<uint8_t>(blockOffset);
        blockOffset += chassisArea.size() / areaAlign;
    }
    if (!boardArea.empty())
    {
        out[3] = static_cast<uint8_t>(blockOffset);
        blockOffset += boardArea.size() / areaAlign;
    }
    if (!productArea.empty())
    {
        out[4] = static_cast<uint8_t>(blockOffset);
        blockOffset += productArea.size() / areaAlign;
    }
    out[7] = zeroChecksum(out, 0, 7);

    out.insert(out.end(), chassisArea.begin(), chassisArea.end());
    out.insert(out.end(), boardArea.begin(), boardArea.end());
    out.insert(out.end(), productArea.begin(), productArea.end());
    return out;
}

} // namespace fru
