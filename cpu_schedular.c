#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOTALRAM 2048
#define CPU1 512
#define CPU2 (TOTALRAM - CPU1)
#define QUANTUM_HIGH_PRIORITY 8
#define QUANTUM_MEDIUM_PRIORITY 16

typedef struct Process {
    char name[10];
    int arrival_time;
    int priority;
    int burst_time;
    int ram_required;
    int cpu_usage;
    int remaining_time;
    struct Process *next;
} Process;

typedef struct {
    Process *front;
    Process *rear;
} Queue;

void initialize_queue(Queue *q) {
    q->front = NULL;
    q->rear = NULL;
}

int is_queue_empty(Queue *q) {
    return q->front == NULL;
}

void enqueue(Queue *q, Process *p, FILE *output_file) {
    if (is_queue_empty(q)) {
        q->front = p;
        q->rear = p;
        p->next = NULL;
    } else {
        q->rear->next = p;
        q->rear = p;
        p->next = NULL;
    }
    fprintf(output_file, "Process %s is queued due to insufficient RAM.\n", p->name);
}

Process *dequeue(Queue *q) {
    if (!is_queue_empty(q)) {
        Process *temp = q->front;
        q->front = q->front->next;
        if (q->front == NULL) {
            q->rear = NULL;
        }
        return temp;
    } else {
        return NULL;
    }
}

void release_ram(Process *p, int *ram_available, FILE *output_file) {
    *ram_available += p->ram_required;
    fprintf(output_file, "Process %s releases RAM.\n", p->name);
}

void print_process_assigned(Process *p, int cpu, FILE *output_file) {
    fprintf(output_file, "Process %s is assigned to CPU-%d.\n", p->name, cpu);
}

void print_process_completed(Process *p, FILE *output_file) {
    fprintf(output_file, "Process %s is completed and terminated.\n", p->name);
}

void sort_processes_by_arrival(Process *processes[], int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (processes[j]->arrival_time > processes[j + 1]->arrival_time) {
                Process *temp = processes[j];
                processes[j] = processes[j + 1];
                processes[j + 1] = temp;
            }
        }
    }
}

void sort_processes_by_burst_time(Process *processes[], int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (processes[j]->burst_time > processes[j + 1]->burst_time) {
                Process *temp = processes[j];
                processes[j] = processes[j + 1];
                processes[j + 1] = temp;
            }
        }
    }
}

int current_time = 0;

int check_ram_availability(Process *process, int ram_available) {
    return process->ram_required <= ram_available;
}

void assign_process(Process *p, int cpu, int *ram_available, FILE *output_file) {
    *ram_available -= p->ram_required;
    print_process_assigned(p, cpu, output_file);
    fprintf(output_file, "Process %s starts at time %d.\n", p->name, current_time);
    current_time += p->burst_time;
    fprintf(output_file, "Process %s completes at time %d.\n", p->name, current_time);
    print_process_completed(p, output_file);
    release_ram(p, ram_available, output_file);
}

void handle_insufficient_ram(Process *p, Queue *waiting_queue, FILE *output_file) {
    fprintf(output_file, "Process %s could not be assigned due to insufficient RAM.\n", p->name);
    enqueue(waiting_queue, p, output_file);
}

void assign_processes(Process *processes[], int n, Queue *waiting_queue, int *ram_available, FILE *output_file) {
    for (int i = 0; i < n; i++) {
        Process *p = processes[i];
        if (check_ram_availability(p, *ram_available)) {
            if (p->priority == 0) {
                assign_process(p, 1, ram_available, output_file);
            } else {
                assign_process(p, 2, ram_available, output_file);
            }
        } else {
            handle_insufficient_ram(p, waiting_queue, output_file);
        }
    }
}

void fcfs_scheduler(Process *processes[], int n, FILE *output_file, Queue *waiting_queue, int *ram_available) {
    printf("CPU-1 que1(priority-0)(FCFS):");
    for (int i = 0; i < n; i++) {
        if (processes[i]->priority == 0) {
            assign_processes(&processes[i], 1, waiting_queue, ram_available, output_file);
            printf("%s-", processes[i]->name);
        }
    }
    printf("\n");
}

