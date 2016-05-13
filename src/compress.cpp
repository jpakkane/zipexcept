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

#include"compress.h"
#include"file.h"
#include"utils.h"
#include"mmapper.h"

#include<memory>
#include<stdexcept>

#include<lzma.h>
#include<cstdio>

#define CHUNK 16384

compressresult compress_entry(const std::string &s) {
    File infile(s, "rb");
    MMapper buf = infile.mmap();
    FILE *f = tmpfile();
    unsigned char out[CHUNK];
    uint32_t filter_size;
    if(!f) {
        throw_system("Could not create temp file: ");
    }
    compressresult result{File(f), CRC32(buf, buf.size())};
    lzma_options_lzma opt_lzma;
    lzma_stream strm = LZMA_STREAM_INIT;
    if(lzma_lzma_preset(&opt_lzma, LZMA_PRESET_DEFAULT)) {
        throw std::runtime_error("Unsupported LZMA preset.");
    }
    lzma_filter filter[2];
    filter[0].id = LZMA_FILTER_LZMA1;
    filter[0].options = &opt_lzma;
    filter[1].id = LZMA_VLI_UNKNOWN;

    lzma_ret ret = lzma_raw_encoder(&strm, filter);
    if(ret != LZMA_OK) {
        throw std::runtime_error("Could not create LZMA encoder.");
    }
    if(lzma_properties_size(&filter_size, filter) != LZMA_OK) {
        throw std::runtime_error("Could not determine LZMA properties size.");
    } else {
        std::string x(filter_size, 'X');
        if(lzma_properties_encode(filter, (unsigned char*)x.data()) != LZMA_OK) {
            throw std::runtime_error("Could not encode filter properties.");
        }
        result.f.write8(9); // This is what Python's lzma lib does. Copy it without understanding.
        result.f.write8(4);
        result.f.write16le(filter_size);
        result.f.write(x);
    }
    std::unique_ptr<lzma_stream, void(*)(lzma_stream*)> lcloser(&strm, lzma_end);

    strm.avail_in = buf.size();
    strm.next_in = buf;
    /* compress until data ends */
    lzma_action action = LZMA_RUN;
    while(true) {
        if(strm.total_in >= buf.size()) {
            action = LZMA_FINISH;
        } else {
            strm.avail_out = CHUNK;
            strm.next_out = out;
        }
        ret = lzma_code(&strm, action);
        if(strm.avail_out == 0 || ret == LZMA_STREAM_END) {
            size_t write_size = CHUNK - strm.avail_out;
            if (fwrite(out, 1, write_size, result.f) != write_size || ferror(result.f)) {
                throw_system("Could not write to file:");
            }
            strm.next_out = out;
            strm.avail_out = CHUNK;
        }

        if(ret != LZMA_OK) {
            if(ret == LZMA_STREAM_END) {
                break;
            }
            throw std::runtime_error("Compression failed.");
        }
    }

    fflush(result.f);
    return result;
}
