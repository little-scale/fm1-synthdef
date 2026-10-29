CC ?= cc
CFLAGS = -std=c99 -O2 -Wall -Wextra -Werror -Wpedantic -fno-fast-math -ffp-contract=off
SRC = src/fm_load.c src/fm_render.c
.PHONY: all test reference-test sanitize clean
all: build/fm-render build/fm-inspect build/libfmsynth.dylib
build:
	mkdir -p build
build/fm-render: $(SRC) src/fm_synth.h src/fm_filters.inc src/fm_noise.inc src/fm_bandlimited.inc src/fm_tables.inc src/fm_delay.inc src/fm_fast.inc $(wildcard src/*.inc) tools/render.c | build
	$(CC) $(CFLAGS) -Isrc $(SRC) tools/render.c -lm -o $@
build/libfmsynth.dylib: $(SRC) src/fm_synth.h src/fm_filters.inc src/fm_noise.inc src/fm_bandlimited.inc src/fm_tables.inc src/fm_delay.inc src/fm_fast.inc $(wildcard src/*.inc) | build
	$(CC) $(CFLAGS) -fPIC -shared -Isrc $(SRC) -lm -o $@
test: all
	$(CC) $(CFLAGS) -Isrc src/fm_load.c tests/decay_sparse.c -lm -o build/decay-sparse-test
	build/decay-sparse-test
	python3 tests/test_engine.py
	python3 tests/test_expansion.py
	python3 tests/test_gendy.py
	python3 tests/test_gendy_oracle.py
	python3 tests/test_gendy_oracle.py --device
	python3 tests/test_miverb_oracle.py
	python3 tests/test_mimu_oracle.py
clean:
	rm -f build/fm-render build/libfmsynth.dylib

build/fm-inspect: src/fm_load.c src/fm_synth.h tools/inspect.c | build
	$(CC) $(CFLAGS) -Isrc src/fm_load.c tools/inspect.c -lm -o $@
reference-test: all
	$(CC) $(CFLAGS) -Isrc src/fm_load.c tests/decay_sparse.c -lm -o build/decay-sparse-test
	build/decay-sparse-test
	python3 tests/compare_reference.py
sanitize: | build
	$(CC) -std=c99 -O1 -g -ffp-contract=off -fsanitize=undefined,bounds,float-cast-overflow -fno-sanitize-recover=all -Isrc $(SRC) tests/fuzz.c -lm -o build/fuzz-ubsan
	build/fuzz-ubsan fixtures/perc.scsyndef
	build/fuzz-ubsan fixtures/m2_LagControl_array.scsyndef
	build/fuzz-ubsan fixtures/m2_Select_audio.scsyndef
	build/fuzz-ubsan fixtures/m2_RLPF_ar_step.scsyndef
	build/fuzz-ubsan fixtures/m3_noise_mix.scsyndef
	build/fuzz-ubsan fixtures/m3_Pulse_ar_glide.scsyndef
	build/fuzz-ubsan fixtures/m4_DelayN_ar_step.scsyndef
	build/fuzz-ubsan fixtures/m4_CombC_audio.scsyndef
	build/fuzz-ubsan fixtures/m4_AllpassL_ar_negative.scsyndef
	build/fuzz-ubsan fixtures/m5_random_noise.scsyndef
	build/fuzz-ubsan fixtures/m5_addy.scsyndef
	build/fuzz-ubsan fixtures/m6_SampleRate.scsyndef
	build/fuzz-ubsan fixtures/m6_Delay2_ar_initial.scsyndef
	build/fuzz-ubsan fixtures/m6_OnePole_ar_step.scsyndef
	build/fuzz-ubsan fixtures/m6_Latch_ar_ar.scsyndef
	build/fuzz-ubsan fixtures/m6_TRand_ar_ar.scsyndef
	build/fuzz-ubsan fixtures/m6_Dust_ar_2.scsyndef
	build/fuzz-ubsan fixtures/m6_Impulse_ar_step.scsyndef
	build/fuzz-ubsan fixtures/m6_Decay2_ar_2.scsyndef

	build/fuzz-ubsan fixtures/m7_FreeVerb_noise.scsyndef
	build/fuzz-ubsan fixtures/m7_FreeVerb2_noise.scsyndef
	build/fuzz-ubsan fixtures/m7_MiVerb_freeze.scsyndef

# Exercise hostile mutations through each new state family.
	build/fuzz-ubsan fixtures/m8_Fold_ar_1.scsyndef
	build/fuzz-ubsan fixtures/m8_Trig_ar_0.scsyndef
	build/fuzz-ubsan fixtures/m8_Stepper_ar_2.scsyndef
	build/fuzz-ubsan fixtures/m8_XFade2_ar_1.scsyndef
	build/fuzz-ubsan fixtures/m8_LinExp_ar_0.scsyndef
	build/fuzz-ubsan fixtures/m8_MiMu_dc3.scsyndef
	build/fuzz-ubsan fixtures/m9_LFDNoise3_ar_1.scsyndef
	build/fuzz-ubsan fixtures/m9_SyncSaw_ar_1.scsyndef
	build/fuzz-ubsan fixtures/m9_Amplitude_kr_ar_0.scsyndef
	build/fuzz-ubsan fixtures/m9_BLowShelf_2_2.scsyndef

	build/fuzz-ubsan fixtures/m10_Gendy1_ar_cp64.scsyndef
	build/fuzz-ubsan fixtures/m10_Gendy2_ar_cp64.scsyndef
	build/fuzz-ubsan fixtures/m10_Gendy3_ar_cp64.scsyndef
