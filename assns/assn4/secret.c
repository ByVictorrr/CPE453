#include <sys/socket.h>
#includ <sys/ucred.h>
#include <minix/type.h>
#include <unistd.h>

static typdef enum BOOL{FALSE, TRUE} bool;
static typdef enum REQUEST{READ,WRITE} req_t;


// TODO: If secret size is greater than this return ENOSPC
#define SECRET_SIZE 10000
char secret[SECRET_SIZE] = {0};


// QUESTIONS: is the number of opens on a file an attribute of inode struct




/* HOW TO READ or WRITE FROM a variable of another proc

   	* SYS task help from function below

int sys safecopyfrom (
endpoint t source, /* source process */
cp grant id t grant, /* source buffer */
vir bytes grant offset, /* offset in source buffer (for block devs) */
vir bytes my address, /* virtual address of destination buffer */
size t bytes, /* bytes to copy */
int my seg /* memory segment (It’s ’D’ :-) */
);

int sys safecopyto (
endpoint t source, /* destination process */
cp grant id t grant, /* destination buffer */
vir bytes grant offset, /* offset in destination buffer (for block devs) */
vir bytes my address, /* virtual address of source buffer */
size t bytes, /* bytes to copy */
int my seg /* memory segment (It’s ’D’ :-) */
);
*/

/* when_open:{
   1. After a write
   	Assign owned process as owner
   2.
}
assumptions:{
	1. (Current process owned the secret prev or is "related" to it
		"related": the parent of the process wrote to it)
		OR
	2. (The secret is empty)
}
*/
// For owned process gid,pid,uid
struct ucred owned;
static bool is_empty = TRUE;

bool when_open(req_t type){
	switch(type){
		/* request is a read
		   	dont change id
			set flag empty
		*/
		case READ:
			is_empty=TRUE;
			break;
		/* request is a write
		   	suceeds if if /dev/Secret not owned
			by another process( meaning its empty)
		*/
		case WRITE
			// TODO : see how to determine if /dev/Secret is empty
			if (is_empty){
				owned.pid=getpid();
				owned.uid=getuid();
				owned.gid=getgid();
				// TODO : write secret in

				is_empty=FALSE;
			}else{
				return FALSE;
			}
			break;
		default:
	}
}

bool in_close(){
	/* calls close to file (closes fd)
		needs to reset ucred var in in_open
		needs to reset bool empty flag
	*/

	// No process has negative pid, gid, uid
	owned.gid=-1;
	owned.pid=-1;
	owned.uid=-1;
	// TODO : not fully because other devices might have fd's open
	is_empty = TRUE;

}



/*
   description{
   	called when try to open this file
	if (R-W access)
		return EACESS
	else if (R | W access)
		call with_open()

   }
*/
bool try_open(){

	// TODO : find out a way to determine is full (maybe use semaphore)
	if(full)
 		// give device this error ENOSPC

	if( getpid() != owned.pid){
		// return  EACCES
	}
}
