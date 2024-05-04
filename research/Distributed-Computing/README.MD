# Hybrid Mutual Exclusion in Distributed Systems
[OG repo](https://github.com/TIFitis/Distributed-Computing)

All the files are precompiled in the names, masterserver, vc and sk, however you may recompile them from the source codes, with -lpthread appended, please see the image for reference.

## Step 1:

Run the Project.out with <input.dat and >log.dat, to take input from the input.dat file, and output them to the log.dat file.

## Step 2:
Open n new terminals where n is the no. of processes, and execute the client process from each, the processes should be showing how many more processes they are expecting to connect before they all start executing.

## Step3:
After each process outputs the message "All tasks completed...", you may close the masterserver program, along with rest of the terminals.

## Step4:
While executing the process do output to the terminal whenever any event occurs, also the log file gives a detailed overview.

## Input File Format:

Input file must contain n, l, a, m where n is no. of processes, l is inter event delay in secs(integer), a is the ratio of internal and message sending events(double), and m is total no. of events(int). Followed by the topology in the form of adjacency matrix.

Note: Adjacency Matrix has been used as input instead of Adjacency List, as in the samle input format there was no identifier mentioned to indicate where the list of one process ends, and another starts, ie, if Adjacency list is

1 2 3
4 5
1 3 7 8 9 0
