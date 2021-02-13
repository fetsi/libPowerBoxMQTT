#!/bin/bash

g++ -c -fPIC -o PowerBox3PX.o PowerBox3PX.cpp -I../deps/include -L../deps/lib_arm-raspi -lpthread -l:libpaho-mqtt3c.a
g++ -shared -o libPowerBoxMQTT.so.1.0.1 -fPIC -Wl,-soname,libPowerBoxMQTT.so.1 -Wl,--whole-archive ../deps/lib_arm-raspi/libpaho-mqtt3c.a -Wl,--no-whole-archive PowerBox3PX.o -lc
g++ -o testi_linkattu program.cpp libPowerBoxMQTT.so.1.0.1 -lpthread
