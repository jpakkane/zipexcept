# Testing behavior of code written with exceptions and error objects

This is a simple Zip file unpacker originally from [Parzip](https://github.com/jpakkane/parzip). It originally used exceptions for all error flow but it was then converted into using only GLib style error objects. This gives us a real world code base which is useful for testing purposes.

## Compiling

    meson <build dir>
    ninja

The error code version can be built with or without `-fno-exceptions`. To enable the switch invoke this command in the build dir:

    mesonconf -Dnoexcept=true

How to disable it is left as an exercise to the reader. :)
