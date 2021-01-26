#!/bin/bash
ulimit -c unlimited
apport-unpack /var/crash/_usr_bin_nautilus.1000.crash ./core-dumps
gdb ./seq.bin ./core-dumps/CoreDump

