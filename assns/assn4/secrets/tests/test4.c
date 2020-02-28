#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>


#define MSG "Hello, World\n"

int main(int argc, char *argv[]){
		int fd1,fd2, res, uid;
		char buff[1000]={0};
		/* Test 1*/
		char *msg=MSG;
		fd1=open("/dev/secret", O_WRONLY);
		fd2=open("/dev/secret", O_RDONLY);
		printf("opening... fd1=%d\n",fd1);
		res=write(fd1, msg, strlen(msg));

		printf("writing... res=%d\n", res);

		execlp("service", "service update /dev/") 


		res=close(fd1);

		return 0;
}
