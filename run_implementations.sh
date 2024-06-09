#!/usr/bin/env sh

mkdir $1
mkdir $1/Treiber
mkdir $1/CoarseQueue
mkdir $1/CoarseStack
mkdir $1/HWQueue
mkdir $1/MSQueue
mkdir $1/TSStack
mkdir $1/TSStackUnlin
echo "Treiber"
./bin/run_treiber suites/$1.txt $1/Treiber/tr
echo "Coarse Queue"
./bin/run_coarse_queue suites/$1.txt $1/CoarseQueue/cq
echo "Coarse Stack"
./bin/run_coarse_stack suites/$1.txt $1/CoarseStack/cst
#echo "HW Queue"
#./bin/run_hwqueue suites/$1.txt $1/HWQueue/hwq
echo "MSQueue"
./bin/run_msqueue suites/$1.txt $1/MSQueue/msq

echo "TSStack"
./bin/run_tsstack suites/$1.txt $1/TSStack/tss

echo "TSStack Unlin"
./bin/run_tsstack_unlin suites/$1.txt $1/TSStackUnlin/tssu
