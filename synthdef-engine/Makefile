CC ?= cc
CFLAGS = -std=c99 -O2 -Wall -Wextra -Werror -Wpedantic -fno-fast-math -ffp-contract=off
SRC = src/fm_load.c src/fm_render.c
.PHONY: all test reference-test sanitize clean
all: build/fm-render build/fm-inspect build/libfmsynth.dylib
build:
	mkdir -p build
build/fm-render: $(SRC) src/fm_synth.h src/fm_filters.inc src/fm_noise.inc src/fm_bandlimited.inc src/fm_tables.inc src/fm_delay.inc src/fm_fast.inc $(wildcard src/fm_extensions.inc) tools/render.c | build
	$(CC) $(CFLAGS) -Isrc $(SRC) tools/render.c -lm -o $@
build/libfmsynth.dylib: $(SRC) src/fm_synth.h src/fm_filters.inc src/fm_noise.inc src/fm_bandlimited.inc src/fm_tables.inc src/fm_delay.inc src/fm_fast.inc $(wildcard src/fm_extensions.inc) | build
	$(CC) $(CFLAGS) -fPIC -shared -Isrc $(SRC) -lm -o $@
test: all
	python3 tests/test_engine.py
clean:
	rm -f build/fm-render build/libfmsynth.dylib

build/fm-inspect: src/fm_load.c src/fm_synth.h tools/inspect.c | build
	$(CC) $(CFLAGS) -Isrc src/fm_load.c tools/inspect.c -lm -o $@
reference-test: all
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
