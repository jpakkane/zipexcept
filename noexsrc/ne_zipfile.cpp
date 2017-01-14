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

#include"ne_zipfile.h"
#include"ne_utils.h"
#include"ne_fileutils.h"
#include"ne_mmapper.h"
#include"ne_naturalorder.h"
#include<ne_portable_endian.h>
#ifdef _WIN32
#include<winsock2.h>
#include<windows.h>
#else
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#endif
#include<cstdio>
#include<cerrno>
#include<cstring>
#include<stdexcept>
#include<future>
#include<algorithm>
#include"ne_decompress.h"

#ifndef _WIN32
using std::max;
#endif

namespace {

void unpack_zip64_sizes(const std::string &extra_field, uint64_t &compressed_size, uint64_t &uncompressed_size,
        Error **e) {
    size_t offset = 0;
    while(offset < extra_field.size()) {
        uint16_t header_id = le16toh(*reinterpret_cast<const uint16_t*>(&extra_field[offset]));
        offset+=2;
        uint16_t data_size = le16toh(*reinterpret_cast<const uint16_t*>(&extra_field[offset]));
        offset+=2;
        if(header_id == ZIP_EXTRA_ZIP64) {
            uncompressed_size = le64toh(*reinterpret_cast<const uint64_t*>(&extra_field[offset]));
            offset += 8;
            compressed_size = le64toh(*reinterpret_cast<const uint64_t*>(&extra_field[offset]));
            return;
        }
        offset += data_size;
    }
    *e = create_error("Entry extra field did not contain ZIP64 extension, file can not be parsed.");
}

void unpack_unix(const std::string &extra, unixextra &unix) {
    size_t offset = 0;
    while(offset < extra.size()) {
        uint16_t header_id = le16toh(*reinterpret_cast<const uint16_t*>(&extra[offset]));
        offset+=2;
        uint16_t data_size = le16toh(*reinterpret_cast<const uint16_t*>(&extra[offset]));
        offset+=2;
        auto extra_end = offset + data_size;
        if(header_id == ZIP_EXTRA_UNIX) {
            unix.atime = le32toh(*reinterpret_cast<const uint32_t*>(&extra[offset]));
            offset += 4;
            unix.mtime = le32toh(*reinterpret_cast<const uint32_t*>(&extra[offset]));
            offset += 4;
            unix.uid = le16toh(*reinterpret_cast<const uint16_t*>(&extra[offset]));
            offset += 2;
            unix.gid = le16toh(*reinterpret_cast<const uint16_t*>(&extra[offset]));
            offset += 2;
            unix.data = std::string(&extra[offset], &extra[extra_end]);
            return;
        }
        offset += data_size;
    }
    unix.atime = 0;
}

void check_filename(const std::string &fname, Error **e) {
    if(fname.size() == 0) {
        *e = create_error("Empty filename in directory");
    }
    if(is_absolute_path(fname)) {
        *e = create_error("Archive has an absolute filename which is forbidden.");
    }
}

localheader read_local_entry(File &f, Error **e) {
    localheader h;
    uint16_t fname_length, extra_length;
    h.needed_version = f.read16le(e);
    if(*e) {
        return h;
    }
    h.gp_bitflag = f.read16le(e);
    if(*e) {
        return h;
    }
    h.compression = f.read16le(e);
    if(*e) {
        return h;
    }
    h.last_mod_time = f.read16le(e);
    if(*e) {
        return h;
    }
    h.last_mod_date = f.read16le(e);
    if(*e) {
        return h;
    }
    h.crc32 = f.read32le(e);
    if(*e) {
        return h;
    }
    h.compressed_size = f.read32le(e);
    if(*e) {
        return h;
    }
    h.uncompressed_size = f.read32le(e);
    if(*e) {
        return h;
    }
    fname_length = f.read16le(e);
    if(*e) {
        return h;
    }
    extra_length = f.read16le(e);
    if(*e) {
        return h;
    }
    h.fname = f.read(fname_length, e);
    if(*e) {
        return h;
    }
    h.extra = f.read(extra_length, e);
    if(*e) {
        return h;
    }
    if(h.compressed_size == 0xFFFFFFFF || h.uncompressed_size == 0xFFFFFFFF) {
        unpack_zip64_sizes(h.extra, h.compressed_size, h.uncompressed_size, e);
        if(*e) {
            return h;
        }
    }
    unpack_unix(h.extra, h.unix);
    check_filename(h.fname, e);
    return h;
}

centralheader read_central_entry(File &f, Error **e) {
    centralheader c;
    uint16_t fname_length, extra_length, comment_length;
    c.version_made_by = f.read16le(e);
    if(*e) {
        return c;
    }
    c.version_needed = f.read16le(e);
    if(*e) {
        return c;
    }
    c.bit_flag = f.read16le(e);
    if(*e) {
        return c;
    }
    c.compression_method = f.read16le(e);
    if(*e) {
        return c;
    }
    c.last_mod_time = f.read16le(e);
    if(*e) {
        return c;
    }
    c.last_mod_date = f.read16le(e);
    if(*e) {
        return c;
    }
    c.crc32 = f.read32le(e);
    if(*e) {
        return c;
    }
    c.compressed_size = f.read32le(e);
    if(*e) {
        return c;
    }
    c.uncompressed_size = f.read32le(e);
    if(*e) {
        return c;
    }
    fname_length = f.read16le(e);
    if(*e) {
        return c;
    }
    extra_length = f.read16le(e);
    if(*e) {
        return c;
    }
    comment_length = f.read16le(e);
    if(*e) {
        return c;
    }
    c.disk_number_start = f.read16le(e);
    if(*e) {
        return c;
    }
    c.internal_file_attributes = f.read16le(e);
    if(*e) {
        return c;
    }
    c.external_file_attributes = f.read32le(e);
    if(*e) {
        return c;
    }
    c.local_header_rel_offset = f.read32le(e);
    if(*e) {
        return c;
    }

    c.fname = f.read(fname_length, e);
    if(*e) {
        return c;
    }
    c.extra_field = f.read(extra_length, e);
    if(*e) {
        return c;
    }
    c.comment = f.read(comment_length, e);
    return c;
}

zip64endrecord read_z64_central_end(File &f, Error **e) {
    zip64endrecord er;
    er.recordsize = f.read64le(e);
    if(*e) {
        return er;
    }
    er.version_made_by = f.read16le(e);
    if(*e) {
        return er;
    }
    er.version_needed = f.read16le(e);
    if(*e) {
        return er;
    }
    er.disk_number = f.read32le(e);
    if(*e) {
        return er;
    }
    er.dir_start_disk_number = f.read32le(e);
    if(*e) {
        return er;
    }
    er.this_disk_num_entries = f.read64le(e);
    if(*e) {
        return er;
    }
    er.total_entries = f.read64le(e);
    if(*e) {
        return er;
    }
    er.dir_size = f.read64le(e);
    if(*e) {
        return er;
    }
    er.dir_offset = f.read64le(e);
    if(*e) {
        return er;
    }
    auto ext_size = er.recordsize - 2 - 2 - 4 - 4 - 8 - 8 - 8 - 8;
    er.extensible = f.read(ext_size, e);
    return er;
}

zip64locator read_z64_locator(File &f, Error **e) {
    zip64locator loc;
    loc.central_dir_disk_number = f.read32le(e);
    if(*e) {
        return loc;
    }
    loc.central_dir_offset = f.read64le(e);
    if(*e) {
        return loc;
    }
    loc.num_disks = f.read32le(e);
    return loc;
}

endrecord read_end_record(File &f, Error **e) {
    endrecord el;
    el.disk_number = f.read16le(e);
    if(*e) {
        return el;
    }
    el.central_dir_disk_number = f.read16le(e);
    if(*e) {
        return el;
    }
    el.this_disk_num_entries = f.read16le(e);
    if(*e) {
        return el;
    }
    el.total_entries = f.read16le(e);
    if(*e) {
        return el;
    }
    el.dir_size = f.read32le(e);
    if(*e) {
        return el;
    }
    el.dir_offset_start_disk = f.read32le(e);
    if(*e) {
        return el;
    }
    auto csize = f.read16le(e);
    if(*e) {
        return el;
    }
    el.comment = f.read(csize, e);
    return el;
}

}

