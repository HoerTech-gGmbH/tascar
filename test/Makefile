TESTS = test_snd_zeros test_ir1 test_ir2 test_ir3 test_ir4						\
	test_ir6_ismminmax test_snd1 test_snd2 test_snd4 test_snd3					\
	test_snd5 test_pos1 test_pos2 test_pos_trackinterp test_snd_door1		\
	test_snd_door2 test_snd_door3 test_pos_door test_snd_layers					\
	test_snd_rec_amb1h1v test_wav_diffuse test_wav_diffuse_hoa2d				\
	test_wav_diffuse_nsp4 test_wav_diffuse_hoa2d4												\
	test_wav_diffuse_layerbug test_wav_diffirs test_sndgainramp					\
	test_sndgainramp2 test_ir_spkcorr test_snd_calib1 test_snd_calib2		\
	test_snd_vbap51 test_snd_pink test_snd_pink_diffuse									\
	test_snd_sndfile test_snd_sndfile2 test_snd_door4 test_snd_roommic	\
	test_fail_vbap3d_flat test_notfail_vbap3d_3d test_level_vbap3d			\
	test_ir_ortf_0deg test_ir_ortf_55deg test_ir_scatter								\
	test_ir_noscatter test_ir_scatter_ortf test_snd_sourcedir_generic		\
	test_snd_sourcedir_generic1 test_snd_sourcedir_generic_cardioid			\
	test_unload_on_error jack_routeplugin test_ir_wfs										\
	test_ir_wfs_noplanewave jackfail_connectfail jack_connect hoa3d			\
	test_fail_duplicatenames test_fail_duplicatescenenames							\
	test_level_hoa2d_basic test_level_hoa2d_maxre jack_hoafdnrot				\
	test_ir_reverb test_snd_reverb test_ir_simplefdn


TEST_SND=$(wildcard test_snd*.tsc)


RECEIVERS = omni nsp amb3h0v amb3h3v amb1h0v amb1h1v cardioid	\
  neukom_basic neukom_inphase hann vbap hoa2d ortf	\
  intensityvector vmic chmap hoa2d_fuma cardioidmod debugpos	\
  hoa2d_fuma_hos wfs



LEVELREC = nsp hoa2d neukom_basic wfs vbap hann
#neukom_inphase

LEVTHR=0.25
IRLEN = 4000

LIBPATH=LD_LIBRARY_PATH=../libtascar/build/:../plugins/build/
VALIDATE=../apps/build/tascar_validatetsc
RENDERIR=../apps/build/tascar_renderir
RENDERFILE=../apps/build/tascar_renderfile
TASCARCLI=../apps/build/tascar_cli
JACKLSP=../apps/build/tascar_lsjackp
JACKIO=../apps/build/tascar_jackio
COMPARE=../apps/build/compare_sndfile
COMPLEV=../apps/build/compare_level_sum

TEST_REC = $(patsubst %,test_rec_%,$(RECEIVERS))
TEST_LEV = $(patsubst %,test_reclevel_%,$(LEVELREC))

FAILING_TESTS = test_ir5

TOL = 1e-7
RMSTOL = 1e-7

all: $(TESTS) testrec testlev

test_reclevel_wfs test_reclevel_vbap test_reclevel_hann: LEVTHR=2.5

#test_level_hoa3d_allrad: LEVTHR=2.7

hoa3d: test_snd_hoa3d_enc_hor test_snd_hoa3d_enc_vert								\
	test_snd_hoa3d_pinv_hor test_snd_hoa3d_pinv_vert									\
	test_snd_hoa3d_allrad_hor test_snd_hoa3d_allrad_vert							\
	test_level_hoa3d_pinv test_level_hoa3d_pinv_lidhan								\
	test_level_hoa3d_pinv_maxre test_level_hoa3d_allrad								\
	test_level_hoa3d_allrad_lidhan test_level_hoa3d_pinv_lidhan_maxre


testrec: $(TEST_REC)

