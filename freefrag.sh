#!/bin/bash

BLK_DEV=$1
ROUND=$2
OUT="freefrag.out"

RESULT="e2freefrag $BLK_DEV"

echo "$BLK_DEV round# $ROUND" >> $OUT
echo "$RESULT" >> $OUT
