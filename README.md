# parallel-sum

A C program that distributes integer summation across 1, 2, or 4 child processes using `fork()` and pipes, then aggregates partial sums in the parent process. Includes wall-clock timing to compare performance across process counts.

## Platform
**Linux only.** Uses POSIX APIs (`fork`, `pipe`, `unistd.h`) not available on Windows or macOS without compatibility layers.

## Usage
Compile and run:
```bash
gcc -o parallel-sum main.c
./parallel-sum
```
You will be prompted to enter the number of child processes (1, 2, or 4) and a file index (1–3) corresponding to `file1.dat`, `file2.dat`, or `file3.dat` — plain text files with one integer per line.

## How It Works
- The parent loads integers from the selected file into memory
- Numbers are split into equal chunks and each child receives an offset/block size
- Each child computes a partial sum and writes it back to the parent via a pipe
- The parent aggregates results and prints the total and elapsed time