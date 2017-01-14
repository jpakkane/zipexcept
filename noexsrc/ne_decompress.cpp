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


/* Zlib decompression is based on:
 *
 *  zpipe.c: example of proper use of zlib's inflate() and deflate()
 *  Not copyrighted -- provided to the public domain
 *  Version 1.4  11 December 2005  Mark Adler */

#include "ne_decompress.h"

#include"ne_zipdefs.h"
#include"ne_utils.h"
#include"ne_fileutils.h"
#include"ne_file.h"

#include"ne_portable_endian.h"
#include<zlib.h>

#ifdef _WIN32
#include<windows.h>
#else
#include<lzma.h> // Disabled on Windows because libxz does not compile with MSVC.
#include<unistd.h>
#include<utime.h>
#include<sys/stat.h>
#endif
#include<algorithm>
#include<cstdint>

#if defined(__linux__)
#include<sys/sysmacros.h>
#endif

#include <cstring>
#include <cassert>
#include<cstdio>
#include<cstdlib>

#include<memory>

#define CHUNK 1024*1024

namespace {

uint32_t inflate_to_file(const unsigned char *data_start, uint64_t data_size, FILE *ofile, Error **e);
uint32_t lzma_to_file(const unsigned char *data_start, uint64_t data_size, FILE *ofile, Error **e);
uint32_t unstore_to_file(const unsigned char *data_start, uint64_t data_size, FILE *ofile, Error **e);

/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
uint32_t inflate_to_file(const unsigned char *data_start,
                         uint64_t data_size,
                         FILE *ofile,
                         Error **e) {
    uint32_t crcvalue = crc32(0, Z_NULL, 0);
    int ret;
    unsigned have;
    z_stream strm;
    const unsigned char *current = data_start;
    std::unique_ptr<unsigned char[]> out(new unsigned char [CHUNK]);

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit2(&strm, -15);
    if (ret != Z_OK) {
        *e = create_error("Could not init zlib.");
        return 0;
    }
    std::unique_ptr<z_stream, int (*)(z_stream_s*)> zcloser(&strm, inflateEnd);

    /* decompress until deflate stream ends or end of file */
    strm.avail_in = data_size;
    strm.next_in = const_cast<unsigned char*>(current); // zlib header is const-broken
    do {
        if(strm.total_in >= data_size) {
            break;
        }

        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out.get();
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                *e = create_error(strm.msg);
                return 0;
            }
            have = CHUNK - strm.avail_out;
            crcvalue = crc32(crcvalue, out.get(), have);
            if (fwrite(out.get(), 1, have, ofile) != have || ferror(ofile)) {
                *e = create_system_error("Could not write to file:");
                return 0;
            }
        } while (strm.avail_out == 0);
        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);
    return crcvalue;
}

#ifdef _WIN32
uint32_t lzma_to_file(const unsigned char *data_start, uint64_t data_size, FILE *ofile, Error **e) {
    *e = create_error("LZMA not supported on Windows.");
}

#else
uint32_t lzma_to_file(const unsigned char *data_start,
                      uint64_t data_size,
                      FILE *ofile,
                      Error **e) {
    uint32_t crcvalue = crc32(0, Z_NULL, 0);
    std::unique_ptr<unsigned char[]> out(new unsigned char [CHUNK]);
    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_filter filter[2];
    unsigned int have;

    size_t offset = 2;
    uint16_t properties_size = le16toh(*reinterpret_cast<const uint16_t*>(data_start + offset));
    offset+=2;
    filter[0].id = LZMA_FILTER_LZMA1;
    filter[1].id = LZMA_VLI_UNKNOWN;
    lzma_ret ret = lzma_properties_decode(&filter[0], nullptr, data_start + offset, properties_size);
    offset += properties_size;
    if(ret != LZMA_OK) {
        *e = create_error("Could not decode LZMA properties.");
        return 0;
    }
    ret = lzma_raw_decoder(&strm, &filter[0]);
    free(filter[0].options);
    if(ret != LZMA_OK) {
        *e = create_error("Could not initialize LZMA decoder.");
        return 0;
    }
    std::unique_ptr<lzma_stream, void(*)(lzma_stream*)> lcloser(&strm, lzma_end);

    const unsigned char *current = data_start + offset;
    strm.avail_in = (size_t)(data_size - offset);
    strm.next_in = current;
    /* decompress until data ends */
    do {
        if (strm.total_in == data_size - offset)
            break;

        do {
            strm.avail_out = CHUNK;
            strm.next_out = out.get();
            ret = lzma_code(&strm, LZMA_RUN);
            if(ret != LZMA_OK && ret != LZMA_STREAM_END) {
                *e = create_error("Decompression failed.");
                return 0;
            }
            have = CHUNK - strm.avail_out;
            crcvalue = crc32(crcvalue, out.get(), have);
            if (fwrite(out.get(), 1, have, ofile) != have || ferror(ofile)) {
                *e = create_error("Could not write to file:");
                return 0;
            }
        } while (strm.avail_out == 0);
    } while (true);
    return crcvalue;
}
#endif

