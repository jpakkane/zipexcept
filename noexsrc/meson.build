if host_machine.system() == 'windows'
  linkargs = 'Ws2_32.lib'
else
  linkargs = []
endif

if get_option('noexcept')
  cpp_args = ['-fno-exceptions']
else
  cpp_args= []
endif

zl = static_library('noexccore',
  'ne_zipfile.cpp',
  'ne_decompress.cpp',
  'ne_fileutils.cpp',
  'ne_utils.cpp',
  'ne_file.cpp',
  'ne_mmapper.cpp',
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