testlev: $(TEST_LEV) test_level_diff_decorr test_level_diff_nodecorr	\
  test_level_hoa2d_basic test_level_hoa2d_maxre												\
  test_level_hoa3d_allrad_lidhan test_level_hoa3d_allrad							\
  test_level_hoa3d_pinv_lidhan_maxre test_level_hoa3d_pinv_lidhan			\
  test_level_hoa3d_pinv_maxre test_level_hoa3d_pinv test_level_nsp		\
  test_level_vbap3d

db: $(TEST:test=db_test)  $(TEST_REC:test=db_test)

test_ir%: test_ir%.tsc expected_ir%.wav
	@echo "Impulse response test: $<"
	@$(LIBPATH) $(VALIDATE) $< || (echo "$(<):2: Validation failed"; false)
	@$(LIBPATH) $(RENDERIR) -o test_ir$*.wav $< -t 1 -l $(IRLEN) -f 44100 || (echo "$(<):2: Rendering failed"; false)
	@$(LIBPATH) $(COMPARE) expected_ir$*.wav test_ir$*.wav $(TOL) $(RMSTOL) || (echo "$(<):2: Output differs"; false)
	@rm -f test_ir$*.wav

test_wav%: test_wav%.tsc input_wav%.wav expected_wav%.wav
	@echo "Input-output processing test: $<"
	@$(LIBPATH) $(VALIDATE) $< || (echo "$(<):2: Validation failed"; false)
	@$(LIBPATH) $(RENDERFILE) -i input_wav$*.wav -d -f 64 -o test_wav$*.wav $< || (echo "$(<):2: Rendering failed"; false)
	@$(LIBPATH) $(COMPARE) expected_wav$*.wav test_wav$*.wav $(TOL) $(RMSTOL) || (echo "$(<):2: Output differs"; false)
	@rm -f test_wav$*.wav

test_snd%: test_snd%.tsc expected_snd%.wav
	@echo "Generation test: $<"
	@$(LIBPATH) $(VALIDATE) $< || (echo "$(<):2: Validation failed"; false)
	@$(LIBPATH) $(RENDERFILE) -i zeros.wav -d -f 64 -o test_snd$*.wav $< || (echo "$(<):2: Rendering failed"; false)
	@$(LIBPATH) $(COMPARE) expected_snd$*.wav test_snd$*.wav $(TOL) $(RMSTOL) || (echo "$(<):2: Output differs"; false)
	@rm -f test_snd$*.wav

test_pos%: test_pos%.tsc expected_pos%.wav
	@echo "Position test: $<"
	@$(LIBPATH) $(VALIDATE) $< || (echo "$(<):2: Validation failed"; false)
	@$(LIBPATH) $(RENDERFILE) -i zeros.wav -d -f 64 -o test_pos$*.wav $< || (echo "$(<):2: Rendering failed"; false)
	@$(LIBPATH) $(COMPARE) expected_pos$*.wav test_pos$*.wav 1e-9 1e-9|| (echo "$(<):2: Output differs"; false)
	@rm -f test_pos$*.wav

test_rec_%: test_rec.tsc
	@echo "Receiver test: $< ($*)"
	@$(LIBPATH) RECEIVER=$* $(RENDERFILE) -i zeros.wav -d -f 64 -o test_rec_$*.wav $< || (echo "$(<):2: Rendering failed"; false)
	@rm -f test_rec_$*.wav

test_reclevel_%: test_level.tsc
	@echo "Receiver level test: $< ($*)"
	@$(LIBPATH) RECEIVER=$* CHANNELS=8 $(RENDERFILE) -i zeros.wav -d -f 64 -o test_level_$*.wav $< || (echo "$(<):2: Rendering failed"; false)
	@$(LIBPATH) $(COMPLEV) 70 test_level_$*.wav $(LEVTHR) || (echo "$(<):2: Level differs (8ch)"; false)
	@$(LIBPATH) RECEIVER=$* CHANNELS=36 $(RENDERFILE) -i zeros.wav -d -f 64 -o test_level_$*.wav $< || (echo "$(<):2: Rendering failed"; false)
	@$(LIBPATH) $(COMPLEV) 70 test_level_$*.wav $(LEVTHR) || (echo "$(<):2: Level differs (36ch)"; false)
	@rm -f test_level_$*.wav

