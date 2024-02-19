# You Spin Me Round Robin

The goal of the lab is to implement round robin scheduling when given a workload and the quantum length. We parse the processes file and calcualte the respective waiting and response time.

## Building
The Makefile has he targets set-up, so simply run the make command to build the code.
```shell
make
```

## Running
Insert the process file with the given processes in the right format. The one used was processes.txt. Also, specify the quantum_length for the time slice.
```shell
./rr_scheduler <processes_file> <quantum_length>
```

results 
The average waiting time and the average response time for all the processes that were parsed through in the given input are calculated and outputted.
```shell
Average waiting time: <wait_time>
Average response time: <response_time>
```

## Cleaning up
The code cleans up all the data. Simply remove the object files and associated executables as defined in the Makefile. 
```shell
clean
```