uint32_t unstore_to_file(const unsigned char *data_start,
                         uint64_t data_size,
                         FILE *ofile,
                         Error **e) {
    auto bytes_written = fwrite(data_start, 1, data_size, ofile);
    if(bytes_written != data_size) {
        *e = create_system_error("Could not write file fully:");
        return 0;
    }
    return CRC32(data_start, data_size);
}

void create_symlink(const unsigned char *data_start, uint64_t data_size, const std::string &outname, Error **e) {
#ifndef _WIN32
    std::string symlink_target(data_start, data_start + data_size);
    printf("Symlinking.\n");
    if(symlink(symlink_target.c_str(), outname.c_str()) != 0) {
        printf("Not work.\n");
        *e = create_system_error("Symlink creation failed:");
        return;
    }
#endif
}

void create_file(const localheader &lh,
                 const centralheader &ch,
                 const unsigned char *data_start,
                 uint64_t data_size,
                 const std::string &outname,
                 Error **e) {
    decltype(unstore_to_file) *f;
    if(ch.compression_method == ZIP_NO_COMPRESSION) {
        f = unstore_to_file;
    } else if(ch.compression_method == ZIP_DEFLATE) {
        f = inflate_to_file;
    } else if(ch.compression_method == ZIP_LZMA) {
        f = lzma_to_file;
    } else {
        *e = create_error("Unsupported compression format.");
        return;
    }
    if(exists_on_fs(outname)) {
        *e = create_error("Already exists, will not overwrite.");
        return;
    }
    create_dirs_for_file(outname, e);
    if(*e) {
        return;
    }
    std::string extraction_name = outname + "$ZIPTMP";
    File ofile;
    ofile.initialize(extraction_name.c_str(), "w+b", e);
    if(*e) {
        return;
    }
    uint32_t crc32;
    crc32 = (*f)(data_start, data_size, ofile.get(), e);
    if(*e) {
        unlink(extraction_name.c_str());
        return;
    }

    uint32_t original = lh.gp_bitflag&(1<<2) ? ch.crc32 : lh.crc32;
    if(crc32 != original) {
        unlink(extraction_name.c_str());
        *e = create_error("CRC32 checksum is invalid.");
        return;
    }
    ofile.close();
    if(rename(extraction_name.c_str(), outname.c_str()) != 0) {
        unlink(extraction_name.c_str());
        *e = create_error("Could not rename tmp file to target file:");
        return;
    }
}

void create_device(const localheader &lh, const std::string &outname, Error **e) {
#ifdef _WIN32
  // Windows does not have character devices.
#else
    const std::string &d = lh.unix.data;
    if(d.size() != 8) {
        std::string msg("Incorrect extra data for character device, expected 8, got ");
        msg += std::to_string(d.size());
        msg += ".";
        *e = create_error(msg.c_str());
        return;
    }
    create_dirs_for_file(outname, e);
    if(*e) {
        return;
    }
    uint32_t major_id = le32toh(*reinterpret_cast<const uint32_t*>(&d[0]));
    uint32_t minor_id = le32toh(*reinterpret_cast<const uint32_t*>(&d[4]));
    if(mknod(outname.c_str(), S_IFCHR, makedev(major_id, minor_id)) != 0) {
        std::string msg("Could not create device node, major ");
        msg += std::to_string(major_id);
        msg += " minor ";
        msg += std::to_string(minor_id);
        msg += ": ";
        *e = create_system_error(msg.c_str());
    }
#endif
}

