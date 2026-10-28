# Build and verify

The release includes unchanged source snapshots of fm1-base and felucca-bsp at the revisions recorded in `setup/dependencies.json`. No submodule download is needed. Keep these pins when reporting results.

## Host

Set up `.venv` as in QUICKSTART, then run from the project root:

```sh
export PATH="$PWD/.venv/bin:$PATH"
make -C synthdef-engine test
make -C fm1-synth test
make -C synthdef-engine sanitize
make -C fm1-synth sanitize
python3 fm1-synth/tools/test-panel.py
python3 fm1-synth/tools/test-extensions-panel.py
python3 fm1-synth/tools/test-reverbs-panel.py
```

The screen/protocol tests need Node.js. The simulator needs the pinned font dependencies and a working Pillow RAQM installation (`brew install fribidi` if needed). No connected FM-1 is required for these checks. Sanitizers use undefined-behaviour, bounds and float-cast checks; ASan is not claimed because its runtime hung before main on the test Mac.

With SuperCollider 3.14.1 installed in `/Applications`:

```sh
make -C synthdef-engine reference-test
python3 fm1-synth/tools/test-device-math.py --host
python3 fm1-synth/tools/test-device-math.py
```

The test build also uses C++17 for an isolated MiVerb oracle. Pinned DSP sources, plugin headers and the language class are included; no extra system plugin install or network download is required. `make test` includes the identical-input MiVerb comparison. Fixture export and browser compilation load stock SC classes plus the bundled MiVerb class, with user extensions excluded.

Run those reference commands sequentially: they share generated files. Reference audio is generated locally, not shipped as a large build cache. See the engine README for tolerances and fixture regeneration.

The development UGen additions require a new device build. The 0.1.0-preview release assets retain the earlier, hardware-tested 53-type firmware. See VERIFICATION.md for the distinction.

## Device firmware

Start Docker Desktop. Obtain the pinned JieLi Linux toolchain with:

```sh
bash setup/fetch-toolchain.sh
bash fm1-synth/tools/build-device.sh
```

The fetch script requires the recorded SHA-256; it will stop if the vendor's moving URL no longer serves the pinned archive. Use the matching archive supplied to the script as a path in that case. It never silently advances the compiler version.

The minimal SDK packaging data and its Apache-2.0 licence are included at the pinned revision under `dependencies/ac79-sdk`. `setup/verify-sdk.py` verifies the three file hashes before building. No radio libraries are included or linked. The Docker image is pinned by digest in `setup/docker-image.txt`.

Output: `fm1-synth/firmware/build/synthdef.fwsc`. Inspect the build report for image, RAM, pool and no-Bluetooth checks. Do not infer realtime safety from link success; new patches need playback timing checks with two voices and the effect.

## Distribution

`tools/package-release.py --output <new-directory>` creates a complete release tree and ZIP from this repository. It copies an explicit file set, includes the current built firmware, and writes a per-file SHA-256 manifest. It excludes local banks, virtual environments, compiler downloads, Git metadata and build caches. It requires a firmware matching the recorded acceptance hash, so changed firmware needs new verification before release.

The Git repository carries source, fixtures, example packages and selected evidence. Firmware and the complete source archive are release assets. Retain the GPL source, platform snapshots and third-party notices alongside any redistributed firmware.
