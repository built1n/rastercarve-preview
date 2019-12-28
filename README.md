# RasterCarve-Preview

This program generates SVG previews of engraving G-code toolpaths. It
was designed for use with
[rastercarve](https://github.com/built1n/rastercarve), but should
be applicable with any such toolpath.

It uses [Dillon Huff's](https://github.com/dillonhuff) excellent
G-code parser, [gpr](https://github.com/dillonhuff).

## Requirements

- git
- cmake
- C++11 compiler (any recent GCC or clang will do)

## Installation

Compile with:

```
$ git clone --recursive https://github.com/built1n/rastercarve-preview
$ cd rastercarve-preview
$ mkdir build
$ cd build
$ cmake ..
$ make
```

If you don't use `git clone --recursive`,  you have to manually setup the submodules with:

```
$ git submodule update --init --recursive
```

## Usage

TODO

## License

GPLv2
