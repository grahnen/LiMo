#!/usr/bin/env sh

echo "running VIOLIN"
parallel --jobs 48 "timeout 60 /usr/bin/time -f '{},%e' ../violin/bin/logchecker.rb -a saturate -r -i {} 2>&1 > /dev/null" ::: $1/*/*.hist > violin_result.csv
echo "LIMOUSINE"
parallel --jobs 48 "timeout 60 /usr/bin/time -f '{},%e' ./bin/linearize {} 2>&1 > /dev/null" ::: $1/*/*.hist > limousine_result.csv
