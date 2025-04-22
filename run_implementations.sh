#!/usr/bin/env sh

R="-r 10"

mkdir $1
mkdir $1/Treiber
mkdir $1/CoarseQueue
mkdir $1/CoarseStack
mkdir $1/MSQueue
mkdir $1/TSStack
mkdir $1/TSStackUnlin

echo "Treiber"
./bin/run_treiber $R suites/$1.txt $1/Treiber/tr
echo "Coarse Queue"
./bin/run_coarse_queue $R suites/$1.txt $1/CoarseQueue/cq
echo "Coarse Stack"
./bin/run_coarse_stack $R suites/$1.txt $1/CoarseStack/cst

echo "MSQueue"
./bin/run_msqueue $R suites/$1.txt $1/MSQueue/msq

echo "TSStack"
./bin/run_tsstack $R suites/$1.txt $1/TSStack/tss

echo "TSStack Unlin"
./bin/run_tsstack_unlin $R suites/$1.txt $1/TSStackUnlin/tssu
