#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>

#define BUF_SIZE 300
#define SHARED_MEMORY_NAME "/shm_file"
#define FIRST_SEM "/sem1"
#define SECOND_SEM "/sem2"
#define THIRD_SEM "/sem3"

int main() {

	int fd_shm;
	char* shmem;
	char* tmp = (char*)malloc(sizeof(char) * BUF_SIZE);
	char* buf_size = (char*)malloc(sizeof(char) * 10);

	sem_t* sem1 = sem_open(FIRST_SEM, O_CREAT, 0660, 0);
	sem_t* sem2 = sem_open(SECOND_SEM, O_CREAT, 0660, 0);
	sem_t* sem3 = sem_open(THIRD_SEM, O_CREAT, 0660, 0);
	if (sem1 == SEM_FAILED || sem2 == SEM_FAILED || sem3 == SEM_FAILED) {
		perror("ошибка создания семафоров в программ 'a'\n");
		exit(1);
	}

	if (shm_unlink(SHARED_MEMORY_NAME) == -1) {
		perror("ошибка shm_unlink\n");
		exit(1);
	}

	if ((fd_shm = shm_open(SHARED_MEMORY_NAME, O_RDWR | O_CREAT | O_EXCL, 0660)) == -1) {
		perror("ощибка shm_open в программе 'a'\n");
		exit(1);
	}

	if (ftruncate(fd_shm, BUF_SIZE) == -1) {
		perror("ошибка ftruncate в программе 'a'\n");
		exit(-1);
	}

	shmem = (char*)mmap(NULL, BUF_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, fd_shm, 0);

	sprintf(buf_size, "%d", BUF_SIZE);

	char* argv[] = { buf_size, SHARED_MEMORY_NAME, SECOND_SEM, THIRD_SEM, NULL };

  	while (scanf ("%s", tmp)) {

  		pid_t p = fork();
  		if (p == 0) {
  				pid_t p_1 = fork();
  				if (p_1 == 0) {
  					sem_wait(sem1);
  					printf("программа a взяла:\n");
  					if (execve("./b.out", argv, NULL) == -1) {
  						perror("не уладось выаолнить программу 'a'\n");
  					}
  				} else if (p_1 > 0) {
  					sem_wait(sem3);
					if (execve("./c.out", argv, NULL) == -1) {
						perror("не удалось выполнить программу 'a'\n");
					}
  				}
		} else if (p > 0) {
			
			sprintf(shmem, "%s", tmp);
  			sem_post(sem1);
  			sem_wait(sem2);
  			printf("\n");
		}	
  	}
  	
  	sem_unlink(FIRST_SEM);
	sem_unlink(SECOND_SEM);
	sem_unlink(THIRD_SEM);
	sem_close(sem1);
	sem_close(sem2);
	sem_close(sem3);
	close(fd_shm);
}