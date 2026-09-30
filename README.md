# ISP — Witten & Abbess (1979), extracted and rebuilt

Source: *witten1979_synth by rule.pdf*, the appendix "ISP listing" (printed pp. 595–620).
ISP (Integrated Synthesis Program) is a real-time speech synthesis-by-rule program. It
ran on an LSI-11 driving hardware formant synthesizers (PAT, ICL, Computalker).

The appendix is a scanned image; the PDF's text layer is garbage there. It was
transcribed from 600 dpi renders of the 26 landscape pages.

## Layout

| Path | What |
|---|---|
| `original/isp.h`, `original/isp.c`, `original/rule` | Verbatim transcription of the listing (1979 C, K&R), split into header, program, and rule table |
| `original/listing_full.txt` | The whole appendix as one text file, in reading order |
| `port/` | Modernized copy that builds and runs today (changes marked `PORT:`) |
| `port/port.h`, `port/port.c` | Host stand-ins for the LSI-11 hardware, plus a PAT-style formant renderer to WAV |
| `port/examples/` | Sample input, WAV output, parameter tracks, synthesis list, spectrogram |

Note: the code on printed pp. 605–606 actually continues from p. 608. The pages are
out of order in the journal/scan, so the code is assembled in content order:
PDF pages 20, 23, 24, 21, 22, 25.

## Build

Windows (MSVC 2019): `port\build.bat` produces `port\isp.exe`.

gcc/clang (Linux/WSL/macOS):

```bash
gcc -std=gnu89 -w -O2 -o isp isp.c -lm
```

Only `isp.c` is compiled; it `#include`s `port.c`.

## Run

```bash
cd port && ./isp.exe -o out.wav < examples/jack.txt
```

- `-r FILE`: the rule table (default `rule`). It plays the role of the host sending the table over the DR-11 link.
- `-o FILE`: the WAV output (default `isp.wav`, 16 kHz mono).
- Utterances come in on stdin. Messages go to stderr. Synthesis-list and parameter output go to stdout.

Input notation (from the paper): `1 dhis  iz  dhuh  /hauus  dhuht  /*dzhaak  bilt. !`

- `1`, `1+`, `2`, … select an intonation contour. `/` starts a foot, `/*` starts the stressed foot.
- Two spaces mark a word boundary. `.` and `;` end tone groups. `!` ends the utterance and triggers synthesis.
- The phoneme codes are the first column of the ph table in `rule`.

Directives (start a line with `@`):

- `@o1 0` prints the synthesis list (rhythm and pitch analysis).
- `@o2 0` prints per-10 ms PAT parameter frames.
- `@o0 0` produces speech (the default).
- `@s` restarts and re-reads the rules.
- `@r1` selects rule set 1. `@e0` turns off the per-utterance error report.

## Defects in the printed listing (fixed only in `port/`)

- `main()`: two closing braces were lost at a page break (pp. 600/601).
- `rhythm()`: missing `}` after `smark = 0;`.
- `computalker()`: missing `{` after its `if (... < ADVANCLOCK)`.
- `icl()`: `state=2` where `==` is clearly meant (as in `pat()` and `computalker()`).
- The `#define NMISC ...` line was wrapped onto two lines by the typesetter.

## Porting changes (all marked `PORT:` in the source)

- 1970s syntax: `=+`, `=&`, `=<<` etc. became `+=`, `&=`, `<<=`. Initializers like `int x 0;` became `int x = 0;`.
- 64-bit safety:
  - Forward declarations for functions that return pointers or chars.
  - `allocit()` sizes and aligns list elements, which now hold 8-byte pointers.
- The 32/16-bit `ldiv` is reimplemented.
- Two latent bugs that were harmless on the PDP-11 but crash a modern machine:
  - A divide by zero in `phdurs()`. The LSI-11's DIV doesn't trap, and the result was unused.
  - `lexindex[-1]` read on event elements in the phoneme scans.
- Hardware:
  - The device registers, DR-11 and console I/O are replaced by files and stdio.
  - `runsynth()` calls the synthesizer's interrupt handler in a loop in place of the real-time clock.

## Caveats

- Everything up to the parameter frames is ISP's own code and data: prosody, event scheduling, and scaling to PAT's 6-bit parameters.
- The WAV renderer is a stand-in for the PAT hardware, which the paper doesn't describe. It is a cascade F1–F3 synthesizer with a sibilance resonator at fh, and its bandwidths and gains are my choices. Expect robotic 1970s formant speech.
- Transcription: the phoneme table and the log table were re-checked against the scan (the log table matches `round(8192·log2(1+i/32))` exactly). Other readings were resolved by C sense and are noted in the port comments.