void sjf_scheduler(Process *processes[], int n, FILE *output_file, Queue *waiting_queue, int *ram_available) {
    printf("CPU-2 que2(priority-1) (Sjf):");
    Process *priority1_processes[200];
    int priority1_count = 0;

    for (int i = 0; i < n; i++) {
        if (processes[i]->priority == 1) {
            priority1_processes[priority1_count++] = processes[i];
        }
    }

    sort_processes_by_burst_time(priority1_processes, priority1_count);

    for (int i = 0; i < priority1_count; i++) {
        assign_processes(&priority1_processes[i], 1, waiting_queue, ram_available, output_file);
        printf("%s-", priority1_processes[i]->name);
    }
    printf("\n");
}

void rr_scheduler(Process *processes[], int n, int quantum, FILE *output_file, Queue *waiting_queue, int *ram_available) {
    if (quantum == 8) {
        printf("CPU-2 que3(priority-2) (RR-q8):");
    } else {
        printf("CPU-2 que4(priority-3) (RR-q16):");
    }

    int *remaining_burst = (int *)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) {
        remaining_burst[i] = processes[i]->burst_time;
    }

    while (1) {
        int all_completed = 1;

        for (int i = 0; i < n; i++) {
            if (remaining_burst[i] > 0 && (processes[i]->priority == 2 || processes[i]->priority == 3)) {
                all_completed = 0;

                if (remaining_burst[i] <= quantum) {
                    if ((quantum == 8 && processes[i]->priority == 2) ||
                        (quantum == 16 && processes[i]->priority == 3)) {
                        
                        assign_processes(&processes[i], 1, waiting_queue, ram_available, output_file);
                        remaining_burst[i] = 0;
                        
                        release_ram(processes[i], ram_available, output_file);
                        printf("%s-", processes[i]->name);
                    }
                    remaining_burst[i] = 0;
                } else {
                    if ((quantum == 8 && processes[i]->priority == 2) ||
                        (quantum == 16 && processes[i]->priority == 3)) {
                        
                        assign_processes(&processes[i], 1, waiting_queue, ram_available, output_file);
                        
                        enqueue(waiting_queue, processes[i], output_file);
                        fprintf(output_file, "Process %s run until the defined quantum time and is queued again because the process is not completed.\n", processes[i]->name);
                        printf("%s-", processes[i]->name);
                    }
                }
                remaining_burst[i] -= quantum;
            }
        }

        if (all_completed) {
            break;
        }
    }

    free(remaining_burst);
    printf("\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s input.txt\n", argv[0]);
        return 1;
    }

    FILE *input_file = fopen(argv[1], "r");
    if (input_file == NULL) {
        printf("Error opening input file.\n");
        return 1;
    }

    Process *processes[200];
    int process_count = 0;
    char line[100];

    while (fgets(line, sizeof(line), input_file) != NULL) {
        Process *new_process = (Process *)malloc(sizeof(Process));
        
        if (new_process == NULL) {
            printf("Memory allocation failed.\n");
            return 1;
        }

        sscanf(line, "%[^,],%d,%d,%d,%d,%d",
               new_process->name,
               &new_process->arrival_time,
               &new_process->priority,
               &new_process->burst_time,
               &new_process->ram_required,
               &new_process->cpu_usage);

        new_process->remaining_time = new_process->burst_time;
        new_process->next = NULL;

        processes[process_count++] = new_process;
    }

    fclose(input_file);

    FILE *output_file = fopen("output.txt", "w");
    if (output_file == NULL) {
        printf("Error opening output file.\n");
        return 1;
    }

    int ram_available = TOTALRAM;
    Queue waiting_queue;
    initialize_queue(&waiting_queue);

    fcfs_scheduler(processes, process_count, output_file, &waiting_queue, &ram_available);
    sjf_scheduler(processes, process_count, output_file, &waiting_queue, &ram_available);
    rr_scheduler(processes, process_count, QUANTUM_HIGH_PRIORITY, output_file, &waiting_queue, &ram_available);
    rr_scheduler(processes, process_count, QUANTUM_MEDIUM_PRIORITY, output_file, &waiting_queue, &ram_available);

    fclose(output_file);

    for (int i = 0; i < process_count; i++) {
        free(processes[i]);
    }

    return 0;
}
