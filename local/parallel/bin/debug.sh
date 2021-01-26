#!/bin/bash
ulimit -c unlimited
./dcbc.bin 2 64 4 4 16 E
apport-unpack /var/crash/_usr_bin_nautilus.1000.crash ./core-dumps
gdb ./dcbc.bin ./core-dumps/CoreDump