test_level_%: test_level_%.tsc
	@echo "Level test: $< ($*)"
	@$(LIBPATH) $(VALIDATE) $< || (echo "$(<):2: Validation failed"; false)
	@$(LIBPATH) RECEIVER=$* $(RENDERFILE) -i zeros.wav -d -f 64 -o test_level_$*.wav $< || (echo "$(<):2: Rendering failed"; false)
	@$(LIBPATH) $(COMPLEV) 70 test_level_$*.wav $(LEVTHR) || (echo "$(<):2: Level differs"; false)
	@rm -f test_level_$*.wav


test_fail_%: fail_%.tsc
	@(! $(LIBPATH) $(RENDERFILE) -i zeros.wav -d -f 64 -o test_fail.wav $< ) || (echo "$(<):2: Did not fail"; false)
	@rm -f test_fail.wav

test_notfail_%: notfail_%.tsc
	@$(LIBPATH) $(RENDERFILE) -i zeros.wav -d -f 64 -o test_fail.wav $< || (echo "$(<):2: Failed."; false)
	@rm -f test_fail.wav


# debugging rules:

db_test_wav%: test_wav%.tsc input_wav%.wav expected_wav%.wav
	$(LIBPATH) $(VALIDATE) $< 
	echo run -i input_wav$*.wav -d -f 64 -o test_wav$*.wav $< > gdbcmd
	$(LIBPATH) gdb $(RENDERFILE) -x gdbcmd
	$(LIBPATH) $(COMPARE) expected_wav$*.wav test_wav$*.wav 2e-8
	rm -f test_wav$*.wav

db_test_rec_%: test_rec.tsc
	echo run -i zeros.wav -d -f 64 -o test_rec_$*.wav $< > gdbcmd
	$(LIBPATH) RECEIVER=$* gdb $(RENDERIR) -x gdbcmd
	rm -f test_rec_$*.wav

db_test_ir%: test_ir%.tsc expected_ir%.wav
	echo run -o test_ir$*.wav $< -t 1 -l $(IRLEN) -f 44100 > gdbcmd
	$(LIBPATH) gdb $(RENDERIR) -x gdbcmd
	$(LIBPATH) $(COMPARE) expected_ir$*.wav test_ir$*.wav 1e-9 1e-9
	rm -f test_ir$*.wav

db_test_snd%: test_snd%.tsc expected_snd%.wav
	echo run -i zeros.wav -o test_snd$*.wav -d -f 64 $< > gdbcmd
	$(LIBPATH) gdb $(RENDERFILE) -x gdbcmd
	$(LIBPATH) $(COMPARE) expected_snd$*.wav test_snd$*.wav 1e-9 1e-9
	rm -f test_snd$*.wav

db_test_pos%: test_pos%.tsc expected_pos%.wav
	echo run -i zeros.wav -o test_pos$*.wav -d -f 64 $< > gdbcmd
	$(LIBPATH) gdb $(RENDERFILE) -x gdbcmd
	$(LIBPATH) ../build/compare_posfile expected_pos$*.wav test_pos$*.wav 1e-9 1e-9
	rm -f test_pos$*.wav

leak_test_snd%: test_snd%.tsc expected_snd%.wav
	$(LIBPATH) valgrind --leak-check=full $(RENDERFILE) -i zeros.wav -o test_snd$*.wav -d -f 64 $<
	$(LIBPATH) $(COMPARE) expected_snd$*.wav test_snd$*.wav 1e-9 1e-9
	rm -f test_snd$*.wav

