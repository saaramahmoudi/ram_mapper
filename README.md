# RAM Mapper
The mapper_Stratix_IV.cpp contains the mapper which works for Stratix IV architecture. A more general code can be found on mapper_general.cpp, which works with two different BRAM types and LUTRAM. The BRAMs' ratio and LUTRAM ratio is also configurable. Finally, a python script named "best_architecture.py" has been written to find the most efficient architecture with the minimum area possible with the developed algorithm. The following sections in this file guide you on how to compile and test the results by the checker.  

# Compilation and Run 
To compile the mapper_Stratix_IV.cpp, you can simply use the following command. 

```bash
make mapper_Stratix_IV
```

For result check, you can use the following command. This command will execute the mapper. Hence, the mapper_Stratix_IV.txt will be generated, which will be checker input. Checker will automatically be executed using the command, and you can see the code result. 

```bash
make mapper_Stratix_IV_Run
```

# Explore Different Architectures
To compile the general mapper, you can simply type the following command. 

```bash
make mapper_general
```

The object file of the general mapper will have six inputs as follows:

- First BRAM size in bits (1024, 2048, ...)
- First BRAM max-width
- First BRAM ratio (e.g., 10 for BRAM 8k in Startix_IV)
- Second BRAM size in bits (1024, 2048, ...)
- Second BRAM max-width
- Second BRAM ratio (e.g., 300 for BRAM 128k in Startix_IV)
- LUTRAM ratio (1,2,...)

Running the object file with proper inputs will give you a text file named "checker_input". Hence, the result can be seen by running the checker file. 

A command has been provided to explore different architecture by running the python script. The python script will go through four nested loops. The script will look over any combination of available BRAMs to find the best area. It changes the LUTRAM ratio from 1 to 5 and tests every combination of BRAMs with different ratios from 1 to 300.  The script may take some time to complete. Hence, I sent the script's output named "final_result.txt". Some selected architecture within the nested loop will be printed while execution of the script. Therefore, you can see some tried architecture and their area in this file. 

```bash
make mapper_general_Run
```