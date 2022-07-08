lib=allocator.so         # For general use as a malloc replacement
liblib=liballocator.so   # For testing (does not include malloc overrides)

# Set the following to '0' to disable log messages:
LOGGER ?= 1

# Compiler/linker flags
CFLAGS += -g -Wall -fPIC -DLOGGER=$(LOGGER) -pthread -shared
LDLIBS +=
LDFLAGS += -L. -Wl,-rpath='$$ORIGIN'

$(lib):  allocator_overrides.c $(liblib)
	$(CC) $(CFLAGS) $(LDLIBS) $(LDFLAGS) allocator_overrides.c $(liblib) -o $@

$(liblib): allocator.c allocator.h logger.h
	$(CC) $(CFLAGS) $(LDLIBS) $(LDFLAGS) allocator.c -o $@

docs: Doxyfile
	doxygen

clean:
	rm -f $(lib) $(liblib)
	rm -rf docs


# Tests --

test_repo=usf-cs326-sp22/P3-Tests

test: $(lib) $(liblib) ./.testlib/run_tests ./tests
	@DEBUG="$(debug)" ./.testlib/run_tests $(run)

grade: ./.testlib/grade
	./.testlib/grade $(run)

testupdate: testclean test

testclean:
	rm -rf tests .testlib

./tests:
	rm -rf ./tests
	git clone https://github.com/$(test_repo) tests

./.testlib/run_tests:
	rm -rf ./.testlib
	git clone https://github.com/malensek/cowtest.git .testlib

./.testlib/grade: ./.testlib/run_tests
