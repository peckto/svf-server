# SVF Server

The SVF Server separates the SVF graph generation from the analysis build on top of the graph.
The graph is build once in memory and you can load our applications as a shared library.
In this way, the time-consuming task of creating the SVF graph must only be done once and the development and execution of the graph applications can be done independently.

This project is based on the svf-example project (https://github.com/SVF-tools/SVF-example).

## 1. Build 

```
$ CC=/usr/bin/clang CXX=/usr/bin/clang++ Z3_DIR=~/Downloads/z3-z3-4.8.14/ LLVM_DIR=/ SVF_DIR=~/Downloads/SVF cmake -DCMAKE_BUILD_TYPE=Debug
$ make
```

## 2. Analyze a bc file

```
$ clang -S -c -g -fno-discard-value-names -emit-llvm example.c -o example.ll
$ export LD_DEBUG=libs 
$ bin/svf-server example.ll
> help
Available commands:
    load <lib.so>  load library
    ls             show available functions provided by loaded library
    run <func>     run function from loaded library
    help|?         show this help
    exit           stop the server

> load src/libsvf-plugin.so
> ls
plugin help
    findAllMemcpys           finds and prints all memcpy calls
    findArrayMemcpys         finds and prints all memcpy calls with array type dst buffer
    findMissingRetCodeCheck  [ignore func, ...] finds missing return code checks
> run findMissingRetCodeCheck
```
