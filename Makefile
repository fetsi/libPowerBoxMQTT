filename = libPowerBoxMQTT.so.1.0.1
soname = libPowerBoxMQTT.so.1
libpath = ./deps/lib_arm-raspi
srcpath = ./src
buildpath = ./build
inclpath = ./deps/include

pbox : PowerBox3PX.o
	g++ -shared -o $(buildpath)/$(filename) -fPIC -Wl,-soname,$(soname) -Wl,--whole-archive $(libpath)/libpaho-mqtt3c.a -Wl,--no-whole-archive $(buildpath)/PowerBox3PX.o -lc

PowerBox3PX.o : $(srcpath)/PowerBox3PX.cpp
	g++ -c -fPIC -o $(buildpath)/PowerBox3PX.o $(srcpath)/PowerBox3PX.cpp -I$(inclpath) -L$(libpath) -lpthread -l:libpaho-mqtt3c.a


clean :
	rm $(buildpath)/$(filename) $(buildpath)/PowerBox3PX.o
