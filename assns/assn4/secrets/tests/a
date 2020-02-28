#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define MSG "HI BUDDY"

int main(){
		int fd1,fd2, res;
		char buff[1000]={0};
		/* Test 1*/
		char *msg=MSG;
		fd1=open("/dev/secret", O_WRONLY);
		printf("opening... fd1=%d\n",fd1);
		res=write(fd1, msg, strlen(msg));
		printf("writing... res=%d\n", res);

		fd2=open("/dev/secret", O_RDONLY);
		printf("opening... fd2=%d\n",fd2);
		
		res=read(fd2, buff, 3);
		printf("reading... res=%d, buff=%s\n", res, buff);

		res=read(fd2, buff, 2);
		printf("reading... res=%d, buff=%s\n", res, buff);

		res=read(fd2, buff, 4);
		printf("reading... res=%d, buff=%s\n", res, buff);

		res=read(fd2, buff, 4);
		printf("reading... res=%d, buff=%s\n", res, buff);


		res=close(fd1);
		printf("closing... res=%d\n", res);
		res=close(fd2);
		printf("closing... res=%d\n", res);
		/*test 2*/




}
