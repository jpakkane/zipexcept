project('zip exception experiment', 'cpp',
  version : '1.1.0',
  license : 'GPLv3+',
  default_options : ['cpp_std=c++14', 'warning_level=3', 'buildtype=debugoptimized'])

zdep = dependency('zlib', fallback : ['zlib', 'zlib_dep'])
if host_machine.system() != 'windows'
  lzmadep = dependency('liblzma', fallback : ['liblzma', 'lzma_dep'])
  compr_deps = [zdep, lzmadep]
else
  compr_deps = [zdep]
endif

utest_exe = find_program('unziptest.py')
subdir('src')
subdir('noexsrc')

