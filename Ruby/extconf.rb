require 'mkmf'
dir_config("QAR")

$LIBS << " -lcurl"

qmake_path = `which qmake`.chomp

if qmake_path.empty?
  warn "********************************************************************************"
  warn 'Cannot find qmake (should be installed with Qt development packages)'
  warn "********************************************************************************"
  exit
end

qmake_path = File.readlink(qmake_path) if File.symlink?(qmake_path)
qt_path = qmake_path.sub('/bin/qmake', '')

pkg_config("#{qt_path}/lib/pkgconfig/QtCore.pc")

create_makefile("QAR")
