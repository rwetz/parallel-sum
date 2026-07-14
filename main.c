#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <string.h>

#define MAX_NUMBERS 100000

int load_file(const char *filename, int *numbers, int max) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        return -1;
    }
    int count = 0;
    while (count < max && fscanf(fp, "%d", &numbers[count]) == 1) {
        count++;
    }
    fclose(fp);
    return count;
}

int main(void) {
    int num_procs, file_index;

    //get number of child process
    printf("Enter number of child processes (1, 2, or 4): ");
    if (scanf("%d", &num_procs) != 1 || (num_procs != 1 && num_procs != 2 && num_procs != 4)) {
        fprintf(stderr, "Error: number of processes must be 1, 2, or 4.\n");
        return 1;
    }

    //get file index
    printf("Enter file index (1, 2, or 3): ");
    if (scanf("%d", &file_index) != 1 || file_index < 1 || file_index > 3) {
        fprintf(stderr, "Error: file index must be 1, 2, or 3.\n");
        return 1;
    }

    //build filename
    char filename[32];
    snprintf(filename, sizeof(filename), "file%d.dat", file_index);

    //load numbers
    int *numbers = malloc(MAX_NUMBERS * sizeof(int));
    if (!numbers) {
        perror("malloc");
        return 1;
    }

    int total_count = load_file(filename, numbers, MAX_NUMBERS);
    if (total_count < 0) {
        fprintf(stderr, "Error: could not open '%s'.\n", filename);
        free(numbers);
        return 1;
    }
    printf("\nFile '%s' loaded: %d numbers.\n", filename, total_count);

    //start timer
    struct timeval t_start, t_end;
    gettimeofday(&t_start, NULL);

    //distribute numbers
    int base  = total_count / num_procs;
    int extra = total_count % num_procs;

    //parent reads child writes
    int pipes[4][2]; //4 children max
    for (int i = 0; i < num_procs; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            free(numbers);
            return 1;
        }
    }

    //flush before forking: when stdout is a pipe/file it is fully buffered,
    //and fork() would duplicate the unflushed buffer into every child,
    //re-printing the prompts once per child
    fflush(stdout);

    //fork
    int offset = 0;
    for (int i = 0; i < num_procs; i++) {
        int block_size = base + (i < extra ? 1 : 0);

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            free(numbers);
            return 1;
        }

        if (pid == 0) {
            //child
            for (int j = 0; j < num_procs; j++) {
                close(pipes[j][0]);
                if (j != i)
                    close(pipes[j][1]);
            }

            long long sum = 0;
            for (int k = 0; k < block_size; k++) {
                sum += numbers[offset + k];
            }

            printf("Child %d (PID %d): summed %d numbers [index %d..%d], partial sum = %lld\n",
                   i + 1, getpid(), block_size,
                   offset, offset + block_size - 1, sum);

            write(pipes[i][1], &sum, sizeof(sum));
            close(pipes[i][1]);

            free(numbers);
            exit(0);
        }

        //parent
        close(pipes[i][1]);

        offset += block_size;
    }

    //collect results
    long long grand_total = 0;
    long long child_sum;

    printf("\n--- Results ---\n");
    for (int i = 0; i < num_procs; i++) {
        //read partial sum from child i
        if (read(pipes[i][0], &child_sum, sizeof(child_sum)) != sizeof(child_sum)) {
            fprintf(stderr, "Warning: failed to read result from child %d\n", i + 1);
            child_sum = 0;
        }
        close(pipes[i][0]);

        printf("Child %d result: %lld\n", i + 1, child_sum);
        grand_total += child_sum;
    }

    //wait
    for (int i = 0; i < num_procs; i++) {
        wait(NULL);
    }

    //stop timer
    gettimeofday(&t_end, NULL);
    double elapsed = (t_end.tv_sec - t_start.tv_sec) +
                     (t_end.tv_usec - t_start.tv_usec) / 1e6;

    printf("\nTotal sum of all %d numbers: %lld\n", total_count, grand_total);
    printf("Running time: %.6f seconds\n", elapsed);

    free(numbers);
    return 0;
}
