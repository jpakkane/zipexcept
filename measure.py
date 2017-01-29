#!/usr/bin/env python3

# Copyright (C) 2017 Jussi Pakkanen.
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of version 3, or (at your option) any later version,
# of the GNU General Public License as published
# by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import shutil, os, sys, subprocess

class Measurer:
    def __init__(self):
        self.builddir = 'tmpbuild'
        self.meson = shutil.which('meson')
        if not self.meson:
            self.meson = shutil.which('meson.py')
        if not self.meson:
            self.meson = '../meson/meson.py'
        self.ninja = 'ninja'
        self.strip = 'strip'
        self.basic_options = ['--buildtype=debugoptimized']
        self.exc_bin = os.path.join(self.builddir, 'src/exc-unzip')
        self.errcode_bin = os.path.join(self.builddir, 'noexsrc/noexc-unzip')

    def clear(self):
        if os.path.exists(self.builddir):
            shutil.rmtree(self.builddir)

    def conf_and_build(self, env, extra_options=[]):
        self.clear()
        subprocess.check_call([self.meson, self.builddir] + self.basic_options + extra_options,
                              env=env)
        subprocess.check_call([self.ninja, '-C', self.builddir])
        subprocess.check_call([self.strip, self.exc_bin])
        subprocess.check_call([self.strip, self.errcode_bin])

    def sizes(self):
        estat = os.stat(self.exc_bin)
        errcodestat = os.stat(self.errcode_bin)
        return (estat.st_size, errcodestat.st_size)

    def measure(self):
        env = os.environ.copy()
        env['CXX'] = 'g++'
        self.conf_and_build(env)
        gcc_ex_size, gcc_errcode_size = self.sizes()
        self.conf_and_build(env, ['-Dnoexcept=true'])
        _, gcc_ecode_noexcept_size = self.sizes()
        env['CXX'] = 'clang++'
        self.conf_and_build(env)
        clang_ex_size, clang_errcode_size = self.sizes()
        self.conf_and_build(env, ['-Dnoexcept=true'])
        _, clang_ecode_noexcept_size = self.sizes()
        print('GCC exception size:', gcc_ex_size)
        print('GCC errorcode size:', gcc_errcode_size)
        print('GCC errorcode,noexcept size:', gcc_ecode_noexcept_size)
        print('Clang exception size:', clang_ex_size)
        print('Clang errorcode size:', clang_errcode_size)
        print('Clang errorcode,noexcept size:', clang_ecode_noexcept_size)

if __name__ == '__main__':
    m = Measurer()
    m.measure()
