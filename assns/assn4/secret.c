#include <sys/socket.h>
#include <sys/ucred.h>
#include <minix/type.h>
#include <unistd.h>
// R/W perms consts
#include <minix/const.h>

static typdef enum BOOL{FALSE, TRUE} bool;
#define NOT_OWNED -1

// TODO: If secret size is greater than this return ENOSPC
#define SECRET_SIZE 10000
char secret[SECRET_SIZE] = {0};

// Initalize phony creds 
static struct ucred owner_cred = {NOT_OWNED,NOT_OWNED,NOT_OWNED}; 

/* State var to count number of times the device has been  opened */
PRIVATE int open_count;

/* entry point for this driver*/
PRIVATE struct driver s_dtab = {
	s_name, 			/* current device's name */
	s_do_open, 			/* open request, init device */
	s_do_close,			/* release device */
	s_do_ioctl,			/* ioctl */
	s_prepare, 			/* prepare for I/O */
	s_transfer, 		/* do I/O read or write*/
	nop_cleanup, 		/* nothing to clean up */
	nop_geometry, 		/* no geometry*/
	nop_alarm,			/* no alarm */
	nop_cancel,			/* nop cancel */ 
	nop_select, 		/* not really sure */
	s_ioctl,			/* cath-all for unrecognized commands  and ioctl*/
	do_nop				/* not sure for dr_hw_int*/
};

/* represents /dev/Secret */
PRIVATE struct device secret_device;

/*============================================================
						main
*==============================================================*/

int main(){
	/* SEF local startup */
	sef_local_startup();

	/* Call the generic recieve loop */
	driver_task(&s_dtab, DRIVER_STD);

	return OK;
}

/*============== DRIVER FUNCTIONS======================*/
/*============================================================
						s_name
*==============================================================*/
PRIVATE char *s_name(){
	return "Secret";
}

/*============================================================
						s_do_open
*==============================================================*/

/* Helper function 
	Description: checking the perms are correct of open
*/
PRIVATE bool isPerms(int perms){
	switch (perms)
	{
	case R_BIT:
		return TRUE;
		break;
	case W_BIT:
		return TRUE;
		break;
	default:
		// could be WR (dont want this)
		return FALSE;
		break;
	}
	return FALSE;
}

/* Returns{
	EACCES - if other permission other than RONLY or WRONLY	
			Or if not owner 
	FALSE - any other error
}
*/
PRIVATE int s_do_open(struct driver *dp, message *m_ptr){
	endpoint_t proc_endpt = getpid();
	// step 0 - check permission given
	if(!isPerms(m_ptr->COUNT)){
		return EACCES; 
	}
	/*step 1 - check if proc_endpt=current.pid
		DO this if empty | owner is proc 
	*/
	// if owned 
	if(!(owner_cred=={NOT_OWNED,NOT_OWNED,NOT_OWNED})){
		// and if proc_enpt is not the owner
		if(!owner_cred.pid==proc_endpt){
			return EACCES;
		}
	}

	open_count++;
	return OK;
}


/*============================================================
				  	 s_prepare 
*==============================================================*/
PRIVATE struct device *s_prepare(int dev){
	// ASK : what is base and size of a partion?
	s_device.dev_base.lo=0;
	s_device.dev_base.hi=0;
	s_device.dev_base.dv_size.lo=strlen(secret);
	s_device.dev_base.dv_size.hi=0;
	return &s_device;
}

/*============================================================
					s_do_read
*==============================================================*/
PRIVATE int s_do_read(
	u64_t position,
	iovect_t *iov,
	unsigned nr_req
){
	int ret;

	ret=sys_safecopyto(proc_nr, iov->iov_addr, 0,
							(vir_bytes)(secret+position.lo),
							bytes,D)
	iov->iov_size-=bytes;
	return ret;
}
/*============================================================
					s_do_write
*==============================================================*/
PRIVATE int s_do_wrtie(
	u64_t position,
	iovect_t *iov,
	unsigned nr_req
){
	int ret;

	ret=sys_safecopyfrom(proc_nr, iov->iov_addr, 0,
							(vir_bytes)(secret+position.lo),
							bytes,D)
	iov->iov_size-=bytes;
	return ret;
}
	

