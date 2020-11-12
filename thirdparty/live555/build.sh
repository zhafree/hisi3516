#!/bin/sh

cd /workspace/hisi/hi3516dv300/3rd/himpp
make clean && make static && make install

cd /workspace/hisi/hi3516dv300/3rd/live555
#./genMakefiles armlinux-himix200-no-openssl
make