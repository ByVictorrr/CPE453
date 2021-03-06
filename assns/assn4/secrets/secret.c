#include <minix/drivers.h>
#include <minix/driver.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <minix/ds.h>
#include <sys/ioctl.h>
#include <sys/ucred.h>
#include <unistd.h>
#include <minix/com.h>
#include <minix/const.h>
#include <string.h>


#define NOT_OWNED -1
#define SECRET_SIZE 6000
#define TRUE 1
#define FALSE 0 
#define O_RDONLY 04
#define O_WRONLY 02
#define O_RDWR 06


char secret[SECRET_SIZE] = {'\0'};
int is_reading=FALSE;

PRIVATE uid_t owner;

PRIVATE int open_counter;
PRIVATE struct device s_device;

/* helper function*/
FORWARD _PROTOTYPE( int isFlag, (int flag )); 
FORWARD _PROTOTYPE( void clear_secret, (char *message, int size) ); 
FORWARD _PROTOTYPE( int is_not_owned, (uid_t owner) ) ;
FORWARD _PROTOTYPE( void rst_owner, (uid_t * owner) );
FORWARD _PROTOTYPE( int is_cred_same, (uid_t own, uid_t curr) );


FORWARD _PROTOTYPE( char * s_name, (void) );
FORWARD _PROTOTYPE( int s_do_ioctl, (struct driver *dp, message *m_ptr) );
FORWARD _PROTOTYPE( int s_do_open, (struct driver *dp, message *m_ptr) );
FORWARD _PROTOTYPE( int s_do_close, (struct driver *dp, message *m_ptr) );
FORWARD _PROTOTYPE( struct device * s_prepare, (int dev) );
FORWARD _PROTOTYPE( int s_transfer, (
	int proc_nr,
	int opcode,
	u64_t position, 
	iovec_t *iov,
	unsigned nr_req) 
	);

FORWARD _PROTOTYPE( int sef_cb_lu_state_save, (int state) );
FORWARD _PROTOTYPE( int sef_cb_init_fresh, (int type, sef_init_info_t *info) );
FORWARD _PROTOTYPE( int lu_state_restore, (void) );
FORWARD _PROTOTYPE( void sef_local_startup, (void) );





/* entry point for this driver*/
PRIVATE struct driver s_dtab = {
	s_name, 			/* current device's name */
	s_do_open, 			/* open request, init device */
	s_do_close,			/* release device */
	s_do_ioctl,			/* ioctl */
	s_prepare, 			/* prepare for I/O */
	s_transfer, 			/* do I/O read or write*/
	nop_cleanup, 			/* nothing to clean up */
	NULL, 				/* no geometry*/
	nop_alarm,			/* no alarm */
	nop_cancel,			/* nop cancel */ 
	nop_select, 			/* not really sure */
	NULL,
	do_nop				/* not sure for dr_hw_int*/
};



PUBLIC int main(){
	printf("hello world\n");
	rst_owner(&owner);
	/* SEF local startup */
	sef_local_startup();


	/* Call the generic recieve loop */
	driver_task(&s_dtab, DRIVER_STD);

	return OK;
}


PRIVATE char *s_name(){
	return "Secret";
}

PRIVATE int s_do_ioctl(dp, m)
	struct driver *dp;
	message *m;
{
	int res;
	uid_t grantee;
	if( m->REQUEST== SSGRANT ){
		res=sys_safecopyfrom(
			m->IO_ENDPT,
			(vir_bytes)m->IO_GRANT, 
			0,
			(vir_bytes)&grantee, 
			sizeof(grantee),
			D);

		owner = grantee;
		return OK;
	}
	printf("command for ioctl doesnt exist");
	return ENOTTY;

}

PRIVATE int s_do_open(struct driver *dp, message *m_ptr){


	struct ucred curr;
	int flag, is_empty=FALSE;

	flag=(m_ptr->COUNT & (R_BIT|W_BIT)); 


	/* Step 1 - check to see if the flag is only R or W*/
	if(!isFlag(flag)){
		printf("open() was opened not in read or write mode\n"); 
		return EACCES; 
	}
	/*Step 2 - get the processes endpoint(uid, gid, pid)	*/
	if(getnucred(m_ptr->IO_ENDPT, &curr)){
		printf("getnucred failed\n");
		return errno;
	}

	printf("owner uid: %d\n", owner);
	printf("curr uid: %d\n", curr.uid);
	/*Step 3 - first check to see if owner is enmpty;*/
	if(is_not_owned(owner)){
		is_empty=TRUE;
		owner=curr.uid;
	/*Step 4 - this means that owner isnt full*/
	}else if(!is_cred_same(owner, curr.uid)){
		return EACCES;
	}
	/* step 5 - see if there request is R/W
		Assumption only if we make it here the 
		Owner is empty or the curr is owner
	*/
	switch(flag){
		case O_RDONLY:
			/*ask about this*/
			is_reading=TRUE;
			/* only owner can read the message(assumption ony)*/
			if (is_empty){
				printf("Cant read it because this file doesnt contain anything\n");
			}else{
				/* if the secret is full*/
				printf("Opened in read mode where the secret is full\n");
			}
			break;
		case O_WRONLY:
			/*only if empty you can write*/
			if(is_empty){
				/*then one can write*/
				printf("Empty secret in WRITE only mode\n");
			}else{
				/* cant write because its full*/
				printf("Secret is full cannot write anything\n");
				return ENOSPC;
			}
	}
	open_counter++;
	return OK;
}

