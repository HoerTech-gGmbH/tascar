ALLTEX = $(wildcard tutorial*.tex)
ALLPDF = $(ALLTEX:.tex=.pdf)

all: $(ALLPDF) doxy

tutorial0.pdf: ODIR=tutorial0_basics
tutorial1.pdf: ODIR=tutorial1_sensors
tutorial2.pdf: ODIR=tutorial2_matlab_octave
tutorial3.pdf: ODIR=tutorial3_openmha
tutorial4.pdf: ODIR=tutorial4_roomacoustic
tutorial5.pdf: ODIR=tutorial5_audiovisual
tutorial6.pdf: ODIR=tutorial6_headtrackedheadphone
tutorial7.pdf: ODIR=tutorial7_music_light

%.pdf: %.tex
	pdflatex -draftmode -halt-on-error -file-line-error $<
	pdflatex -halt-on-error -file-line-error $<
	-mkdir -p ../examples/$(ODIR)
	cp $@ ../examples/$(ODIR)

clean:
	rm -Rf *.aux *.log *.idx *.out $(ALLPDF) *~ latex html

doxy: $(ALLPDF)
	rm -Rf html
	doxygen doxygen.cfg