ZipFile::ZipFile() {
}

void ZipFile::initialize(const char *fname, Error **e) {
    zipfile.initialize(fname, "rb", e);
    if(*e) {
        return;
    }
    readLocalFileHeaders(e);
    if(*e) {
        return;
    }
    readCentralDirectory(e);
    if(*e) {
        return;
    }
    if(entries.size() != centrals.size()) {
        std::string msg("Mismatch. File has ");
        msg += std::to_string(entries.size());
        msg += " local entries but ";
        msg += std::to_string(centrals.size());
        msg += " central entries.";
        *e = create_error(msg.c_str());
        return;
    }
    auto id = zipfile.read32le(e);
    if(*e) {
        return;
    }
    if(id == ZIP64_CENTRAL_END_SIG) {
        z64end = read_z64_central_end(zipfile, e);
        if(*e) {
            return;
        }
        if(z64end.total_entries != entries.size()) {
            *e = create_error("File is broken, zip64 directory has incorrect number of entries.");
            return;
        }
        id = zipfile.read32le(e);
        if(*e) {
            return;
        }
        if(id == ZIP64_CENTRAL_LOCATOR_SIG) {
            z64loc = read_z64_locator(zipfile, e);
            if(*e) {
                return;
            }
            id = zipfile.read32le(e);
            if(*e) {
                return;
            }
        }
    }
    if(id != CENTRAL_END_SIG) {
        *e = create_error("Zip file broken, missing end of central directory.");
        return;
    }
    endloc = read_end_record(zipfile, e);
    if(*e) {
        return;
    }
    if(endloc.total_entries != 0xFFFF && endloc.total_entries != entries.size()) {
        *e = create_error("Zip file broken, end record has incorrect directory size.");
        return;
    }
    zipfile.seek(0, SEEK_END);
    fsize = zipfile.tell();
}

