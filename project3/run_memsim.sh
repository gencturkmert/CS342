#!/bin/bash

# Example parameters
LEVEL=2
ADDRFILE="addrs.txt"
SWAPFILE="swapfile.bin"
FCOUNT=16
ALGO="CLOCK"
TICK=10
OUTFILE="output.txt"

# Run memsim with the specified parameters
./memsim -p $LEVEL -r $ADDRFILE -s $SWAPFILE -f $FCOUNT -a $ALGO -t $TICK -o $OUTFILE
