#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

#define SHM_NAME "/lab3_shm"
#define SHM_SIZE 4096
#define FILENAME_SIZE 256

typedef struct {
    char filename[FILENAME_SIZE];
    int numbers[100];
    int count;
    int sum;
    int ready;
    int processed;
} shared_data;

shared_data *shm_ptr;

void sigusr1_handler(int sig) {
    int i;
    
    if (shm_ptr->ready == 1) {
        shm_ptr->sum = 0;
        for (i = 0; i < shm_ptr->count; i++) {
            shm_ptr->sum += shm_ptr->numbers[i];
        }
        
        FILE *f = fopen(shm_ptr->filename, "a");
        if (f == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
        
        fprintf(f, "Сумма: %d\n", shm_ptr->sum);
        fclose(f);
        
        shm_ptr->ready = 0;
        
        kill(getppid(), SIGUSR1);
    }
}

void sigterm_handler(int sig) {
    munmap(shm_ptr, SHM_SIZE);
    exit(EXIT_SUCCESS);
}

int main() {
    int shm_fd;
    
    shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    
    shm_ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    
    signal(SIGUSR1, sigusr1_handler);
    signal(SIGTERM, sigterm_handler);
    
    FILE *f = fopen(shm_ptr->filename, "w");
    if (f != NULL) {
        fclose(f);
    }
    
    while (1) {
        pause();
    }
    
    return 0;
}