require 'mkmf'
require 'pathname'

dir_config("QAR")

$LIBS << " -lcurl"

qmake_path = `which qmake`.chomp

if qmake_path.empty?
  warn "********************************************************************************"
  warn 'Cannot find qmake (should be installed with Qt development packages)'
  warn "********************************************************************************"
  exit
end

qmake_path = Pathname.new(qmake_path).realpath
qt_path = qmake_path.sub(/\/bin\/qmake.*$/, '')

if ENV['PKG_CONFIG_PATH'] 
  ENV['PKG_CONFIG_PATH'] += ":#{qt_path}/lib/pkgconfig"
else
  ENV['PKG_CONFIG_PATH'] = "#{qt_path}/lib/pkgconfig"
end

pc = "#{qt_path}/lib64/pkgconfig/QtNetwork.pc"

if !File.exists? pc
  pc = "#{qt_path}/lib/pkgconfig/QtNetwork.pc"
end

if !File.exists? pc
  warn "Could not find QtNetwork.pc"
  exit 1
end

pkg_config(pc)

create_makefile("QAR")
