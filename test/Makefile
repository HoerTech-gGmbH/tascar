TESTS = test_ir1 test_ir2 test_ir3 test_ir4 test_ir6_ismminmax		\
	test_snd1 test_snd2 test_snd4 test_snd3 test_snd5 test_pos1	\
	test_pos2 test_pos_trackinterp test_snd_door1 test_snd_door2	\
	test_snd_door3 test_pos_door test_snd_layers			\
	test_snd_rec_amb1h1v test_wav_diffuse test_wav_diffuse_hoa2d	\
	test_wav_diffuse_nsp4 test_wav_diffuse_hoa2d4			\
	test_wav_diffuse_hoa2d_179 test_wav_diffuse_layerbug

RECEIVERS = omni nsp amb3h0v amb3h3v amb1h0v amb1h1v cardioid	\
  neukom_basic neukom_inphase hann vbap vbap3d hoa2d ortf	\
  intensityvector vmic chmap hoa2d_fuma cardioidmod debugpos	\
  hoa2d_fuma_hos

LIBPATH=LD_LIBRARY_PATH=../libtascar/build/:../plugins/build/
VALIDATE=../apps/build/tascar_validatetsc
RENDERIR=../apps/build/tascar_renderir
RENDERFILE=../apps/build/tascar_renderfile
COMPARE=../apps/build/compare_sndfile

TEST_REC = $(patsubst %,test_rec_%,$(RECEIVERS))

FAILING_TESTS = test_ir5

all: $(TESTS) $(TEST_REC)

test_ir%: test_ir%.tsc expected_ir%.wav
	$(LIBPATH) $(VALIDATE) $< 
	$(LIBPATH) $(RENDERIR) -o test_ir$*.wav $< -t 1 -l 4000 -f 44100 
	$(LIBPATH) $(COMPARE) expected_ir$*.wav test_ir$*.wav 1e-9
	rm -f test_ir$*.wav

test_wav%: test_wav%.tsc input_wav%.wav expected_wav%.wav
	$(LIBPATH) $(VALIDATE) $< 
	$(LIBPATH) $(RENDERFILE) -i input_wav$*.wav -d -f 64 -o test_wav$*.wav $<
	$(LIBPATH) $(COMPARE) expected_wav$*.wav test_wav$*.wav 2e-8
	rm -f test_wav$*.wav

test_snd%: test_snd%.tsc expected_snd%.wav
	$(LIBPATH) $(VALIDATE) $< 
	$(LIBPATH) $(RENDERFILE) -i zeros.wav -d -f 64 -o test_snd$*.wav $<
	$(LIBPATH) $(COMPARE) expected_snd$*.wav test_snd$*.wav 1e-9
	rm -f test_snd$*.wav

db_test_rec_%: test_rec.tsc
	echo run -i zeros.wav -d -f 64 -o test_rec_$*.wav $< > gdbcmd
	$(LIBPATH) RECEIVER=$* gdb $(RENDERIR) -x gdbcmd
	rm -f test_rec_$*.wav

test_rec_%: test_rec.tsc
	$(LIBPATH) RECEIVER=$* $(RENDERFILE) -i zeros.wav -d -f 64 -o test_rec_$*.wav $<
	rm -f test_rec_$*.wav
#	$(LIBPATH) RECEIVER=$* $(VALIDATE) $< 
#$(LIBPATH) $(COMPARE) expected_rec_$*.wav test_rec_$*.wav 1e-9

test_pos%: test_pos%.tsc expected_pos%.wav
	$(LIBPATH) $(VALIDATE) $< 
	$(LIBPATH) $(RENDERFILE) -i zeros.wav -d -f 64 -o test_pos$*.wav $<
	$(LIBPATH) $(COMPARE) expected_pos$*.wav test_pos$*.wav 1e-9
	rm -f test_pos$*.wav

db_test_ir%: test_ir%.tsc expected_ir%.wav
	echo run -o test_ir$*.wav $< -t 1 -l 4000 -f 44100 > gdbcmd
	$(LIBPATH) gdb $(RENDERIR) -x gdbcmd
	$(LIBPATH) $(COMPARE) expected_ir$*.wav test_ir$*.wav 1e-9
	rm -f test_ir$*.wav

db_test_snd%: test_snd%.tsc expected_snd%.wav
	echo run -i zeros.wav -o test_snd$*.wav -d -f 64 $< > gdbcmd
	$(LIBPATH) gdb $(RENDERFILE) -x gdbcmd
	$(LIBPATH) $(COMPARE) expected_snd$*.wav test_snd$*.wav 1e-9
	rm -f test_snd$*.wav

db_test_pos%: test_pos%.tsc expected_pos%.wav
	echo run -i zeros.wav -o test_pos$*.wav -d -f 64 $< > gdbcmd
	$(LIBPATH) gdb $(RENDERFILE) -x gdbcmd
	$(LIBPATH) ../build/compare_posfile expected_pos$*.wav test_pos$*.wav 1e-9
	rm -f test_pos$*.wav

leak_test_snd%: test_snd%.tsc expected_snd%.wav
	$(LIBPATH) valgrind --leak-check=full $(RENDERFILE) -i zeros.wav -o test_snd$*.wav -d -f 64 $<
	$(LIBPATH) $(COMPARE) expected_snd$*.wav test_snd$*.wav 1e-9
	rm -f test_snd$*.wav

leak_test_pos%: test_pos%.tsc expected_pos%.wav
	$(LIBPATH) valgrind --leak-check=full $(RENDERIR) -i zeros.wav -o test_pos$*.wav -d -f 64 $<
	$(LIBPATH) ../build/compare_posfile expected_pos$*.wav test_pos$*.wav 1e-9
	rm -f test_pos$*.wav

leak_%: %.tsc
	$(LIBPATH) valgrind --leak-check=full  --show-leak-kinds=all $(RENDERIR) -i zeros.wav -o $*.wav -d -f 64 $<

clean:
	rm -f *~ test*.wav gdbcmd
