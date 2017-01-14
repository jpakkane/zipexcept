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
#include"ne_file.h"
#include"ne_utils.h"
#include<string>
#include<vector>
#include<thread>

class ZipFile {

public:
    ZipFile();
    ~ZipFile();

    void initialize(const char *fname, Error **e);

    size_t size() const { return entries.size(); }

    void unzip(const std::string &prefix, Error **e) const;

    const std::vector<localheader> localheaders() const { return entries; }

private:

    void run(const std::string &prefix, int num_threads) const;

    void readLocalFileHeaders(Error **e);
    void readCentralDirectory(Error **e);

    File zipfile;
    std::vector<localheader> entries;
    std::vector<centralheader> centrals;
    std::vector<long> data_offsets;

    zip64endrecord z64end;
    zip64locator z64loc;
    endrecord endloc;
    size_t fsize;

    mutable std::unique_ptr<std::thread> t;
};
