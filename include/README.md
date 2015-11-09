# Support files for generated flatbuffer files

These support files are not needed by modern C compilers when only
reading already created flatbuffers.

## Flatcc Builder Support

For user code to generate flatbuffers, the `libflatccbuilder` library
must be linked and `flatcc` must be in the include path to provide the
necessary headers for the library. This is not needed when only reading
flatbuffers.

## Portability Layer

Compile with `-DFLATCC_PORTABLE` to include
`flatcc/flatcc_portable.h` in generated header files, or
simply include the file before any generated files.

The portability layer is not really tested and is not prioritized. It
should be seen as a starting point such that a specific platform can
test with this layer and add compiler specific patches upstream if any
issues are discovered. Even so, care has been taken to only use features
that can be supported on a wide range of compilers.

Recent clang compilers versions should work without the portability
layer, also if std=-c11 is not specified in the project.

## Flatbuffer Library

The main source for parsing and generating code can be accessed via
commandline tool, or alternatively via the flatbuffers library. To
inteface with the library, include `flatcc/flatcc.h`. This
library and header is not of interest to generated code.
