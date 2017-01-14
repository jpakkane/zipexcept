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

#include<cstdio>
#include<thread>

#ifdef _WIN32
#include<WinSock2.h>
#include<Windows.h>
#endif

#include"ne_zipfile.h"
#include"ne_utils.h"

int main(int argc, char **argv) {
    if(argc != 2 ) {
        printf("%s <zip file>\n", argv[0]);
        return 1;
    }
    Error *e = nullptr;
    ZipFile f;
    f.initialize(argv[1], &e);
    if(e) {
        printf("Opening file failed: %s\n", e->msg.c_str());
        free_error(e);
        return 1;
    }
    f.unzip("", &e);
    if(e) {
        printf("Unzipping failed: %s\n", e->msg.c_str());
        free_error(e);
        return 1;
    }
    return 0;
}
