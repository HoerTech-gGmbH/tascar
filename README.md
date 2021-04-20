# Toolbox for Acoustic Scene Creation and Rendering (TASCAR)

TASCAR is a collection of tools used for creation of spatially dynamic
acoustic scenes in various render formats, e.g., higher order
Ambisonics or VBAP. The toolbox is developed for applications in the
context of hearing research and hearing aid evaluation.

For installation of binary packages, please see
[install.tascar.org](http://install.tascar.org/)

## References:

Grimm, Giso; Luberadzka, Joanna; Hohmann, Volker. A Toolbox for
Rendering Virtual Acoustic Environments in the Context of
Audiology. Acta Acustica united with Acustica, Volume 105, Number 3,
May/June 2019, pp. 566-578(13),
[doi:10.3813/AAA.919337](https://doi.org/10.3813/AAA.919337)


Grimm, G.; Luberadzka, J.; Herzke, T.; Hohmann, V. (2015): Toolbox for
acoustic scene creation and rendering (TASCAR) - Render methods and
research applications. in: Proceedings of the Linux Audio Conference,
Mainz, 2015 [paper](http://lac.linuxaudio.org/2015/papers/11.pdf)


Grimm, G.; Hohmann, V. (2014): Dynamic spatial acoustic scenarios in
multichannel loudspeaker systems for hearing aid evaluations. in:
Proc. of the 17th annual meeting of the Deutsche Gesellschaft fuer
Audiologie, Oldenburg, 2014. [conferece
proceedings](http://www.uzh.ch/orl/dga-ev/publikationen/tagungsbaende/tagungsbaende.html)


Grimm, G., & Herzke, T. (2012). A framework for dynamic spatial
acoustic scene generation with Ambisonics in low delay realtime. In
F. Neumann (Ed.), Proceedings of the Linux Audio Conference. Stanford,
CA, USA: Center for Computer Research in Music and Acoustics, Stanford
University. [paper](http://lac.linuxaudio.org/2012/papers/14.pdf)


## Main author:

Giso Grimm

g.grimm@uni-oldenburg.de


## Testing:

C++ unit tests (gtest):

````
make unit-tests
````

Reproducibility test:

````
make test
````

Measure test coverage:

````
make clean cleancov
make allwithcov
make coverage
````

