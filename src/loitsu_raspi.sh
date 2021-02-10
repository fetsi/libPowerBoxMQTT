#!/bin/bash

#Until the author can be arsed to create a makefile


#Compile raspi version
g++ -Wall -g -o ../build/test PowerBox3PX.cpp program.cpp -I../deps/include -L../deps/lib_armv8-raspi -lpthread -l:libpaho-mqtt3c.a
