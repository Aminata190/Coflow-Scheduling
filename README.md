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
│   ├── ...                          # Simulation helper functions (Network functions, simulations methods, ...)
│   ├── exec                         # Simulation execution main file
│   ├── config                       # Simulation configuration
│   ├── BIN/                         # Executable files
│   ├── Inputs/                      # Files from workload generator
│   ├── Outputs/                     # Output files from simulations
│   └── README.md
│
├── workload_data_generator/         # Facebook traces and workload generation
│   ├── ...                          # Workload generator functions
│   ├── instances_generator          # Generator parameters
│   ├── README.md
│
├── Analysis/                        # Jupyter notebook + plotting scripts
│
└── README.md
```



## 📣 Citation


```bibtex
@misc{coflow2025,
  title        = {Online Coflow Scheduling with Predictions},
  author       = {Aminata Sangho and Olivier Brun and Balakrishna Prabhu},
  year         = {2025},
  howpublished = {\url{https://github.com/Aminata190/Coflow-Scheduling}}
}
```
