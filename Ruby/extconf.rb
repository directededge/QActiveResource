require 'mkmf'
dir_config("QAR")
$INCFLAGS = "-I.. #{$INCFLAGS}"

qt_dir = File.readlink(`which qmake`.chomp).sub('/bin/qmake', '')

pkg_config("#{qt_dir}/lib/pkgconfig/QtCore.pc")

create_makefile("QAR")
