#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

const int ARG_MAX = 2097;
int main()
{
	pid_t pid;
	int rv;
	int count;
	char *str = "привет\n";
	char command[ARG_MAX]; 

	//char* command = (char*)malloc(ARG_MAX);
	char* file = (char*)malloc(ARG_MAX);

	switch (pid = fork()) {
		case -1: {
			perror("fork"); //~ произошла ошибка~
			exit(1); // ~выход из родительского процесса~
		}
		default: {

			case 0: {
				//~ Потомок~
				scanf("%s\n%s", command, file);
				FILE* my = freopen(file, "rw", stdin); // используется для связи существующего потока с новым файлом
				count = fwrite(&str, sizeof(char), 10, my);
				printf("~~~~~~~~ %lu  %d.\n", (unsigned long)count, (my) == 0); // почему fclose приводит к беде // пытаюсь закрыть стандартный поток вывода а exec не может завершиться
				rv = execlp(command, command, NULL); // заменяет текущий образ процесса новым образом
				// printf("~~~~~~~~ %lu  %d.\n", (unsigned long)count, fclose(my) == 0); // почему fclose приводит к беде // пытаюсь закрыть стандартный поток вывода а exec не может завершиться
				fclose(my);
				_exit(rv);
			}

			//~Родитель~
			waitpid(pid, &rv, 0);
			exit(WEXITSTATUS(rv));
		}
	}
	free(file);
	free(command);
}



// #include <errno.h>
// #include <malloc.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <sys/types.h>
// #include <sys/wait.h>
// #include <unistd.h>

// const int ARG_MAX = 2097152;
// int main()
// {
//     pid_t pid;
//     int rv;
//     char* command = (char*)malloc(ARG_MAX);
//     char* file = (char*)malloc(ARG_MAX);

//     switch (pid = fork()) {
// 	    case -1: {
// 	        perror("fork"); //~ произошла ошибка~
// 	        exit(1); // ~выход из родительского процесса~
// 	    }
// 	    default: {

// 		    case 0: {
// 		        //~ Потомок~
// 		        scanf("%s\n%s", command, file);
// 		        FILE* my = freopen(file, "r", stdin);
// 		        rv = execlp(command, command, NULL);
// 		        fclose(my);
// 		        _exit(rv);
// 		    }

// 	        //~Родитель~
// 	        waitpid(pid, &rv, 0);
// 	        exit(WEXITSTATUS(rv));
// 	    }
//     }
//     free(file);
//     free(command);
// }