filetype detect_filetype(const localheader &lh, const centralheader &ch, Error **e) {
#ifndef _WIN32
    if(ch.version_made_by>>8 == MADE_BY_UNIX) {
        uint16_t extattrs = ch.external_file_attributes >> 16;
        if(S_ISDIR(extattrs)) {
            return DIRECTORY_ENTRY;
        } else if(S_ISLNK(extattrs)) {
            if(ch.compression_method != ZIP_NO_COMPRESSION) {
                *e = create_error("Symbolic link stored compressed. Not supported.");
            }
            return SYMLINK_ENTRY;
        } else if(S_ISCHR(extattrs)) {
            return CHARDEV_ENTRY;
        } else if(S_ISREG(extattrs)) {
            return FILE_ENTRY;
        } else {
            return UNKNOWN_ENTRY;
        }
    }
#endif
    // Nothing much to do.
    if(lh.fname.back() == '/') {
        return DIRECTORY_ENTRY;
    }
    return FILE_ENTRY;
}

filetype do_unpack(const localheader &lh,
               const centralheader &ch,
               const unsigned char *data_start,
               uint64_t data_size,
               const std::string &outname,
               Error **e) {
    auto ftype = detect_filetype(lh, ch, e);
    if(*e) {
        return ftype;
    }
    switch(ftype) {
    case DIRECTORY_ENTRY : mkdirp(outname, e); break;
    case SYMLINK_ENTRY : create_symlink(data_start, data_size, outname, e); break;
    case CHARDEV_ENTRY : create_device(lh, outname, e); break;
    case FILE_ENTRY : create_file(lh, ch, data_start, data_size, outname, e); break;
    default : *e = create_error("Unknown file type.");
    }
    return ftype;
}

void set_unix_permissions(const localheader &lh, const centralheader &ch, const std::string &fname,
        Error **e) {
#ifndef _WIN32
    // This part of the zip spec is poorly documented. :(
    // https://trac.edgewall.org/attachment/ticket/8919/ZipDownload.patch
    if(chmod(fname.c_str(), (ch.external_file_attributes >> 16)&0777) != 0) {
        *e = create_system_error("Could not change ownership: ");
        return;
    }
    // Only support mtime if it is in zip64 info.
    // FIXME add support for crazy zip dos format.
    // http://mindprod.com/jgloss/zip.html
    if(lh.unix.atime != 0) {
        // These can fail for various reasons (i.e. no chown privilegde), so ignore return values.
        struct utimbuf tb;
        tb.actime = lh.unix.atime;
        tb.modtime = lh.unix.mtime;
        utime(fname.c_str(), &tb);
    }
    chown(fname.c_str(), lh.unix.uid, lh.unix.gid);
#endif
}

}

UnpackResult unpack_entry(const std::string &prefix, const localheader &lh,
        const centralheader &ch,
        const unsigned char *data_start,
        uint64_t data_size,
        Error **e) {
    std::string ofname;
    if(prefix.empty()) {
        ofname = lh.fname;
    } else {
        if(prefix.back() != '/') {
            ofname = prefix + '/' + lh.fname;
        } else {
            ofname = prefix + lh.fname;
        }
    }
    auto ftype = do_unpack(lh, ch, data_start, data_size, ofname, e);
    if(*e) {
        return UnpackResult{false, "FAIL: " + lh.fname};
    }
    if(ch.version_made_by>>8 == MADE_BY_UNIX && ftype != SYMLINK_ENTRY) {
        set_unix_permissions(lh, ch, ofname, e);
        if(*e) {
            return UnpackResult{false, "FAIL: " + lh.fname};
        }
    }
    return UnpackResult{true, "OK: " + lh.fname};
}
