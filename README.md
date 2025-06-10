# Custom Branch Predictor for gem5

## Overview
This project adds a custom branch predictor to the gem5 simulator. It uses 6 bits of the branch address and 6 bits of global history, combined using a NAND operation to index a 64-entry Pattern History Table (PHT) with 2-bit saturating counters.

## Features
- **Indexing**: NAND of 6-bit PC and 6-bit global history
- **PHT**: 64-entry table with 2-bit saturating counters
- **Supported Methods**: `lookup`, `update`, `uncondBranch`, `btbUpdate`, `squash`
- **Benchmarks Used**: `dijkstra_small`, `qsort_small`

---

## Setup Instructions

### 1. Add Predictor Class in `BranchPredictor.py`

Edit `gem5/src/cpu/pred/BranchPredictor.py` and add:

```python
class CustomBP(BranchPredictor):
    type = 'CustomBP'
    cxx_class = 'CustomBP'
    cxx_header = "cpu/pred/custom.hh"
    PredictorSize = Param.Unsigned(64, "Size of predictor (entries).")
    PHTCtrBits = Param.Unsigned(2, "Bits per counter.")
    globalHistoryBits = Param.Unsigned(6, "Bits of the global history.")
```

### 2: Modify `SConscript`

To ensure that your custom branch predictor is compiled correctly, add the following lines at the **end** of `gem5/src/cpu/pred/SConscript`:

```
Source('custom.cc')
DebugFlag('Mispredict')
DebugFlag('CDebug')
```

### 3: Implementing the Custom GShare Branch Predictor

Navigate to: `gem5/src/cpu/pred`:

Create the following files:

- `custom.hh` – Header file containing class definition and variables
- `custom.cc` – Source file implementing all required predictor logic
---

### 4: Finally, build the gem5 project

```
scons build/ARM/gem5.opt -j$(nproc)
```

The Custom Branch Predictor can now be simulated on Gem5!
