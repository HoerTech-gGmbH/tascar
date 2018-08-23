**Module development for TASCAR**

See file:///usr/share/doc/libtascar/html/group__moddev.html for
documentation of the code.

*Compilation*

Adapt the Makefile and source code (src directory) to your needs. Then
compile with

make

*Testing*

The plugins can be tested with:

LD_LIBRARY_PATH=./build tascar

*Installation*

To install the plugins in /usr/local/lib, type:

sudo make install

To uninstall the plugins, type:

sudo make uninstall