PRIVATE int s_do_close(struct driver *dp, message *m_ptr){

	if(open_counter < 1){
		panic("closed too often\n");
	}
	/*if this is the last open file descriptor then entry the message*/
	if(open_counter==1 && is_reading){
		printf("Closed last file descriptor, clearing the secret\n");
		clear_secret(secret, SECRET_SIZE);
		is_reading=FALSE;
		owner=NOT_OWNED;
	}

	printf("open fds: %d\n", --open_counter);
	return OK;
}
PRIVATE struct device *s_prepare(int dev){
	s_device.dv_base.lo=0;
	s_device.dv_base.hi=0;
	s_device.dv_size.lo=strlen(secret);
	s_device.dv_size.hi=0;
	return &s_device;
}

	

/* Assumption: that file is open(that is secret is empty or owner reading)*/
PRIVATE int s_transfer(
	int proc_nr,
	int opcode,
	u64_t position, 
	iovec_t *iov,
	unsigned nr_req
){
	int ret, bytes;
	struct ucred curr; 



	/* step 3 - is it a read or write request? */ 
	switch (opcode)
	{
	/* READ*/
	case DEV_GATHER_S: 		
			/*position - where we are in the /dev/Secret file*/

			bytes = strlen(secret) - position.lo < iov->iov_size ?
					strlen(secret) - position.lo  : iov->iov_size; 

			/* step 2 - if less than 0 bytes give a EOF */ 
			if(bytes <=0 ) {
				return OK;
			}

			
			/* step 3.1 - send the data to user at iov_add */
			ret=sys_safecopyto(
				proc_nr, 
				(vir_bytes)iov->iov_addr, 
				(vir_bytes)0,
				(vir_bytes)(secret+position.lo),
				bytes, 
				D);
		/* step 3.2 - if didnt didnt read all of it*/
		iov->iov_size-=bytes;	
		/* step 3.3 - shift, then check to see if read all of it */ 
		break;
	case DEV_SCATTER_S: /* WRITE*/
		/* step 3.1 - write data from iov_add to secret*/
		/* step 1 - if the size asked for is greater than our size*/
		/*position - where we are in the /dev/Secret file*/
		bytes = SECRET_SIZE - position.lo < iov->iov_size ?
			SECRET_SIZE - position.lo  : iov->iov_size; 
	
		if(iov->iov_size > SECRET_SIZE){
			return ENOSPC;
		}
			ret=sys_safecopyfrom(
					proc_nr, 
					iov->iov_addr,
					(vir_bytes)0, 
					(vir_bytes)(secret+position.lo),
					bytes, 
					D);
		iov->iov_size-=bytes;	
		break;	
	default:
		printf("not read/write\n");
		return EINVAL;
	}
	return ret;
}



PRIVATE void sef_local_startup(){
	/* Register init callbacks to be called by RS*/
	sef_setcb_init_fresh(sef_cb_init_fresh);
	sef_setcb_init_lu(sef_cb_init_fresh);
	sef_setcb_init_restart(sef_cb_init_fresh);

	/* Register live callback (for diff events)*/
	sef_setcb_lu_state_save(sef_cb_lu_state_save);

	sef_startup();
	
}

PRIVATE int sef_cb_lu_state_save(int state){

	/* save the state of open_counter*/
	if(ds_publish_u32("open_counter",open_counter, DSF_OVERWRITE)){
		return EAGAIN;
	}
	return OK;
}


PRIVATE int sef_cb_init_fresh(int type, sef_init_info_t *info){
	/*  Initalize the secret driver. */
	int do_announce_driver = TRUE;

	open_counter = 0;
	switch(type){
		/* init fresh */
		case SEF_INIT_FRESH:
			printf("fresh start\n"); 
			break;
		/* init after live update */
		case SEF_INIT_LU:
			/* restores the state */
			if(lu_state_restore()==ESRCH){
				return ESRCH;
			}
			do_announce_driver=FALSE;
			printf("Hey, I'm a new version!\n");
			printf("open fd's restored at %d= !\n", open_counter);
			break;
		case SEF_INIT_RESTART:
			printf("Hey, I've just been restarted\n");
			break;

	}
	if (do_announce_driver){
		driver_announce();
	}

	/* init went sucessfully*/
	return OK;
}

PRIVATE int lu_state_restore(){
	/* restore the state */
	u32_t value;
	/* Retrieve an unsigned int */
	if(ds_retrieve_u32("open_counter", &value) == ESRCH){
		return ESRCH;
	}
	/* delete the variable stored in ds*/
	if(ds_delete_u32("open_counter")==ESRCH){
		return ESRCH;
	}
	open_counter = (int)value;

	return OK;
}


PRIVATE int isFlag(int flags){
	switch (flags)
	{
	case O_WRONLY:
		return TRUE;
		break;
	case O_RDONLY:
		return TRUE;
		break;
	default:
		return FALSE;
		break;
	}
	return FALSE;
}

PRIVATE void clear_secret(char *message, int size){
	int i;
	for(i=0; i < size; i++){
		message[i] = '\0';
	}
}
PRIVATE void rst_owner(uid_t *owner){
	*owner=NOT_OWNED;
}

PRIVATE int is_not_owned(uid_t owner){
		return owner==NOT_OWNED;
}
PRIVATE int is_cred_same(uid_t owner, uid_t curr){
		return owner==curr;
	}	

