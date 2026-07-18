# HLS Design Generation

A C++ tool that automates **Design Space Exploration (DSE)** for High-Level Synthesis (HLS) by randomly generating valid Vivado/Vitis HLS directive files. Instead of manually crafting pragma combinations, this tool reads a `.lib` metadata file describing the target design's loops, arrays, and functions, then emits 100 randomised TCL directive scripts ready for HLS evaluation.

---

## Table of Contents

- [Overview](#overview)
- [How It Works](#how-it-works)
- [Repository Structure](#repository-structure)
- [Library (.lib) File Format](#library-lib-file-format)
- [Design Space Knobs](#design-space-knobs)
- [Generated Directive Examples](#generated-directive-examples)
- [Supported Benchmarks](#supported-benchmarks)
- [Building the Project](#building-the-project)
- [Configuration & Usage](#configuration--usage)
- [Known Limitations & Roadmap](#known-limitations--roadmap)
- [License](#license)

---

## Overview

High-Level Synthesis transforms C/C++ descriptions into RTL; its quality of results (QoR — latency, throughput, area) is highly sensitive to the directives (pragmas) applied to loops, arrays, and functions. The search space is combinatorially large, making manual exploration impractical.

This project addresses that by:

1. **Parsing** a `.lib` metadata file that describes which loops, arrays, and functions exist in a target HLS design, along with their key parameters (e.g., loop trip count, array size).
2. **Randomly sampling** the directive search space — choosing loop unrolling vs. pipelining, array reshape type and factor, and function inlining vs. instantiation.
3. **Emitting** 100 unique TCL directive scripts per run, each representing a different design point that can be fed directly to Vivado/Vitis HLS for synthesis and evaluation.

---

## How It Works

```
  .lib metadata file
        │
        ▼
  generate_ds (parser)
   ┌──────────────────────┐
   │  loop_list           │
   │  array_list          │
   │  func_list           │
   └──────────────────────┘
        │
        ▼
  dse::start_dse (100 iterations)
   ┌──────────────────────────────────────────────┐
   │  1. Shuffle loop/array/function order        │
   │  2. Randomly assign per-loop directive       │
   │     (unroll factor  OR  pipeline II)         │
   │  3. Randomly assign per-array reshape        │
   │     (block / cyclic / complete, with factor) │
   │  4. Randomly assign per-function directive   │
   │     (inline off/recursive OR instantiate)    │
   │  5. Write directive_N.tcl                    │
   └──────────────────────────────────────────────┘
        │
        ▼
  100 × .tcl directive files
  → feed into Vivado/Vitis HLS
  → collect latency / resource reports
  → rank Pareto-optimal designs
```

---

## Repository Structure

```
HLS_Design_Generation/
├── README.md
├── HLS_Design_Generation.sln            # Top-level VS2015 solution
│
├── HLS_Design_Generation/               # VS project wrapper
│   ├── HLS_Design_Generation.vcxproj
│   └── HLS_design_generation/           # Core source project
│       ├── main.cpp                     # All class implementations + main()
│       ├── general.h                    # Standard library includes
│       ├── parameter.h                  # Base class: parameter
│       ├── loop.h                       # Derived class: loop  (+  loop_list typedef)
│       ├── hls_array.h                  # Derived class: hls_array (+  array_list typedef)
│       ├── func.h                       # Derived class: func  (+  func_list typedef)
│       ├── generate_ds.h                # Parser class: generate_ds
│       ├── dse.h                        # DSE engine class: dse
│       ├── HLS_design_generation.vcxproj
│       └── HLS_design_generation.vcxproj.filters
│
└── library_file/                        # Sample .lib metadata files
    ├── loop_aes.lib
    ├── loop_apdcm.lib
    ├── loop_blowfish.lib
    ├── loop_dfadd.lib
    ├── loop_dfdiv.lib
    ├── loop_dfsin.lib
    ├── loop_gsm.lib
    ├── loop_jpeg.lib
    ├── loop_mips.lib
    └── loop_sha.lib
```

### Class hierarchy

```
parameter  (base)
├── loop        → stored in  loop_list  (std::list<loop>)
├── hls_array   → stored in  array_list (std::list<hls_array>)
└── func        → stored in  func_list  (std::list<func>)

generate_ds   owns loop_list, array_list, func_list; parses a .lib file
dse           consumes a generate_ds object; runs the randomised DSE loop
```

---

## Library (.lib) File Format

Each `.lib` file describes one HLS benchmark with three mandatory sections:

```
--------------Loop----------------
function_name  loop_name  max_iteration
loop  <function>  <loop_label>  <trip_count>
...

-----------Array-----------
array_name  num_elements  dimension  function_name(s)
array  <array_name>  <size>  <function>
...

----------Functions----------
function_name  argument_list
function  <func_name>  [arg1  arg2  ...]
...

----------Interfaces-----------
ap_none
ap_fifo
ap_bus
axis
m_axi
```

**Parser rules** (in `generate_ds` constructor, `main.cpp:61-96`):

| First token | Fields consumed | Stored as |
|---|---|---|
| `loop` | `[1]=function` `[2]=loop_name` `[3]=trip_count` | `loop_list` |
| `array` | `[1]=name` `[2]=size` `[3]=function` | `array_list` |
| `function` | `[1]=name` `[2..n]=args` | `func_list` |

Lines starting with `#` or other tokens are silently skipped (comment/disabled records, e.g. `#loop encrypt L3 9` in `loop_aes.lib`).

---

## Design Space Knobs

All knobs are defined in `dse.h`:

| Category | Options |
|---|---|
| **Loop unroll factor** | `{0, 2, 4, 8, 16, 20}` — clamped to loop trip count |
| **Pipeline II** | Random integer in `[0, 3]` |
| **Array reshape type** | `block`, `cyclic`, `complete` |
| **Array reshape factor** | `{2, 4, 6, 8}` — clamped to array size |
| **Function inline** | `""` (default inline), `off`, `recursive` |
| **Function instantiate** | one of the function's argument variables |

Per iteration the lists are shuffled, then loops are **split** into a random unrolled portion and a pipelined portion (`main.cpp:186-220`). This ensures mixed unroll/pipeline strategies are explored.

---

## Generated Directive Examples

A sample generated TCL file might look like:

```tcl
# Loop directives
set_directive_unroll -factor 8 "encrypt/L1"
set_directive_pipeline -II 1 "MixColumn_AddRoundKey/L5"
set_directive_pipeline -II 2 "decrypt/L2"

# Array directives
set_directive_array_reshape -type cyclic -factor 4 -dim 1 "aes_main" key
set_directive_array_reshape -type complete -dim 1 "ByteSub_ShiftRow" Sbox

# Function directives
set_directive_inline -recursive "decrypt"
set_directive_function_instantiate "encrypt" type
```

These files are consumed verbatim by Vivado/Vitis HLS directive scripts.

---

## Supported Benchmarks

| Benchmark | Loops | Arrays | Functions | Domain |
|---|---|---|---|---|
| **AES** | 7 | 7 | 9 | Symmetric encryption |
| **ADPCM** | 12 | 9 | 11 | Audio codec |
| **Blowfish** | 4 | 4 | 3 | Block cipher |
| **DFADD** | 0 | 0 | 6 | Double-precision FP add |
| **DFDIV** | 0 | 0 | 3 | Double-precision FP divide |
| **DFSIN** | 0 | 0 | 13 | Double-precision FP sin |
| **GSM** | 9 | 2 | 7 | Speech codec |
| **JPEG** | 25 | 11 | 16 | Image decode |
| **MIPS** | 3 | 2 | 0 | Processor simulation |
| **SHA** | 9 | 1 | 3 | Hash function |

These are standard CHStone / MachSuite HLS benchmark kernels.

---

## Building the Project

### Windows (Visual Studio 2015 or later)

1. Open `HLS_Design_Generation.sln` in Visual Studio.
2. Select **Release | x64** configuration.
3. Build → Build Solution (`Ctrl+Shift+B`).

### Linux / macOS (GCC or Clang, C++11 or later)

```bash
cd HLS_Design_Generation/HLS_Design_Generation/HLS_design_generation
g++ -std=c++11 -O2 -o hls_dse main.cpp
```

No external dependencies are required — only the C++11 standard library.

---

## Configuration & Usage

Before running, update the two hardcoded paths in `main.cpp`:

| Line | Purpose | Example value |
|---|---|---|
| `57` | `.lib` input file path (inside `generate_ds` constructor) | `"path/to/library_file/loop_aes.lib"` |
| `179` | Output directory + filename prefix for `.tcl` files | `"path/to/output/directive"` |

Then run the compiled binary. It will produce 100 TCL files named `directive0.tcl` … `directive99.tcl` in the configured output location.

```bash
./hls_dse
```

### Evaluating generated designs in Vivado HLS

```tcl
# In your Vivado HLS project TCL script:
open_project my_project
set_top my_top_function
add_files my_design.cpp
open_solution "solution1"
set_part {xc7z020clg484-1}
create_clock -period 10 -name default
source directive0.tcl    ;# load one generated directive file
csynth_design
```

Repeat for each generated directive file, collect results, and rank designs by latency, LUT, DSP, BRAM, etc.

---

## Known Limitations & Roadmap

| Issue | Description |
|---|---|
| Hardcoded paths | `main.cpp:57` ignores the constructor argument; both paths must be edited manually before each run |
| II = 0 possible | `random(3)` returns 0–3, so a pipeline II of 0 may be emitted, which is typically invalid in HLS |
| Unroll factor = 0 | The `loop_factor` vector contains 0, potentially emitting `set_directive_unroll -factor 0` |
| Repeated `srand` seeding | Calling `srand(time(0))` inside the iteration loop reduces entropy for fast runs |
| Interface section unused | The `Interfaces` section in `.lib` files is parsed as comments but never used to generate interface directives |

**Possible future improvements:**

- Accept `.lib` path and output directory as command-line arguments
- Add `set_directive_interface` generation from the Interfaces section
- Support exhaustive enumeration mode (small benchmarks) alongside random sampling
- Integrate with Python scripting to post-process HLS reports and rank designs automatically
- Replace `random_shuffle` + multiple `srand` calls with a single seeded `std::mt19937` engine

---

## License

This project does not currently include a license file. Please contact the repository owner before using this code in commercial or research publications.
