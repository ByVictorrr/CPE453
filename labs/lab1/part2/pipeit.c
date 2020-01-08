#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Launch the program : ls | sort -r > outfile*/

enum Pipes_end{READ, WRITE};

int open_file(const char *file){
	int fd;
	if((fd = open(file, O_CREAT | O_RDWR, 0644)) > 0){
		return fd;
	}else{
		return -1;		
	}
}
void check_status(int status){
	/*
	 WIFIEXITED - return true if exit status normal
	 WEXITSTATUS - returns status of exit process
	 */
	if(WIFEXITED(status) && WEXITSTATUS(status) != EXIT_FAILURE){
		return;
	}else{
		exit(EXIT_FAILURE);
	}
}

int main(){
	
	pid_t p1, p2;
	int fd[2], outfile, status;
	char *prog[2][3] = {
		{"/bin/ls", (char*)0, (char*)0},
		{"/bin/sort", "-r", (char*)0}
	};

	pipe(fd);
	outfile = open_file("outfile");
	if((p1 = fork()) == 0){
		close(fd[READ]);
		dup2(fd[WRITE], STDOUT_FILENO);
		if(execvp(prog[0][0], prog[0]) < 0){
			perror("ls error");
			exit(EXIT_FAILURE);
		}
	}else{
		/* Step 1.2.1 - wait for ls process*/
		wait(&status);
		/* step 1.2.2 - check status exit if not normal*/
		check_status(status);
		if((p2=fork()) == 0){
			close(fd[WRITE]);
			dup2(fd[READ], STDIN_FILENO);
			dup2(outfile, STDOUT_FILENO);
			if(execvp(prog[1][0], prog[1]) < 0){
				perror("sort error");
				exit(EXIT_FAILURE);
			}

		}else{
			close(fd[WRITE]);
			close(fd[READ]);
			/* step 1.2.3 - wait for second process*/
			wait(&status);
			check_status(status);
		}

	}

	close(outfile);

	return 0;
}
