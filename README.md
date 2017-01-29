# Testing behavior of code written with exceptions and error objects

This is a simple Zip file unpacker originally from [Parzip](https://github.com/jpakkane/parzip). It originally used exceptions for all error flow but it was then converted into using only GLib style error objects. This gives us a real world code base which is useful for testing purposes.

## Compiling

    meson <build dir>
    ninja
    ninja test # to run the test suite

The error code version can be built with or without `-fno-exceptions`. To enable the switch invoke this command in the build dir:

    mesonconf -Dnoexcept=true
    ninja

How to disable it is left as an exercise to the reader. :)

## Error handling strategy

Every function that may fail takes an argument of type `Error **` which must point to an `Error *` with the value `nullptr`. The caller must then check the error value and if it is no longer `nullptr`, then an error has occurred and the caller must behave appropriately. Almost always this means aborting current work, releasing all resources and passing the error up the call chain.

This allows the error handler to return dynamic error messages and to be naturally thread safe. This is not possible with C APIs modeled after `errno` and `strerror`. 

## Measurements

[Are here](http://nibblestew.blogspot.com/2017/01/testing-exception-vs-error-code.html)