leak_test_pos%: test_pos%.tsc expected_pos%.wav
	$(LIBPATH) valgrind --leak-check=full $(RENDERIR) -i zeros.wav -o test_pos$*.wav -d -f 64 $<
	$(LIBPATH) ../build/compare_posfile expected_pos$*.wav test_pos$*.wav 1e-9
	rm -f test_pos$*.wav

leak_%: %.tsc
	$(LIBPATH) valgrind --leak-check=full  --show-leak-kinds=all $(RENDERFILE) -i zeros.wav -o $*.wav -d -f 64 $<

valgrind_%: %.tsc
	$(LIBPATH) valgrind  $(RENDERFILE) -i zeros.wav -o $*.wav -d -f 64 $<


all_init: $(patsubst %,init_%,$(TEST_SND:.tsc=))

init_%: %.tsc
	$(LIBPATH) valgrind --leak-check=no --undef-value-errors=yes --track-origins=yes $(RENDERFILE) -i zeros.wav -o $*.wav -d -f 64 $<

clean:
	rm -f *~ test*.wav gdbcmd

test_unload_on_error: test_unload_on_error.tsc
	@sleep 1
	rm -f test_unload_on_error.loaded test_unload_on_error.unloaded
	jackd -d dummy & PID=$$!; sleep 1;$(LIBPATH) $(TASCARCLI) $< ; kill -9 $$PID
	test -e test_unload_on_error.loaded || (echo "load failed."; false)
	rm -f test_unload_on_error.loaded
	test -e test_unload_on_error.unloaded || (echo "unload failed."; false)
	rm -f test_unload_on_error.unloaded
	@sleep 1

jack_%: test_%.tsc input_%.wav expected_%.wav
	@$(LIBPATH) $(VALIDATE) $< || (echo "$(<):2: Validation failed"; false)
	@sleep 1
	@jackd -d dummy -r 44100 -p 1024 & JACKPID=$$!; sleep 1;$(LIBPATH) $(TASCARCLI) -s $< & TSCPID=$$!; sleep 1;echo "input: " `$(LIBPATH) $(JACKLSP) -si|grep -v -e '.*:sync_in'`;echo "output: " `$(LIBPATH) $(JACKLSP) -so|grep -v -e '.*:sync_out'` ; $(LIBPATH) $(JACKIO) -s 0 input_$*.wav -o test_$*.wav `$(LIBPATH) $(JACKLSP) -si|grep -v -e '.*:sync_in'` `$(LIBPATH) $(JACKLSP) -so|grep -v -e '.*:sync_out'`; kill -9 $$JACKPID $$TSCPID; true
	@$(LIBPATH) $(COMPARE) expected_$*.wav test_$*.wav $(TOL) $(RMSTOL) || (echo "$(<):2: Output differs"; false)
	@sleep 1

jackfail_%: test_%.tsc
	@sleep 1
	TSCRET=ok; jackd -d dummy -r 44100 -p 1024 & JACKPID=$$!; sleep 1;$(LIBPATH) $(TASCARCLI) -s $< & TSCPID=$$!; sleep 1;ps $$TSCPID > /dev/null || TSCRET=fail; kill -9 $$JACKPID $$TSCPID; test $$TSCRET = fail || (echo "$(<):2: Did not fail."; false)
	@sleep 1


test_snd_hoa3d_enc_hor test_snd_hoa3d_enc_vert test_snd_hoa3d_pinv_hor test_snd_hoa3d_allrad_hor test_snd_hoa3d_allrad_vert test_snd_hoa3d_pinv_vert: TOL=4e-4
test_snd_hoa3d_enc_hor test_snd_hoa3d_enc_vert test_snd_hoa3d_pinv_hor test_snd_hoa3d_allrad_hor test_snd_hoa3d_allrad_vert test_snd_hoa3d_pinv_vert: RMSTOL=1e-4
test_ir_simplefdn: IRLEN=44100
test_ir_reverb: IRLEN=44100
test_ir_partconvrev: IRLEN=44100
test_ir_temp: IRLEN=441000
