# Project 3: Memory Allocator

See: https://www.cs.usfca.edu/~mmalensek/cs326/assignments/project-3.html

The project develops a custom memory allocator. The allocator uses  mmap to allocate entire regions of memory at a time.
The project also uses free space management algorithms to split up and reuse empty regions: first, best, and worst fit.

## Building

To compile and run:

```bash
make
LD_PRELOAD=$(pwd)/allocator.so command
```

## Program Options

```bash
$ make

Options:
    * LD_PRELOAD=$(pwd)/allocator.so command         for one specific command - 'command' will run with allocator
    * export LD_PRELOAD=$(pwd)/allocator.so          everything after this point will use your custom allocator
```

## Included Files

* **allocator.c** -- Implementations of allocator functions.
* **allocator.h** -- Function prototypes and structures for our memory allocator implementation.
* **fallocator_overrides.c** -- Contains stubs that call into the custom allocator library.

## Testing

To execute the test cases, use `make test`. To pull in updated test cases, run `make testupdate`. You can also run a specific test case instead of all of them:

```
# Run all test cases:
make test

# Run a specific test case:
make test run=4

# Run a few specific test cases (4, 8, and 12 in this case):
make test run='4 8 12'

# Run a test case in gdb:
make test run=4 debug=on
```

If you are satisfied with the state of your program, you can also run the test cases on the grading machine. Check your changes into your project repository and then run:

```
make grade
```

## Demo Run

add a screenshot / text of a demo run of your program here.
