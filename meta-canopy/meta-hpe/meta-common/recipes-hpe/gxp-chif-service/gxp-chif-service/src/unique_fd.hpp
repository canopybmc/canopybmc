// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2026 9elements GmbH
#pragma once

#include <unistd.h>

namespace chif
{

// RAII wrapper for file descriptors — ensures close() on scope exit.
struct UniqueFd
{
    int fd = -1;
    explicit UniqueFd(int f) : fd(f) {}
    ~UniqueFd()
    {
        if (fd >= 0)
        {
            close(fd);
        }
    }
    UniqueFd(const UniqueFd&) = delete;
    UniqueFd& operator=(const UniqueFd&) = delete;
    UniqueFd(UniqueFd&& other) noexcept : fd(other.fd) { other.fd = -1; }
    UniqueFd& operator=(UniqueFd&& other) noexcept
    {
        if (this != &other)
        {
            if (fd >= 0)
            {
                close(fd);
            }
            fd = other.fd;
            other.fd = -1;
        }
        return *this;
    }
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator int() const { return fd; }
};

} // namespace chif
