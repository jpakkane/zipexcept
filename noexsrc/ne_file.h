/*
 * Copyright (C) 2016 Jussi Pakkanen.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of version 3, or (at your option) any later version,
 * of the GNU General Public License as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include<memory>

#include<cstdio>
#include<string>
#include"ne_utils.h"

class MMapper;
struct Error;

class File final {
private:

    FILE *f;
    void read(void *buf, size_t bufsize, Error **e);

public:

    File() : f(nullptr) {};
    File(const File &) = delete;
    File(File &&other) { f = other.f; other.f = nullptr; }
    File& operator=(File &&other) { f = other.f; other.f = nullptr; return *this; }
    ~File();


    void initialize(const std::string &fname, const char *mode, Error **e);
    void initialize(FILE *opened, Error **e);

    operator FILE*() { return f; }

    FILE* get() const { return f; }
    int64_t tell() const;
    int seek(int64_t offset, int whence=SEEK_SET);
    int fileno() const;

    std::unique_ptr<MMapper> mmap(Error **e) const;

    uint64_t size(Error **e) const;
    void flush(Error **e);
    void close();

    uint8_t read8(Error **e);
    uint16_t read16le(Error **e);
    uint32_t read32le(Error **e);
    uint64_t read64le(Error **e);
    uint16_t read16be(Error **e);
    uint32_t read32be(Error **e);
    uint64_t read64be(Error **e);
    std::string read(size_t bufsize, Error **e);

    void write8(uint8_t i, Error **e);
    void write16le(uint16_t i, Error **e);
    void write32le(uint32_t i, Error **e);
    void write64le(uint64_t i, Error **e);
    void write16be(uint16_t i, Error **e);
    void write32be(uint32_t i, Error **e);
    void write64be(uint64_t, Error **e);
    void write(const std::string &s, Error **e);
    void write(const unsigned char *s, uint64_t size, Error **e);
};
