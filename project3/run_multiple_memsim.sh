#!/bin/bash

LEVEL=2
ADDRFILE="addrs.txt"
SWAPFILE="swapfile.bin"
TICK=10

# Loop over different algorithms and frame counts
for ALGO in "FIFO" "LRU" "CLOCK" "ECLOCK"; do
    for FCOUNT in 4 8 16 32 64 128; do
        OUTFILE="out${ALGO}${FCOUNT}.txt"
        
        # Run memsim with the specified parameters
        ./memsim -p $LEVEL -r $ADDRFILE -s $SWAPFILE -f $FCOUNT -a $ALGO -t $TICK -o $OUTFILE

        echo "Program executed with algo=$ALGO, count=$FCOUNT, output written to $OUTFILE"
    done
done
