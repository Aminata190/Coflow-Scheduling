 # Coflow Scheduling

The scheduling component builds two executables:

- `exec_grb`: uses the Gurobi solver to compute optimal schedules.  
- `exec_clp`: uses the CLP solver to compute approximate schedules.

### 1.Compilation

Ensure that:
- Gurobi is installed and `GUROBI_HOME` in `Makefile.gurobi` points to a valid installation.
- CLP and COIN-OR headers/libraries are installed (e.g., `libClp`, `libCoinUtils`).
- The `BIN` directory exists (or will be created).

From the project root, run:
```
    make 
```


This will compile and place `exec_grb` and `exec_clp` into the `./BIN/` directory.

### 2. Execution

Basic execution example for the CLP-based simulator:
```
    ./BIN/exec_clp -o -u -c x1 -a x2 -b x3 inputDirName outputDirName
```


Where:

| Argument |Description                                                                 |
|:--------:|-----------------------------------------------------------------------------|
| `-o`     | Run the simulation in online mode.                                         |
| `-u`     | Run the online mode simulation with anticipate intervalle                  |
| `x1`     | Scheduling policy: `-o` (optimal), `-c` (clairvoyant Sincronia), `-n` (prediction), `-r` (round robin). |
| `x2`     | Index of the first instance (file) to be processed.                        |
| `x3`     | Index of the last instance (file) to be processed.                         |
| `inputDirName`  | Directory containing input workload instances.                      |
| `outputDirName` | Directory where results and logs will be written.                   |

Adjust the flags and indices according to the instances and policies you wish to test.

---

## 3. Requirements

- A Unix-like environment (Linux, macOS, or WSL).
- C++ compiler (e.g., `g++`).
- Gurobi (licensed installation) for `exec_grb`.
- CLP / COIN-OR libraries for `exec_clp`.

---

## 4. Notes

- Make sure the `BIN` directory is writable.
- To clean intermediate and log files, use the `clean` target from the provided Makefile.



