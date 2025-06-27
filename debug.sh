#!/bin/sh

echo "Compiling"
./compile.sh
echo "Running"
./bin/linearize $1 -v
echo "Drawing dots"
./drawdot.sh
echo "Creating timeline"
./tex_hist.sh $1
