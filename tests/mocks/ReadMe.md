# Mocking non-virtual functions

There were two reasons to not use virtual functions for mocking:

1. No need to rewrite the code
2. Explore the field of mocking non-virtual functions

The solution to mock non-virtual functions relies on the linker to pull in a
different class implementation for the same header file. This different
implementation forwards all calls to a related "mock" class written as required
by the mock library [trompeloeil](https://github.com/rollbear/trompeloeil).

The forwarding implementation is necessary because the directory of a cpp file
is searched first for a header file. So the real header file will always be
found by the class to test instead of the header file with the mocked class.