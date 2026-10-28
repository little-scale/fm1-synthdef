# Slot transport 1 / capabilities 3 — FM-1 USB

An application SysEx frame is `F0 7D 46 4D 53 01 <packed frame> F7`. `7D` is the non-commercial/development ID. The remaining literal prefix reads FMS plus transport version 1. This namespace is distinct from the manufacturer updater and fm1-base services. Do not transmit slot data without a successful INFO response.

Packing groups up to seven binary bytes: first a byte of their high bits (bit 0 belongs to the first byte), then each byte's low seven bits. No padding group. Unused high bits in a short group must be zero.

The decoded frame has: version u8 = 1, command u8, request ID u16, payload length u16, payload, CRC32 u32. All integers are little endian. CRC32 is IEEE/zlib over everything preceding the CRC. Maximum decoded frame is 256 bytes. Replies set command bit 7 and echo request ID; the first payload byte is status. Statuses: 0 OK, 1 malformed, 2 revision/data conflict, 3 I/O failure, 4 ordering/transaction error, 5 invalid package.

| Command | Request payload | Successful response after status |
| --- | --- | --- |
| INFO 0 | empty | capabilities version u8 = 3, slots u8 = 12, voices u8 = 2, synth slot capacity u32 = 16128, chunk maximum u32 = 192, per-voice delay maximum u32 = 65536, effect slot capacity u32 = 7936 |
| READ 1 | slot u8, offset u32, requested length u16, expected generation u32 | generation u32, total length u32, offset u32, bytes |
| BEGIN 2 | slot u8, expected generation u32, total length u32, whole-package CRC32 u32, transaction ID u32 | empty |
| WRITE 3 | transaction ID u32, offset u32, up to 192 bytes | received contiguous length u32 |
| COMMIT 4 | transaction ID u32 | new generation u32 |
| ABORT 5 | transaction ID u32 | empty |

READ generation `0xFFFFFFFF` discovers the current generation; subsequent chunks must specify it. Other operations require an exact generation. Never-saved slots have generation 0. A clear uses a zero-length transfer with CRC32 of empty data. Tombstones have their own generation so stale clients cannot resurrect an old slot accidentally.

BEGIN only stages in RAM; a new BEGIN replaces an unfinished transaction. WRITE must be contiguous. Identical already-received chunks are accepted; conflicting overlaps or gaps are rejected. COMMIT requires the complete package, its checksum and a successful application validation callback before writing flash. The application must validate package lengths, UTF-8/metadata, binary SynthDef and memory budget using an inactive graph. It must also establish a safe audio/flash window. A missing validator rejects all nonempty commits.

One request is in flight at a time. The browser retries an identical packet up to three total sends at 1.5-second intervals. The C server caches the immediately previous request/reply, making an immediate lost-ACK retry idempotent, including COMMIT. After an ambiguous timeout or reconnect, READ the slot/generation before retrying a save. Do not report “nothing changed” merely because an acknowledgement was lost. An uncommitted transaction remains only in volatile RAM; a later BEGIN replaces it and reboot discards it. Bytes are committed only after the new bank's payload and final header are written and verified.

USB endpoint dispatch, staged commit, readback and browser upload/download are implemented. Synth slots 1–8 retain wire indexes 0–7 and their existing flash addresses. FX1–FX4 use wire indexes 8–11. They share two 8 KiB journal banks at 0xD7000 and 0xD9000. The physical payload is `FXB1`, four little-endian u32 package lengths, then four concatenated packages. The 20-byte directory leaves 7,916 bytes shared package capacity; the INFO effect capacity remains the physical payload bound of 7,936 bytes for legacy reads. An old plain FMB1 payload reads as FX1 with the other effects empty, and migrates on first write. All four logical effects share the physical bank's generation: refresh all four cached revisions after a successful FX commit. A save exceeding shared storage fails without changing flash. Flash at 0xDB000 and above remains excluded. Every FX commit checks all saved synth delay requirements; every synth commit checks all saved effects. The total delay arena is 131072 bytes for both voices and the effect; each synth voice remains limited to 65536 bytes.

Read-only diagnostic 0x7f now returns 14 values (five seven-bit bytes each): the previous 12 counters, visible code-slot index, and effect-active flag. The selected synth index remains separate from the visible code index. Diagnostic 0x7e remains the explicit, interrupting benchmark.

Diagnostic 0x7a is read-only panel telemetry: twelve values, each encoded as five seven-bit bytes: FX control mode (0/1), effect choice (0 bypass, 1–4 presets), pending switch flag, eight knob values in percent, fade gain in thousandths. View indexes 8–11 identify FX code; index 12 is bypass. The selected synth index remains 0–7.
