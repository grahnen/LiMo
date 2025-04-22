# Li(nearizability)Mo(nitor)
A linearizability verifier for concurrent stacks, and a framework for implementing specialized monitors for arbitrary data structures.

# Building
## Manually
In order to build LiMo, you need a =C++20= compiler, =cmake=, =boost=, and =MPI=, and either hardware support for atomic-operations, or the =libatomic= libraries.

Compile the program by running the provided =compile.sh= script. Internally it uses CMAKE, and searches for required libraries.
# Algorithms
For now, the tool contains two monitoring algorithms; one for stacks and one for queues.

In addition to these, it contains some other algorithms we previously developed.

The algorithms in our PLDI paper "Efficient Linearizability Monitoring" are present in `CoverMonitor` and `QueueMonitor`.

# Running
## Single history
To check a history, execute

``` sh
./bin/linearize -a [cover|segment] ./path/to/history.hist
```

which checks the history `history.hist` for linearizability. Optionally append the argument `-v` for verbose output.
## Exhaustive testing
All histories containing exactly `N` values can be checked using
``` sh
mpirun ./bin/exhaustive -a <algoritm> -c <comparison algoritm> N
```
This runs <algoritm> and <comparison algoritm> on every history of size N, and counts how many histories that are i) linearizable, ii) unlinearizable, and iii) not agreed upon by the two algorithms.

This is mostly used for developing *new* monitoring algorithms for which an existing monitor already exists.

Note that it requires MPI, and that if run from within a docker container as the root user, you might need to run

``` sh
mpirun --allow-run-as-root ./bin/exhaustive ...
```

If you get a count of 0, then likely mpirun was only able to create one process. If so, add the argument =-np <k>= to =mpriun= to make sure at least one checker process is active.

## Random testing
There are two methods of generating random tests; one consists of running a stack implementation, and the other consists of generating stack histories algorithmically.
### Execution testing
Implementations are available in ``tool/impl/src/*.cpp``. For each implementation IMPL, runner binaries will be compiled as

``` sh
./bin/run_IMPL
```

which takes a number of arguments. The required input is a ``suite`` filename and an output filename.

If run with the ``-m benchmark`` parameter, the generated histories will not be saved, but instead used as benchmarking tools. The results is recorded in output as json data.

This is how the script `benchmark.sh` collects histories.

Additional parameters are `-r` for repetitions of each configuration in the suite, and `-i` for incremental increases in a suite consisting of an lower and upper bound.
### Generated testing
Generated testing can be done by running

``mpirun ./bin/random <suite> <output>``

This program takes arguments `-i` and `-r` the same way as in execution testing. It also takes the argument `-a algorithm`, where one can supply the monitoring algorithm to use.

Note that it requires MPI, and that if run from within a docker container as the root user, you might need to run

``` sh
mpirun --allow-run-as-root ./bin/random ...
```

Make sure to check that mpirun was able to create more than one process. You can add the argument =-np <k>= to =mpriun= to make sure.

## Histories
The first line of an input history should be

``# @object [atomic-stack|atomic-queue|...]``

Where other data structures than the two mentioned above are still work in progress.
Following this line, a sequence of lines of the form
``
[thread] event
``
where `thread` is an integer and `event` identifies the event, e.g. `call push(3)` or `return`. For instance, the following is a valid history.

```
# @object atomic-stack
[1] call push 1
[2] call pop
[1] return
[2] return 1
```

For more, see the histories directory, containing both simple histories for testing certain structured histories, and generated histories.
Histories can be generated using the `run` tool

``` sh
./bin/run_IMPL [-m mode] [-s suite]  [-i increment] [-r repeats] output-file [-v]
```

Where `IMPL` is one of the stack implementations in the /impl/ directory.


# Structure 

The code consists of several modules. The central monitoring executable is the `linearize` executable. It depends on a generic parser, that parses an input history. Based on the `@object <type>` line in this history, and on the command line parameter `-a algorithm`, it then picks a monitoring algorithm.

The monitoring algorithms are each represented by one instance of the `Monitor` class. It has generic methods for handling events, and each implementation overrides these generics that match the structure for which the monitor is specialized. The monitor for stacks is `CoverMonitor`, and the monitor for queues is `QueueMonitor`.


## Extending
Adding a new implementation of a data structure requires creating a cpp file in the =impl=-directory, in which you implement the ADTImpl class specified in =include/impl.h=.
It requires four methods; a constructor, an add operation, a removal operation, and a destructor. The internal state of the implementation is available to the methods through the state void pointer. Create a struct in the heap and store whatever data you need there, and assign state as a pointer to it.

Adding a new monitor for an existing data structure requires
- Implementing it as a subclass of Monitor
- Adding it to the `enum Algorithm` in `algorithm.hpp`, as well as adding it to the other functions in this file.

Adding a new data structure requires
- Modifying `macros.h` to add missing event types and operation types.
- Creating a monitor for the structure.
