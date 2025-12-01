 # Coflow workload generator

A command-line tool for generating synthetic Coflow workloads, supporting multiple distributions, topologies, and contention models.

 Command Line Argument           | Input Value Range  
:-------------:| :-----:
 `NUM_COFLOWS` | `Integer` value > 0 Number of Coflows to generate.
 `ALPHA`      |   `Integer` value in \{1,...,P\} Parameter related to workload modeling
 `LOAD_FACTOR`     |    `Float` value in (0,1)  Overall load factor for the system.
 `INTRA_COFLOW_CONTENTION`      |    `Float` value in [0,1] Controls intra-coflow contention
 `SOURCE_NUM_DIST`      |    `Char` value U (unfiform dist) or Z (zipf dist) or FB( Facebook Trace)
 `DESTINATION_DATA_DIST`      |    `Char` value U (unfiform dist) or N (normal dist) 
 `TOP_ID`      |   `Integer` value > 0 .Network topology ID.
 `OUTDIR`      |   `string` . Output directory for generated files
 `NUM_PORTS`   |   `Integer` value = P Half Number of network ports.
 `RELEASED_TIME_LAW`      |   `Char` G or PS or P. release time distribution law  
 `RELEASED_TIME_LAW_PARAM`      |   `Integer` release time distribution law parameter


### Running the Generator
To generate workload instances:
1. Set the desired values for each argument.
2. Set the number of instances
3. Run the generator with:

    ```
    ./instances_generator
    ```

### Further Information
For full options, detailed examples, and troubleshooting, see the official documentation 'https://github.com/sincronia-coflow/workload-generator'