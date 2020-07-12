# Installation

For installation, please review the Makefile in the top level
directory (specifically the PREFIX variable). Then type

````
make && sudo make install
````

dependencies:

The toolbox depends on the following libraries:

- libjack
- libsndfile
- liblo
- libltc
- gtkmm-2.4 or gtkmm-3.0
- libxml++-2.6
- liblsl-dev or lsl-liblsl
- libfftw3-dev
- libfftw3-single3
- libfftw3-double3
- libcurl4-openssl-dev
- libboost-all-dev
- libgsl-dev
- libeigen3-dev

For de-installation, please type

````
sudo make uninstall
````

On debian-based systems, the dependencies may be satisfied by typing:

````
sudo apt-get install build-essential default-jre doxygen dpkg-dev g++ git imagemagick jackd2 libasound2-dev libboost-all-dev libcairomm-1.0-dev libcurl4-openssl-dev libeigen3-dev libfftw3-dev libfftw3-double3 libfftw3-single3 libgsl-dev libgtkmm-3.0-dev libgtksourceviewmm-3.0-dev libjack-jackd2-dev liblo-dev libltc-dev libmatio-dev libsndfile1-dev libwebkit2gtk-4.0-dev libxml++2.6-dev lsb-release make portaudio19-dev ruby-dev software-properties-common texlive-latex-extra texlive-latex-recommended vim-common wget
````

On archlinux, these tools may be needed:
````
pacman -Syu git make gcc pkg-config libxml++2.6 jack2 liblo libsndfile fftw gsl eigen gtkmm3 boost libltc xxd gtksourceviewmm webkit2gtk
````

debian packages of lsl-liblsl can be found at 
[https://github.com/sccn/liblsl/releases](https://github.com/sccn/liblsl/releases).

For installation of binary debian packages of TASCAR, please see [http://news.tascar.org/](http://news.tascar.org/).


Definition of docker build environments for TASCAR can be found here:

[https://github.com/HoerTech-gGmbH/docker-buildenv](https://github.com/HoerTech-gGmbH/docker-buildenv)

The docker files can be seen as a installation instruction of all
build dependencies.

Corresponding docker images can be found on docker hub:

[https://hub.docker.com/r/hoertech/docker-buildenv/tags?page=1&name=tascar](https://hub.docker.com/r/hoertech/docker-buildenv/tags?page=1&name=tascar)

