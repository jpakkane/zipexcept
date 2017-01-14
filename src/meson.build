if host_machine.system() == 'windows'
  linkargs = 'Ws2_32.lib'
else
  linkargs = []
endif

zl = static_library('exccore',
  'zipfile.cpp',
  'decompress.cpp',
  'fileutils.cpp',
  'utils.cpp',
  'file.cpp',
  'mmapper.cpp',
  dependencies : compr_deps
)

executable('exc-unzip',
  'exc-unzip.cpp',
  link_with : zl,
  install : true,
  link_args : linkargs,
)

utest_exe = find_program('unziptest.py')
test('unzip test', utest_exe, args : [meson.source_root(), meson.current_build_dir()])

