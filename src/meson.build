if host_machine.system() == 'windows'
  linkargs = 'Ws2_32.lib'
else
  linkargs = []
endif

zl = static_library('parzcore',
  'zipfile.cpp',
  'decompress.cpp',
  'fileutils.cpp',
  'utils.cpp',
  'file.cpp',
  'mmapper.cpp',
  'taskcontrol.cpp',
  dependencies : compr_deps + [threaddep]
)

executable('parunzip',
  'parunzip.cpp',
  link_with : zl,
  install : true,
  link_args : linkargs,
  dependencies : threaddep,
)

utest_exe = find_program('unziptest.py')
test('unzip test', utest_exe, args : [meson.source_root(), meson.current_build_dir()])

