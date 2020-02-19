
# Opening /dev/Secret
* Owner that writes will become owner of scecret via:

 	#include <sys/socket.h>
	#include <unistd.h>
	// puts proc_ep (pid,uid,gid) on ucred
	int getnucred(endpoint_t proc_ep,  struct ucred * ucred);

	// endpoint_t - process number (defined in /usr/src/include/minix/type.h )
	// struct ucred - (defined in /usr/src/include/sys/ucred.h)
* Writing only if secret is not owned by anyone
	ucred.





