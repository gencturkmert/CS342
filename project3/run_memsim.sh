#!/bin/bash

# Example parameters
LEVEL=2
ADDRFILE="addr.txt"
SWAPFILE="swapfile.bin"
FCOUNT=4
ALGO="CLOCK"
TICK=100
OUTFILE="output.txt"

# Run memsim with the specified parameters
./memsim -p $LEVEL -r $ADDRFILE -s $SWAPFILE -f $FCOUNT -a $ALGO -t $TICK -o $OUTFILE
