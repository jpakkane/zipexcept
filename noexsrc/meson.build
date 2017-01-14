if host_machine.system() == 'windows'
  linkargs = 'Ws2_32.lib'
else
  linkargs = []
endif

cpp_args = [] # '-fno-exceptions']

zl = static_library('noexccore',
  'zipfile.cpp',
  'decompress.cpp',
  'fileutils.cpp',
  'utils.cpp',
  'file.cpp',
  'mmapper.cpp',
  'error.cpp',
  cpp_args : cpp_args,
  dependencies : compr_deps
)

e2 = executable('noexc-unzip',
  'noexc-unzip.cpp',
  link_with : zl,
  install : true,
  cpp_args : cpp_args
)

test('noex unzip test', utest_exe, args : [meson.source_root(), meson.current_build_dir(), e2.full_path()])

