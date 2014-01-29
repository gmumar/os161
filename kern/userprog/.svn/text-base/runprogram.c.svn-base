/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <thread.h>
#include <curthread.h>
#include <vm.h>
#include <vfs.h>
#include <test.h>
#include <machine/spl.h>

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
int
runprogram(char *progname, char **args, unsigned long nargs)
{

	int s = splhigh();
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result,j;

	//kprintf("%x",progname);

	DEBUG(1,"In run program printing args %s\n",progname);
	//while(args[j]!=NULL){
	//	DEBUG(1,"%s,\n ",*args[j]);
	//	j++;
	//}


	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, &v);
	if (result) {
		splx(s);
		return result;
	}

	/* We should be a new thread. */
	assert(curthread->t_vmspace == NULL);

	/* Create a new address space. */
	curthread->t_vmspace = as_create();
	if (curthread->t_vmspace==NULL) {
		vfs_close(v);
		splx(s);
		return ENOMEM;
	}

	/* Activate it. */
	as_activate(curthread->t_vmspace);

	/* Load the executable. */
	result = load_elf(v, &entrypoint);//have virtual pages with physical pages/addrs which have the segments loaded
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		vfs_close(v);
		splx(s);
		return result;
	}

	/* Done with the file now. */
	//vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(curthread->t_vmspace, &stackptr);
	if (result) {
		/* thread_exit destroys curthread->t_vmspace */
		splx(s);
		return result;
	}

	int i;
	size_t len;
	size_t stackoffset = 0;
	vaddr_t argvptr[nargs+1];

	// Copy each argument onto the stack going downwards
	for (i = 0; i < nargs; i++)
	{
		len = strlen(args[i]) + 1; // add 1 for the terminating NULL
		stackoffset += len;
		argvptr[i] = stackptr - stackoffset;
		copyout(args[i], (userptr_t) argvptr[i], len);
		DEBUG(1,"run: %s\n",args[i]);
	}
	argvptr[nargs] = 0; // terminating NULL pointer for array

	// Create space for the argument pointers
	stackoffset += sizeof(vaddr_t) * (nargs+1);

	// Adjust the stack pointer and align it
	stackptr = stackptr - stackoffset - ((stackptr - stackoffset)%8);

	// Copy the argument pointers onto the stack
	copyout(argvptr, (userptr_t) stackptr, sizeof(vaddr_t) * (nargs+1));

	// Enter user mode with the arguments in place
	for(i=0;i<nargs;i++){
		kfree(args[i]);
	}
	//kfree(progname);
	kfree(args);
	splx(s);

	md_usermode(nargs, (userptr_t) stackptr, stackptr, entrypoint);

	/* Warp to user mode. */

	//md_usermode(0 /*nargs*/,NULL /* userspace addr to arg*/,
	//	    stackptr, entrypoint);
	
	/* md_usermode does not return */

	panic("md_usermode returned\n");
	return EINVAL;
}

