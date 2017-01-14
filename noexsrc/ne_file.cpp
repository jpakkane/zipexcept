/*
 * Copyright (C) 2016-2017 Jussi Pakkanen.
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

#include"ne_file.h"
#include"ne_utils.h"
#include"ne_mmapper.h"
#include<ne_portable_endian.h>
#include<sys/stat.h>


File::~File() {
    if(f) {
        fclose(f);
    }
}

void File::initialize(const std::string &fname, const char *mode, Error **e) {
    if(f) {
        fclose(f);
    }
    f = fopen(fname.c_str(), mode);
    if(!f) {
        std::string msg("Could not open file ");
        msg += fname;
        msg += ":";
        *e = create_system_error(msg.c_str());
    }

}

void File::close() {
    if (f) {
        fclose(f);
        f = nullptr;
    }
}

int64_t File::tell() const {
#ifdef _WIN32
    return _ftelli64(f);
#else
    return ftell(f);
#endif
}
int File::seek(int64_t offset, int whence) {
#ifdef _WIN32
    return _fseeki64(f, offset, whence);
#else
    return fseek(f, offset, whence);
#endif
}

int File::fileno() const {
    return ::fileno(f);
}

std::unique_ptr<MMapper> File::mmap(Error **e) const {
    MMapper *m = new MMapper();
    m->initialise(*this, e);
    if(*e) {
        delete m;
        return nullptr;
    }
    return std::unique_ptr<MMapper>(m);
}

void File::read(void *buf, size_t bufsize, Error **e) {
    if(fread(buf, 1, bufsize, f) != bufsize) {
        *e = create_system_error("Could not read data:");
    }
}

uint8_t File::read8(Error **e) {
    uint8_t r;
    read(&r, sizeof(r), e);
    return r;
}

uint16_t File::read16le(Error **e) {
    uint16_t r;
    read(&r, sizeof(r), e);
    return le16toh(r);

}

uint32_t File::read32le(Error **e) {
    uint32_t r;
    read(&r, sizeof(r), e);
    return le32toh(r);
}

uint64_t File::read64le(Error **e) {
    uint64_t r;
    read(&r, sizeof(r), e);
    return le64toh(r);
}

uint16_t File::read16be(Error **e) {
    uint16_t r;
    read(&r, sizeof(r), e);
    return be16toh(r);

}

uint32_t File::read32be(Error **e) {
    uint32_t r;
    read(&r, sizeof(r), e);
    return be32toh(r);
}

uint64_t File::read64be(Error **e) {
    uint64_t r;
    read(&r, sizeof(r), e);
    return be64toh(r);
}

std::string File::read(size_t bufsize, Error **e) {
    std::string buf(bufsize, 'X');
    read(&buf[0], bufsize, e);
    return buf;
}

uint64_t File::size(Error **e) const {
    struct stat buf;
    if(fstat(fileno(), &buf) != 0) {
        *e = create_system_error("Statting self failed:");
        return -1;
    }
    return buf.st_size;
}

void File::flush(Error **e) {
    if(fflush(f) != 0) {
        *e = create_system_error("Flushing data failed:");
    }
}

void File::write8(uint8_t i, Error **e) {
    write(reinterpret_cast<const unsigned char*>(&i), sizeof(i), e);
}

void File::write16le(uint16_t i, Error **e) {
    uint16_t c = htole16(i);
    write(reinterpret_cast<const unsigned char*>(&c), sizeof(c), e);
}

void File::write32le(uint32_t i, Error **e) {
    uint32_t c = htole32(i);
    write(reinterpret_cast<const unsigned char*>(&c), sizeof(c), e);

}

void File::write64le(uint64_t i, Error **e) {
    uint64_t c = htole64(i);
    write(reinterpret_cast<const unsigned char*>(&c), sizeof(c), e);

}

void File::write16be(uint16_t i, Error **e) {
    uint16_t c = htobe16(i);
    write(reinterpret_cast<const unsigned char*>(&c), sizeof(c), e);

}

void File::write32be(uint32_t i, Error **e) {
    uint32_t c = htobe32(i);
    write(reinterpret_cast<const unsigned char*>(&c), sizeof(c), e);

}

void File::write64be(uint64_t i, Error **e) {
    uint64_t c = htobe64(i);
    write(reinterpret_cast<const unsigned char*>(&c), sizeof(c), e);

}

void File::write(const std::string &s, Error **e) {
    this->write(reinterpret_cast<const unsigned char*>(s.data()), s.size(), e);
}

void File::write(const unsigned char *s, uint64_t size, Error **e) {
    if(fwrite(s, 1, size, f) != size) {
        *e = create_system_error("Could not write data:");
    }
}