/*============================================================
						s_transfer(for R/W in opcode)
*==============================================================*/
/* Assumption: that file is open*/
PRIVATE int s_transfer(
	int proc_nr,
	int opcode,
	u64_t position, // ASK : what is position(seek mode maybe)???
	iovect_t *iov,
	unsigned nr_req
){
	int ret;

	// TODDO NOT sure what it does
	switch (opcode)
	{

	case DEV_GATHER_S: // READ
		ret=s_do_read();
		break;
	case DEV_SCATTER_S: // WRITE
		ret=s_do_write();
		break;	
	default:
		return EINVAL;
	}

	// Happend in write (change ownership)
	if(getnucred(proc_endpt, &owner_cred)){
		return FALSE;
	}
}

/*============== END DRIVER ======================*/


/*============== SEF FNS================================
	DESCRIPTION{
	Every system service (driver) must call sef_startup() at 
	startup time to handle init. And sef_reieve() when recieving a message

	For registering callbacks to provide handlers for each particular type of event such as:

		Initalization: triggered by init message sent by reincarnation Server(RS) when a server is started
		Ping: triggered by keep-a-live msg send by the RS periodically to check the status of system servicce
		Live Update: triggerd by live update message sent by RS when an update is avail for particular system
}
/*
						sef_local_startup
DESCRIPTION
*==============================================================*/
/*============================================================
						sef_local_startup
*==============================================================*/
PRIVATE void sef_local_startup(){
	/* Register init callbacks to be called by RS*/
	sef_setcb_init_fresh(seef_cb_init_fresh);
	sef_setcb_init_lu(seef_cb_init_fresh);
	sef_setcb_init_restart(seef_cb_init_fresh);

	/* Register live callback (for diff events)*/
	sef_setcb_lu_state_save(sef_cb_lu_state_save);
	
}

/*============================================================
						sef_cb_lu_state_save
DESCRIPTION:{
	Used for live event RS might give us
}
RETURNS:{
	EAGIN - if no more spots in ds
	OK - if init went ok
}
*==============================================================*/
PRIVATE int sef_cb_lu_state_save(int state, int flags){
	/* save the state of open_counter*/
	if(ds_publish_u32("open_counter",open_counter, DSF_OVERWRITE)){
		return EAGIN;
	}
	return OK;
}


/*============================================================
						sef_cb_init_fresh
DESCRIPTION:{
		Used to set sef_cbs.sef_cb_init_fresh, which is used in
	process_init(lib/libsys/sef_init.c), which is used in do_sef_rs_init 
	and do_sef_init_request.
}
RETURNS:{
	ESRCH - if not able to restore open_count(i.e "open_count" DNE)
	OK - if init went ok
}
*==============================================================*/
PRIVATE int sef_cb_init_fresh(int type, sef_init_info_t *info){
	/*  Initalize the secret driver. */
	bool do_announce_driver = TRUE;

	open_counter = 0;
	switch(type){
		/* init fresh */
		case SEF_INIT_FRESH:
			break;
		/* init after live update */
		case SEF_INIT_LU:
			/* restores the state */
			if(lu_state_restore()==ESRCH){
				return ESRCH;
			}
			do_announce_driver=FALSE;
			printf("%Hey, I'm a new version!\n", HELLO_MESSAGE);
			break;
		case SEF_INIT_RESTART:
			printf("%Hey, I've just been restarted\n", HELLO_MESSAGE);
			break;

	}
	if (do_annoounce_driver){
		/* TODO: dont really understand(ASK nico)*/
		driver_announce();
	}

	/* init went sucessfully*/
	return OK;
}

/*============================================================
						lu_state_restore
DESCRIPTION:{
	restores the state of open count 
}
RETURNS: {
	ESRCH - {if no variable named "open_counter"}
	OK - if every thing went ok
}
*==============================================================*/
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

/*============== ENDOF SEF==============================*/



























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

