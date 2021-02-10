#!/bin/bash

#Until the author can be arsed to create a makefile
g++ -Wall -g -o ../build/test PowerBox3PX.cpp program.cpp -I../deps/include -L../deps/lib -lpthread -l:libpaho-mqtt3c.a
