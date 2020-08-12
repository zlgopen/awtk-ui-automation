import os
import app_helper as app

DEPENDS_LIBS = [
  {
    "root" : '../awtk-restful-httpd',
    'shared_libs': ['httpd'],
    'static_libs': []
  }
]

helper = app.Helper(ARGUMENTS);
helper.set_dll_def('src/ui_automation.def').set_libs(['ui_automation']).set_deps(DEPENDS_LIBS).call(DefaultEnvironment)

SConscriptFiles = ['src/SConscript', 'demos/SConscript', 'tests/SConscript']
SConscript(SConscriptFiles)
