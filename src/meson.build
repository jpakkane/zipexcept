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

e1 = executable('exc-unzip',
  'exc-unzip.cpp',
  link_with : zl,
  install : true,
  link_args : linkargs,
)

test('unzip test', utest_exe, args : [meson.source_root(), meson.current_build_dir(), e1.full_path()])

