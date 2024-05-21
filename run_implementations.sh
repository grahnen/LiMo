#!/usr/bin/env sh

mkdir $1
mkdir $1/Treiber
mkdir $1/CoarseQueue
mkdir $1/CoarseStack
mkdir $1/HWQueue
mkdir $1/MSQueue

./bin/run_treiber suites/$1.txt $1/Treiber/tr

./bin/run_coarse_queue suites/$1.txt $1/CoarseQueue/cq

./bin/run_coarse_stack suites/$1.txt $1/CoarseStack/cst

./bin/run_hwqueue suites/$1.txt $1/HWQueue/hwq

./bin/run_msqueue suites/$1.txt $1/MSQueue/msq
