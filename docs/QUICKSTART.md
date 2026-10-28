# Mac setup and installation

Use a data-capable USB-C cable. This preview has been exercised on Apple silicon macOS. Windows and Linux setup are not validated.

## 1. Prepare the Mac

Install Apple's Command Line Tools (`xcode-select --install`), Python 3.11 or later, and SuperCollider 3.14.1 in `/Applications/SuperCollider.app`. SuperCollider is required to compile source; precompiled `.fmslot` files work without it. The tested Python was 3.14.7; the exact dependency versions are in `setup/requirements-*.txt`.

From the extracted release directory:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r setup/requirements-lock.txt -r setup/requirements-midi-lock.txt
make -C fm1-synth all
```

Node.js is needed only for browser-code tests. Docker and the JieLi toolchain are needed only to rebuild device firmware.

## 2. Install the firmware

The complete release archive contains `firmware/FM1-SynthDef-0.1.0-preview.fwsc`. A source checkout must build it first or obtain that file from the matching release. Check the archive against its published SHA-256 before use.

Installing custom firmware replaces the current instrument. Export any work you need from the current firmware first; this project does not migrate stock presets or samples. Keep power and USB connected during installation. Begin with MASTER low.

```sh
.venv/bin/python fm1-base/tools/fm1_install.py --info
.venv/bin/python fm1-base/tools/fm1_install.py firmware/FM1-SynthDef-0.1.0-preview.fwsc
.venv/bin/python fm1-base/tools/fm1_install.py --info
```

The installer asks before writing. The resulting identity is `FM-1_1200`. The MIDI port may still be named `TEMPLATE FM-1`; detection uses the firmware handshake. Do not use `--force` to bypass an unrecognised package.

Firmware and patch slots are separate: the firmware package does not preload this example bank. Empty synth slots are silent until you send a patch.

## 3. Open the editor and load patches

```sh
bash fm1-synth/tools/serve.sh
```

Open the complete launch URL printed by the server, including its session token. Keep that URL private. The editor runs locally on `127.0.0.1:8765`; do not expose it to your network. Only one manager should access a given local bank.

Click **Check FM-1**. For each entry in [the patch list](PATCHES.md), select its numbered slot, **Import file**, choose its `.fmslot`, and **Save slot** to keep a local copy. Then **Send to FM-1 slot N** to write the hardware. Wait for the successful readback message. Send the master echo to slot 0 and Keys to slot 1 first.

A local save does not write hardware. **Read FM-1 slot** retrieves a device patch as a draft; save it locally if you want to retain it. The local bank is created in `fm1-synth/data/local-bank.flash`, and survives a server restart.

Choose slot 1 on the device, raise MASTER gently and play. Knobs 1–2 shape Keys; 3–4 shape the echo. The editor's Listen button is an offline preview using local slot 0; physical knobs update hardware playback live.

## 4. Make a patch

Paste one expression returning a `SynthDef`, without `.add` or server boot code. Click **Compile on this Mac**, **Save slot**, then **Send to FM-1 slot N**. Unsupported graph features produce an error. Compilation evaluates SuperCollider code on your Mac: only compile source you trust.

Start from an included example, keep `Out.ar(0, ...)`, use a gate envelope with `doneAction: 2`, and name live controls `knob1` through `knob4`. Review [compatibility](COMPATIBILITY.md).

## Troubleshooting and recovery

- Silence: check MASTER, select a populated synth slot, and ensure the patch reaches `Out.ar(0, ...)`. Slot 0 is an effect, not a keyboard instrument.
- Overload: HOME clears notes and resets the effect; simplify the patch. Graph size alone does not predict processing cost.
- No device: close other MIDI tools, reconnect the USB data cable and run the identity query above. Stock firmware cannot answer the slot-manager handshake.
- Interrupted installation: the pinned installer's guidance is to run the same installation again to finish if the device remains in update mode. Keep the matching package available.
- Stock recovery: consult the included `fm1-base/bsp/tools/fm1_rescue.py` and upstream recovery documentation. Recovery was not exercised for this preview; no stock recovery firmware is bundled.
