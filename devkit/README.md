# Module development for TASCAR

See [file:///usr/share/doc/libtascar/html/group__moddev.html](file:///usr/share/doc/libtascar/html/group__moddev.html) for
documentation of the code.

## Compilation

Adapt the Makefile and source code (src directory) to your needs. Then
compile with

make

## Testing

The plugins can be tested with:

LD_LIBRARY_PATH=./build tascar

## Installation

To install the plugins in /usr/local/lib, type:

sudo make install

To uninstall the plugins, type:

sudo make uninstall

## Compilation of openMHA wrapper

The openMHA wrapper plugin `tascar_ap_openmha` can be compiled with

```
make MODULES=tascar_ap_openmha
```

To install the compiled plugin, use

```
sudo make MODULES=tascar_ap_openmha install
```