ZipFile::~ZipFile() {
    if(t) {
        t->join();
    }
}

void ZipFile::readLocalFileHeaders(Error **e) {
    while(true) {
        auto curloc = zipfile.tell();
        uint32_t head = zipfile.read32le(e);
        if(*e) {
            return;
        }
        if(head != LOCAL_SIG) {
            zipfile.seek(curloc);
            break;
        }
        entries.emplace_back(read_local_entry(zipfile, e));
        if(*e) {
            return;
        }
        if(entries.back().gp_bitflag & 1) {
            *e = create_error("This file is encrypted. Encrypted ZIP archives are not supported.");
            return;
        }
        data_offsets.push_back(zipfile.tell());
        zipfile.seek(entries.back().compressed_size, SEEK_CUR);
        if(entries.back().gp_bitflag & (1<<2)) {
            zipfile.seek(3*4, SEEK_CUR);
        }
    }
}

void ZipFile::readCentralDirectory(Error **e) {
    while(true) {
        auto curloc = zipfile.tell();
        uint32_t head = zipfile.read32le(e);
        if(*e) {
            return;
        }
        if(head != CENTRAL_SIG) {
            zipfile.seek(curloc);
            break;
        }
        centrals.push_back(read_central_entry(zipfile, e));
        if(*e) {
            return;
        }
    }
}

void ZipFile::unzip(const std::string &prefix, Error **e) const {
    int fd = zipfile.fileno();
    if(fd < 0) {
        *e = create_system_error("Could not open zip file:");
        return;
    }

    MMapper map;
    map.initialise(zipfile, e);
    if(*e) {
        return;
    }

    unsigned char *file_start = map;
    for(size_t i=0; i<entries.size(); i++) {
        auto r = unpack_entry(prefix, entries[i],
                centrals[i],
                file_start + data_offsets[i],
                entries[i].compressed_size, e);
        if(*e) {
            return;
        }
    }
}


