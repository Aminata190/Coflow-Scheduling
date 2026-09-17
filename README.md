# 🌐 Coflow Scheduling with Predictions
Robust Scheduling Framework with Provable Guarantees

## 📌 **Overview**
A **coflow** is a set of parallel flows belonging to the same distributed application (e.g., MapReduce, Spark). Efficient coflow scheduling significantly reduces job completion time in datacenter networks.

This repository implements a **robust coflow scheduling framework** that:

- Operates in online settings where coflows arrive over time.

- Utilizes predicted flow sizes with bounded error.

- Maintains theoretical performance guarantees relative to clairvoyant benchmarks.

- Includes simulators, generators, and analysis tools to reproduce experiments from the associated research work.

- The framework extends classical algorithms (e.g., Sincronia) to prediction-based scenarios.

## 📁 Repository Structure
```
Coflow-Scheduling/
│
├── Online_Coflow_Scheduling/        # Core scheduling algorithms and simulator
│   ├── ...                          # Network utilities, simulation logic, helper classes
│   ├── exec                         # Main executable files for running simulations
│   ├── BIN/                         # Compiled executables
│   ├── Inputs/                      # Workload generator output (coflows, flows, parameters)
│   ├── Outputs/                     # Simulation results (logs, completion times, metrics)
│   ├── Simulation_Configurations.txt # Simulation configuration
│   └── README.md
│
├── workload_data_generator/         # Facebook trace processing + workload generation
│   ├── ...                          # Workload generator functions
│   ├── instances_generator          # Parameter script for workload creation
│   ├── README.md
│
├── Analysis/                        # Jupyter notebook and plotting scripts
│   ├── Data/                        # Simulation output files used for analysis
│   ├── Functions.py                 # Utility functions for visualization & metrics
│   └── Analysis.ipynb               # Plots and comparative metrics (clairvoyant vs prediction-based)
│
├── requirements.txt                 #Python dependencies for analysis
│
└── README.md                        # Main project documentation
```



## 📣 Citation


```bibtex
@misc{coflow2025,
  title        = {Robust Online Coflow Scheduling from Predictions},
  author       = {Aminata Sangho and Olivier Brun and Balakrishna J. Prabhu},
  year         = {2025},
  howpublished = {\url{https://github.com/Aminata190/Coflow-Scheduling}}
}
```
