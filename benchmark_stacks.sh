echo "Running Cover..."
parallel "timeout 60 /usr/bin/time -f '{},%e' ./bin/linearize {} 2>&1 > /dev/null" ::: stack/*/*.hist > cover_result.csv
echo "Running Stack..."
parallel "timeout 60 /usr/bin/time -f '{},%e' ./bin/linearize -a stack {} 2>&1 > /dev/null" ::: stack/*/*.hist > stack_result.csv