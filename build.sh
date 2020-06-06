#!/bin/sh
rm -rf ./build
mkdir build
gcc -o build/main.out src/sdl_main.c  -lglut -lGLEW -lGL `sdl2-config --cflags --libs` -lm
