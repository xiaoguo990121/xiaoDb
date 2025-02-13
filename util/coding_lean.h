#pragma once

#include <cstdint>
#include <cstring>

#include "port/port.h"

namespace XIAODB_NAMESPACE
{
    inline void EncodeFixed16(char *buf, uint16_t value)
    {
        if (port::kLittleEndian)
        {
            memcpy(buf, &value, sizeof(value));
        }
        else
        {
            buf[0] = value & 0xff;
            buf[1] = (value >> 8) & 0xff;
        }
    }

    inline void EncodeFixed32(char *buf, uint32_t value)
    {
        if (port::kLittleEndian)
        {
            memcpy(buf, &value, sizeof(value));
        }
        else
        {
            buf[0] = value & 0xff;
            buf[1] = (value >> 8) & 0xff;
            buf[2] = (value >> 16) & 0xff;
            buf[3] = (value >> 24) & 0xff;
        }
    }

    inline void EncodeFixed64(char *buf, uint64_t value)
    {
        if (port::kLittleEndian)
        {
            memcpy(buf, &value, sizeof(value));
        }
        else
        {
            buf[0] = value & 0xff;
            buf[1] = (value >> 8) & 0xff;
            buf[2] = (value >> 16) & 0xff;
            buf[3] = (value >> 24) & 0xff;
            buf[4] = (value >> 32) & 0xff;
            buf[5] = (value >> 40) & 0xff;
            buf[6] = (value >> 48) & 0xff;
            buf[7] = (value >> 56) & 0xff;
        }
    }

    inline uint16_t DecodeFixed16(const char *ptr)
    {
        if (port::kLittleEndian)
        {
            uint16_t result;
            memcpy(&result, ptr, sizeof(result));
            return result;
        }
        else
        {
            return ((static_cast<uint16_t>(static_cast<unsigned char>(ptr[0]))) |
                    (static_cast<uint16_t>(static_cast<unsigned char>(ptr[1])) << 8));
        }
    }

    inline uint32_t DecodeFixed32(const char *ptr)
    {
        if (port::kLittleEndian)
        {
            // Load the raw bytes
            uint32_t result;
            memcpy(&result, ptr, sizeof(result)); // gcc optimizes this to a plain load
            return result;
        }
        else
        {
            return ((static_cast<uint32_t>(static_cast<unsigned char>(ptr[0]))) |
                    (static_cast<uint32_t>(static_cast<unsigned char>(ptr[1])) << 8) |
                    (static_cast<uint32_t>(static_cast<unsigned char>(ptr[2])) << 16) |
                    (static_cast<uint32_t>(static_cast<unsigned char>(ptr[3])) << 24));
        }
    }

    inline uint64_t DecodeFixed64(const char *ptr)
    {
        if (port::kLittleEndian)
        {
            uint64_t result;
            memcpy(&result, ptr, sizeof(result));
            return result;
        }
        else
        {
            uint64_t lo = DecodeFixed32(ptr);
            uint64_t hi = DecodeFixed32(ptr + 4);
            return (hi << 32) | lo;
        }
    }
}