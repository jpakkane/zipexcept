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

#pragma once

#include"ne_file.h"

#include<string>
#include<cstdint>
#include<cstdio>

class File;


/* Yes, this really should use std::variant<ResultType, Error>
 * but it's so new that compilers don't support it yet.
 */

struct Error {
    std::string msg;
};

Error* create_error(const char *msg);

void free_error(Error *e);

Error* create_system_error(const char *msg);

uint32_t CRC32(const unsigned char *buf, uint64_t bufsize);
uint32_t CRC32(File &f, Error **e);
