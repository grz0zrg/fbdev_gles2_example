# fbdev_gles2_example

A clean C example / sample program of OpenGL ES2 using fbdev `/dev/fb0` device

This program display a red fullscreen quad.

Tested on NanoPI Fire 3 hardware (Mali 400 GPU) with `s5p6818-sd-friendlycore-xenial-4.4-arm64-20190128.img` image file (provided by friendlyarm)

Compiled with : `gcc fullquad_gles2_fbdev.c -lEGL -o fullquad_gles2_fbdev`

Made it because there was no simple functional fbdev + GLES2 sample programs running on the NanoPI Fire 3.