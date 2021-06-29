# Sparse based linting

This tool checks LTP test and library code for common problems.

## Building

The bad news is you must get and build Sparse[^1]. The good news this only
takes a minute.

If you already have the Sparse source code, then do the
following. Where `$SRC_PATH` is the path to the Sparse directory.

```sh
$ cd tools/sparse
$ make -C $SRC_PATH -j$(nproc) # Optional
$ make SPARSE_SRC=$SRC_PATH
```
If not then you can fetch it via the git submodule

```sh
$ cd tools/sparse
$ git submodule update --init
$ cd sparse-src
$ make -C sparse-src -j$(nproc)
$ make
```

## Usage

It is integrated with the LTP build system. Just run `make check` or
`make check-a_test01`, where `a_test01` is an arbitrary test
executable or object file.

[1]: Many distributions have a Sparse package. This only contains some executables. There is no shared library
