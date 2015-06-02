#!/bin/bash

eval `date -u +'BYR=%Y BMON=%-m BDOM=%-d BHR=%-H BMIN=%-M'`
chr () {
    printf \\$(($1/64*100+$1%64/8*10+$1%8))
}
b36 () {
    if [ $1 -le 9 ]; then echo $1; else chr $((0x41 + $1 - 10)); fi
}
printf '%c%c%c%c%c\n' `chr $((0x41 + $BYR - 2011))` `b36 $BMON` `b36 $BDOM` `b36 $BHR` `b36 $(($BMIN/2))`
