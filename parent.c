#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
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
pid_t child_pid;

void sigusr1_handler(int sig) {
    printf("Результат: %d\n", shm_ptr->sum);
}

int main() {
    int shm_fd;
    char input_buffer[1024];
    char *token;
    
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }
    
    shm_ptr = mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    
    memset(shm_ptr, 0, sizeof(shared_data));
    shm_ptr->ready = 0;
    shm_ptr->processed = 0;
    
    signal(SIGUSR1, sigusr1_handler);
    
    printf("Введите имя файла для результатов: ");
    fgets(shm_ptr->filename, FILENAME_SIZE, stdin);
    shm_ptr->filename[strcspn(shm_ptr->filename, "\n")] = 0;
    
    child_pid = fork();
    if (child_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    
    if (child_pid == 0) {
        execl("./child", "child", NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    } 
    else {
        printf("Дочерний процесс создан с PID: %d\n", child_pid);
        sleep(1);
        
        while (1) {
            printf("\nВведите числа через пробел (или 'exit'): ");
            if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
                break;
            }
            
            input_buffer[strcspn(input_buffer, "\n")] = 0;
            if (strcmp(input_buffer, "exit") == 0) {
                break;
            }
            
            while (shm_ptr->ready == 1) {
                usleep(10000);
            }
            
            shm_ptr->count = 0;
            token = strtok(input_buffer, " ");
            while (token != NULL && shm_ptr->count < 100) {
                shm_ptr->numbers[shm_ptr->count] = atoi(token);
                shm_ptr->count++;
                token = strtok(NULL, " ");
            }
            
            if (shm_ptr->count == 0) {
                printf("Нет чисел\n");
                continue;
            }
            
            shm_ptr->ready = 1;
            
            if (kill(child_pid, SIGUSR1) == -1) {
                perror("kill");
                break;
            }
            
            while (shm_ptr->ready == 1) {
                usleep(100000);
            }
        }
        
        kill(child_pid, SIGTERM);
        wait(NULL);
        
        munmap(shm_ptr, SHM_SIZE);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        
        printf("Завершение\n");
    }
    
    return 0;
}