# first argument exhaustive|linearize
# if arg1 == linearize
#		arg2 - history
#		arg3 - [naive_|empty]

if [ $1 == "exhaustive" ]; then
	echo "exhaustive testing"
	echo "mpirun ./bin/$1 -a dstack_unknown_after -b naive_dstack_unknown_after -d unknown-after --size $2 -c $3"
	mpirun ./bin/$1 -a dstack_unknown_after -b naive_dstack_unknown_after -d unknown-after --size $2 -c $3

elif [ $1 == "linearize" ]; then
	echo "linearizing"
	echo "./bin/$1 $2 -a ${3}dstack_unknown_after"
	./bin/$1 $2 -a "${3}dstack_unknown_after"
fi