libname = libPowerBoxMQTT
apiversion = 1
minorversion = 0
filename = $(libname).so.$(apiversion).$(minorversion)
soname = $(libname).so.$(apiversion)
linkname = $(libname).so
deplibpath = ./deps/lib_arm-raspi
srcpath = ./src
buildpath = ./build
inclpath = ./deps/include
libinstallpath = /usr/local/lib
headerinstallpath = /usr/local/include


pbox : PowerBox3PX.o
	g++ -shared -o $(buildpath)/$(filename) -fPIC -Wl,-soname,$(soname) -Wl,--whole-archive $(deplibpath)/libpaho-mqtt3c.a -Wl,--no-whole-archive $(buildpath)/PowerBox3PX.o -lc

PowerBox3PX.o : $(srcpath)/PowerBox3PX.cpp
	g++ -c -fPIC -o $(buildpath)/PowerBox3PX.o $(srcpath)/PowerBox3PX.cpp -I$(inclpath) -L$(deplibpath) -lpthread -l:libpaho-mqtt3c.a


install:
	cp $(buildpath)/$(filename) $(libinstallpath) && cd $(libinstallpath) && ln -s $(filename) $(soname) && ln -s $(soname) $(linkname)
	cp $(srcpath)/PowerBox3PX.hpp $(headerinstallpath)


uninstall:
	rm $(libinstallpath)/$(filename) $(libinstallpath)/$(soname) $(libinstallpath)/$(linkname)
	rm $(headerinstallpath)/PowerBox3PX.hpp

clean :
	rm $(buildpath)/$(filename) $(buildpath)/PowerBox3PX.o
