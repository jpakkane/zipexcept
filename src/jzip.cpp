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
#include<vector>
#include<string>
#include<unistd.h>

#include"fileutils.h"
#include"zipcreator.h"

int main(int argc, char **argv) {
    if(argc < 3) {
        printf("%s <zip file> <files to archive>\n", argv[0]);
        return 1;
    }
    if(exists_on_fs(argv[1])) {
        printf("Output file already exists, will not overwrite.\n");
        return 1;
    }

    std::vector<std::string> files;
    files.push_back(argv[2]);
    // FIXME check for absolute paths.
    if(files.empty()) {
        printf("No input files listed.\n");
        return 1;
    }
    ZipCreator zc(argv[1]);
    try {
        zc.create(files);
    } catch(std::exception &e) {
        unlink(argv[1]);
        printf("Zip creation failed: %s\n", e.what());
        return 1;
    } catch(...) {
        unlink(argv[1]);
        printf("Zip creation failed due to an unknown reason.");
        return 1;
    }
    return 0;
    return 0;
}