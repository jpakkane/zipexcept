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

#include"ne_zipdefs.h"
#include"ne_utils.h"
#include<string>

class TaskControl;

struct UnpackResult {
    bool success;
    std::string msg;
};

UnpackResult unpack_entry(const std::string &prefix,
        const localheader &lh,
        const centralheader &ch,
        const unsigned char *data_start,
        uint64_t data_size,
        Error **e);
