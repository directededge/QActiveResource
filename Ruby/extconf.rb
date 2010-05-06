require 'mkmf'
dir_config("QAR")

$CFLAGS << " -I/usr/local/include"
$LIBS << " -lqactiveresource"

qt_dir = File.readlink(`which qmake`.chomp).sub('/bin/qmake', '')

pkg_config("#{qt_dir}/lib/pkgconfig/QtCore.pc")

create_makefile("QAR")
