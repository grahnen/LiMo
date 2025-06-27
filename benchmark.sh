#!/usr/bin/env sh

ARG=$2
TIMEOUT="${ARG:=60}"

echo "Timeout: ${TIMEOUT}"

echo "running violin"
parallel --use-cpus-instead-of-cores "timeout ${TIMEOUT} /usr/bin/time -f '{},%e' ../violin/bin/logchecker.rb -a saturate -r -i {} 2>&1 > /dev/null" ::: $1/*/*.hist > violin_result.csv
echo "LiMo"
parallel --use-cpus-instead-of-cores "timeout ${TIMEOUT} /usr/bin/time -f '{},%e' ./bin/linearize {} 2>&1 > /dev/null" ::: $1/*/*.hist > limo_result.csv
