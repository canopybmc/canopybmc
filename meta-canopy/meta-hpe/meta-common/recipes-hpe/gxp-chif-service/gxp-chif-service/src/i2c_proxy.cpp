// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH

#include "smif_service.hpp"
#include "unique_fd.hpp"

#include <phosphor-logging/lg2.hpp>

#include <cstring>

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>

namespace chif
{

// Static segment-to-Linux-bus mapping for HPE DL360 Gen11.
// Reverse-engineered from CHIF packet capture: the BIOS segment number
// in the engine field maps to a Linux I2C bus adapter. The BIOS serial
// output prints segments in hex (e.g., "Bus 33" means segment 0x33).
static uint8_t segmentToBusLookup(uint8_t segment)
{
    switch (segment)
    {
        // UBM controller groups (segments 0x33-0x36 -> i2c-12..15)
        // These are on i2c-6-mux channels 14,13,12,11
        case 0x33:
            return 12;
        case 0x34:
            return 13;
        case 0x35:
            return 14;
        case 0x36:
            return 15;

        // UBM EEPROM groups (segments 0x57-0x5A -> i2c-16..19)
        // These are on i2c-4-mux channels 4,3 and i2c-3-mux channels 16,15
        case 0x57:
            return 16;
        case 0x58:
            return 17;
        case 0x59:
            return 18;
        case 0x5A:
            return 19;

        // PSU (segment 0x66=PSU1 on i2c-11, 0x67=PSU2 on i2c-10)
        case 0x66:
            return 11;
        case 0x67:
            return 10;

        // PSU FRU EEPROM (segment 0x64)
        // PSU1 EEPROM at i2c-11 addr 0x50, PSU2 at i2c-10 addr 0x51
        // The BIOS selects which PSU via the address field
        case 0x64:
            return 11; // default to PSU1 bus; address distinguishes PSU

        // Megacell/Energy Pack (segment 0x15 -> bus 15 addr 0x60)
        case 0x15:
            return 15;

        // OCP Retimer Card CPLD (segments 0x14, 0x1D)
        // These map to GXP I2C engines for PCIe slot backplanes.
        // The retimer CPLD may not be populated in all configurations.
        case 0x14:
            return 8; // GXP engine 8 — PCIe slot 1
        case 0x1D:
            return 5; // GXP engine 5 — PCIe slot 2

        // Additional segments from diagnostic capture
        case 0x11:
            return 1; // GXP engine 1
        case 0x00:
            return 0; // GXP engine 0

        default:
            return 0xFF; // unmapped
    }
}

// Perform a real I2C transaction via ioctl(I2C_RDWR).
// Returns I2C error code (0=success, 108=segment not found, 7=general error).
static uint32_t doI2cTransaction(uint8_t linuxBus, uint8_t addr7bit,
                                 const uint8_t* writeData, uint8_t writeLen,
                                 uint8_t* readData, uint8_t readLen)
{
    char devPath[32];
    snprintf(devPath, sizeof(devPath), "/dev/i2c-%u", linuxBus);

    UniqueFd fd(open(devPath, O_RDWR));
    if (fd < 0)
    {
        return i2cSegmentDoesNotExist;
    }

    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data rdwr{};
    int nmsgs = 0;

    if (writeLen > 0)
    {
        msgs[nmsgs].addr = addr7bit;
        msgs[nmsgs].flags = 0; // write
        msgs[nmsgs].len = writeLen;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        msgs[nmsgs].buf = const_cast<uint8_t*>(writeData);
        nmsgs++;
    }

    if (readLen > 0)
    {
        msgs[nmsgs].addr = addr7bit;
        msgs[nmsgs].flags = I2C_M_RD;
        msgs[nmsgs].len = readLen;
        msgs[nmsgs].buf = readData;
        nmsgs++;
    }

    rdwr.msgs = msgs;
    rdwr.nmsgs = static_cast<uint32_t>(nmsgs);

    uint32_t errorCode = i2cSuccess;
    if (nmsgs > 0)
    {
        if (ioctl(fd, I2C_RDWR, &rdwr) < 0)
        {
            errorCode = 7; // I2C_GENERAL_ERROR
        }
    }

    return errorCode;
}

int SmifService::handleI2cTransaction(const ChifPktHeader& reqHdr,
                                      std::span<const uint8_t> reqPayload,
                                      std::span<uint8_t> response)
{
    // Phase 2: real I2C transactions with segment-to-bus mapping.
    // Request payload: reserved(4) + magic(8) + address(2) + engine(1)
    //                  + write_len(1) + read_len(1) + write_data(32)
    // Response: must be exactly 57 bytes (header + 49 byte payload).

    if (response.size() < i2cResponseSize)
    {
        return -1;
    }

    initResponse(response, reqHdr, i2cResponseSize);
    auto resp = responsePayload(response);
    std::memset(resp.data(), 0, i2cPayloadSize);

    // Parse request fields
    uint16_t address = 0;
    uint8_t engine = 0;
    uint8_t writeLen = 0;
    uint8_t readLen = 0;
    const uint8_t* writeData = nullptr;

    if (reqPayload.size() >= 17)
    {
        std::memcpy(&address, reqPayload.data() + 12, sizeof(address));
        engine = reqPayload[14];
        writeLen = reqPayload[15];
        readLen = reqPayload[16];
        if (reqPayload.size() >= 17u + writeLen)
        {
            writeData = reqPayload.data() + 17;
        }
    }

    // Echo address and engine in response
    std::memcpy(resp.data() + 12, &address, sizeof(address));
    resp[14] = engine;

    // Sentinel check
    if (address == 0xFFFF && engine == 0xFF)
    {
        uint32_t ec = i2cBadArgument;
        std::memcpy(resp.data(), &ec, sizeof(ec));
        return i2cResponseSize;
    }

    // Clamp lengths
    if (writeLen > 32)
        writeLen = 32;
    if (readLen > 32)
        readLen = 32;

    // Ensure writeData is valid when writeLen > 0
    if (writeLen > 0 && writeData == nullptr)
    {
        writeLen = 0;
    }

    // Look up Linux bus from BIOS segment
    uint8_t linuxBus = segmentToBusLookup(engine);
    if (linuxBus == 0xFF)
    {
        uint32_t ec = i2cSegmentDoesNotExist;
        std::memcpy(resp.data(), &ec, sizeof(ec));
        return i2cResponseSize;
    }

    // Convert 8-bit BIOS address to 7-bit Linux address
    uint8_t addr7bit = static_cast<uint8_t>(address >> 1);

    // Check if this address is owned by a kernel driver. On buses 12-19
    // (UBM), addr 0x40 (ubm4) and 0x57 (24c02) are driver-bound.
    // ioctl(I2C_RDWR) fails with EBUSY for these. Return success with
    // zeroed read data — our kernel already manages these devices.
    if (linuxBus >= 12 && linuxBus <= 19 &&
        (addr7bit == 0x40 || addr7bit == 0x57 || addr7bit == 0x72))
    {
        // Success — device is managed by our kernel drivers
        uint32_t ec = i2cSuccess;
        std::memcpy(resp.data(), &ec, sizeof(ec));
        resp[16] = readLen;
        // Read data is already zeroed from memset
        return i2cResponseSize;
    }

    // Segment 0 (engine=0) maps to i2c-0 which has no useful devices
    // for the BIOS. The Megacell/energy pack doesn't exist in our system.
    // Return NO_DEV (105) to signal no device at this address.
    if (engine == 0)
    {
        constexpr uint32_t i2cNoDev = 105;
        std::memcpy(resp.data(), &i2cNoDev, sizeof(i2cNoDev));
        return i2cResponseSize;
    }

    // Perform the I2C transaction
    uint8_t readBuf[32] = {};
    uint32_t errorCode = doI2cTransaction(
        linuxBus, addr7bit, writeData, writeLen, readBuf, readLen);

    // Fill response
    std::memcpy(resp.data(), &errorCode, sizeof(errorCode));
    resp[16] = readLen;
    if (errorCode == i2cSuccess && readLen > 0)
    {
        std::memcpy(resp.data() + 17, readBuf, readLen);
    }

    return i2cResponseSize;
}

} // namespace chif
