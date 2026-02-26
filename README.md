# Comanche055-C89

ANSI C89 port of the Apollo 11 Command Module guidance computer flight software (Comanche 055 / Colossus 2A).

Translated from the original AGC assembly source at [chrislgarry/Apollo-11](https://github.com/chrislgarry/Apollo-11).

## Overview

Self-contained, single-threaded console application. No external dependencies.  
Faithfully reproduces AGC fixed-point arithmetic, the Executive job scheduler, Waitlist timer system, and DSKY verb/noun interface rendered as ASCII in the terminal.

**Implemented:**

- **P00** — CMC Idling
- **V82 / R30** — Orbital parameters (apogee, perigee, TFF)
- **V16N36** — Mission clock
- **V35** — Lamp test
- **V36** — Fresh start
- **V37** — Program select
- **V21–V25** — Data load
- **V06 / V05** — Decimal / octal display

**Stubbed:** All other programs (alarm on selection).

## Build

Source is in `Comanche055-C89/`.

### CMake

```bash
cmake -B build
cmake --build build
```

### MSVC (cl)

```bash
cl /W4 /O2 /Fe:comanche055.exe *.c
```

### GCC / Clang

```bash
cc -std=c89 -Wall -O3 -o comanche055 *.c
```

## Usage

```bash
./comanche055
```

Keyboard mapping is shown on the DSKY display:

| Key | Function |
|-----|----------|
| `V` | VERB |
| `N` | NOUN |
| `E` | ENTR |
| `0`–`9` | Digits |
| `+` / `-` | Sign |
| `C` | CLR |
| `R` | RSET |
| `P` | PRO |
| `K` | KEY REL |
| `Q` | Quit |

Example: type `V 3 5 E` for lamp test, `V 1 6 N 3 6 E` for mission clock.

## Roadmap

- Graphical DSKY interface
- Full program implementations (P20–P25, P30–P39, P40–P47, P51–P53, P61–P67)
- State vector propagation and orbit integration
- IMU simulation
- Telemetry downlink display

## License

Public domain, consistent with the original AGC source code.
