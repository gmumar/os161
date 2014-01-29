#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <machine/pcb.h>
#include <machine/spl.h>
#include <machine/trapframe.h>
#include <kern/callno.h>
#include <syscall.h>
#include <synch.h>
#include <test.h>

#include <thread.h>
#include <addrspace.h>

/*
 * System call handler.
 *
 * A pointer to the trapframe created during exception entry (in
 * exception.S) is passed in.
 *
 * The calling conventions for syscalls are as follows: Like ordinary
 * function calls, the first 4 32-bit arguments are passed in the 4
 * argument registers a0-a3. In addition, the system call number is
 * passed in the v0 register.
 *
 * On successful return, the return value is passed back in the v0
 * register, like an ordinary function call, and the a3 register is
 * also set to 0 to indicate success.
 *
 * On an error return, the error code is passed back in the v0
 * register, and the a3 register is set to 1 to indicate failure.
 * (Userlevel code takes care of storing the error code in errno and
 * returning the value -1 from the actual userlevel syscall function.
 * See src/lib/libc/syscalls.S and related files.)
 *
 * Upon syscall return the program counter stored in the trapframe
 * must be incremented by one instruction; otherwise the exception
 * return code will restart the "syscall" instruction and the system
 * call will repeat forever.
 *
 * Since none of the OS/161 system calls have more than 4 arguments,
 * there should be no need to fetch additional arguments from the
 * user-level stack.
 *
 * Watch out: if you make system calls that have 64-bit quantities as
 * arguments, they will get passed in pairs of registers, and not
 * necessarily in the way you expect. We recommend you don't do it.
 * (In fact, we recommend you don't use 64-bit quantities at all. See
 * arch/mips/include/types.h.)
 */

#define MAX_PIDS 320

struct thread *pid_array[MAX_PIDS];
int exit_array[MAX_PIDS];
int interest_array[MAX_PIDS];
int parent_array[MAX_PIDS];
int errsuperflag;

struct cv *wait_array[MAX_PIDS];
struct lock *wait_lock[MAX_PIDS];

extern struct thread *curthread;

extern struct lock *lock;
extern struct cv *read;

//extern int errno;

void
mips_syscall(struct trapframe *tf)
{
	int callno;
	int32_t retval;
	int err;
	int s,act;
	char *d;

	//convert user level pointers to user level pointers
	assert(curspl==0);

	callno = tf->tf_v0;

	/*
	 * Initialize retval to 0. Many of the system calls don't
	 * really return a value, just 0 for success and -1 on
	 * error. Since retval is the value returned on success,
	 * initialize it to 0 by default; thus it's not necessary to
	 * deal with it except for calls that return other values, 
	 * like write.
	 */

	retval = 0;

	switch (callno) {
		case SYS_reboot:
			err = sys_reboot(tf->tf_a0);
			break;
			// syscall.h has all the callno numbers for differnt syscalls
		case SYS_fork:
			err = sysfork(&retval,tf);
			break;

		case SYS_getpid :
			err = sysgetpid(&retval);
			break;

		case SYS_write :
			s=splhigh();
			char *c = (char *)tf->tf_a1;
			kprintf("%c",*c);
			retval = 1;
			splx(s);
			err=0;
			break;

		case SYS_read :
			d = (char *)kmalloc(sizeof(char)*((tf->tf_a2)+1));
			//char *b;
			//b = (char *)kmalloc(sizeof(char)*((tf->tf_a2)+1));

			d[0] = getch();
			d[1] ='\0';
			putch(d[0]);
			putch('\n');

			s=splhigh();
			copyoutstr(d,(userptr_t)(tf->tf_a1),(tf->tf_a2)+1,&act);
			//DEBUG(1,"actual: %d tf size: %d\n",act,(tf->tf_a2)+1);

			//copyinstr((tf->tf_a1),b,(tf->tf_a2)+1,&act);
			//DEBUG(1,"copied bacl:%s\n",b);
			err=0;
			kfree(d);
			retval = strlen(d)+1;
			splx(s);
			break;

		case SYS_waitpid :
			err = syswaitpid(tf->tf_a0,(int *)tf->tf_a1,tf->tf_a2,&retval);
			//syswaitpid(&retval);
			break;

		case SYS__exit :
			err = sysexit(&retval,tf);
			break;

		case SYS_execv:
			err=sysexecv((char*)tf->tf_a0, (char**)tf->tf_a1);
			break;

		case SYS_sbrk:
			err = sys_sbrk((int)tf->tf_a0, &retval);
			break;

		default:
			kprintf("Unknown syscall %d\n", callno);
			err = ENOSYS;
			break;
	}


	if (err) {
		/*
		 * Return the error code. This gets converted at
		 * userlevel to a return value of -1 and the error
		 * code in errno.
		 */
		tf->tf_v0 = err;
		tf->tf_a3 = 1;      /* signal an error */
	}
	else {
		/* Success. */
		tf->tf_v0 = retval;
		tf->tf_a3 = 0;      /* signal no error */
	}

	/*
	 * Now, advance the program counter, to avoid restarting
	 * the syscall over and over again.
	 */

	tf->tf_epc += 4;

	/* Make sure the syscall code didn't forget to lower spl */
	assert(curspl==0);
}

int my_get_pid(){
	//implement this part using the extern pid_array

	//int s=splhigh();
	int i;

	for(i=1;i<MAX_PIDS;i++){
		if(pid_array[i] == NULL){
			pid_array[i]=(struct thread *)0x1;
			exit_array[i]=1111; //current process now exists and has occupied this pid
			//splx(s);
			return i;
		}
	}

	//splx(s);
	return -1;
}





#define kernelpid 0

void pid_bootstrap(){
	int s = splhigh();

	//kprintf("bootstraping pid\n");
	int i;
	errsuperflag = 0;

	curthread->ppid = kernelpid;
	pid_array[kernelpid] = curthread;

	exit_array[kernelpid] = 0;
	interest_array[kernelpid]=0;
	parent_array[kernelpid]=kernelpid;

	for(i=1;i<MAX_PIDS;i++){
		pid_array[i] = NULL;
		exit_array[i] = 1234;
		interest_array[i]=-1;
		parent_array[i]=-1;
		wait_array[i]=cv_create("cv");
		wait_lock[i]=lock_create("lock");
	}
	splx(s);
	return;
}

//Child when created will excute this function
	void
md_forkentry(struct trapframe *tf)
{
	//kprintf("Kernel level fork\n");
	int s = splhigh();
	int *temp;
	if((curthread->errflag)==1){
		splx(s);
		sysexit(temp,tf);
	}else{
		tf->tf_a3 = 0;
		tf->tf_epc +=4;
		tf->tf_v0 = 0;
		pid_array[(int)(curthread->ppid)] = curthread;
		splx(s);
	}
	/*
	 * This function is provided as a reminder. You need to write
	 * both it and the code that calls it.
	 *
	 * Thus, you can trash it and do things another way if you prefer.
	 */
}

	void
forkentry(void *tf, unsigned long child_addrs )
{
	(void)child_addrs;
	int *temp;
	struct trapframe my_tf;

	int s = splhigh();
	if((curthread->errflag)==1){
			splx(s);
			sysexit(temp,tf);
	}else{
		//DEBUG(1,"forkentry %d\n",curthread->ppid);

		memcpy(&my_tf, tf, sizeof(struct trapframe));

		my_tf.tf_a3 = 0;
		my_tf.tf_epc +=4;
		my_tf.tf_v0 = 0;
		splx(s);

		kfree(tf);
		mips_usermode(&my_tf);
	}
}

int sysfork(int32_t *retval, struct trapframe *tf){
	int s=splhigh();
	DEBUG(1,"sysfork\n");

	if(errsuperflag==1){
		*retval=-1;
		splx(s);
		//thread_exit();
		return ENOMEM;
	}

	int ret=0;
	struct thread *child_thread;
	struct trapframe *child_tf;

	int pid = my_get_pid();

	if(pid==-1){
		splx(s);
		//errno=11;
		*retval=-1;
		return 11; // EAGAIN too many processes exist
	}else{
		*retval = pid;
	}
	parent_array[pid]=curthread->ppid; //initalize ur parent;
	DEBUG(1,"I am %d and my mommy is %d,I love her\n",pid,parent_array[pid]);


	child_tf = (struct trapframe *)kmalloc(sizeof(struct trapframe));
	if(child_tf==NULL){
		DEBUG(1,"tf messed %d\n",curthread->ppid);
		errsuperflag=1;
		splx(s);
		//thread_exit();
		*retval=-1;
		return ENOMEM;
	}

	memcpy(child_tf,tf, sizeof(struct trapframe));

	ret = thread_fork("child", child_tf, 0, forkentry, &child_thread);

	/*kprintf("copying stack\n");

	kfree(child_thread->t_stack);

	int act;

	child_thread->t_stack = kmalloc(STACK_SIZE);
	//memcpy(child_thread->t_stack,curthread->t_stack,sizeof(curthread->t_stack));
	copyoutstr(curthread->t_stack,(userptr_t)(child_thread->t_stack),sizeof(curthread->t_stack),&act);*/

	child_thread->my_parent=curthread;
	(curthread->counter)++;

	if(ret!=0) {
		pid_array[pid]=NULL;
		DEBUG(1,"thread_fork messed %d\n",curthread->ppid);
		//int *temp;
		//sysexit(temp,child_tf);
		//kfree(child_thread);
		//errno = ret;
		child_thread->errflag=1;
		//as_activate(child_thread->t_vmspace);
		errsuperflag=1;
		*retval=-1;
		splx(s);
		//thread_exit();
		return ret;
	}

	pid_array[pid] = child_thread;


	ret = as_copy((curthread->t_vmspace), &(child_thread->t_vmspace));
	if(ret!=0){
		DEBUG(1,"as_copy messed mommy: %d child: %d\n",curthread->ppid,pid);
		//int *temp;
		//sysexit(temp,child_tf);
		//kfree(child_thread);
		child_thread->errflag=1;
		as_activate(child_thread->t_vmspace);
		errsuperflag=1;
		*retval=-1;
		//as_destroy((child_thread->t_vmspace));
		splx(s);
		//thread_exit();
		return ret;
	}

	as_activate(child_thread->t_vmspace);

	((child_thread))->ppid = pid;
	//DEBUG(1,"Pid Set %d\n", ((child_thread))->ppid);
	splx(s);
	return 0;
}

int sysgetpid(int *retval){
	*retval = curthread->ppid;
	return 0;
}

int syswrite(int *retval){
	int s=splhigh();
	//DEBUG(1, "WRITE\n");
	*retval=0;
	splx(s);
	return 0;
}
/*
 * pid_t
 syswaitpid(pid_t pid, int *status, int options);

 process specified by pid to exit, and return its exit code in the integer pointed to by status
 */

int checkRestrictions(pid_t wpid, pid_t curpid){
	int s=splhigh();

	if(exit_array[wpid]==0){
		splx(s);
		return 0;
	}else if(interest_array[wpid]==curpid){ // check if waiting on each other
		splx(s);
		return 1;
	}else if(parent_array[wpid]!=curpid){ // check if waiting on immediate child
		splx(s);
		return 1;
	}else{
		interest_array[curpid]=wpid; //write to interest_array ... curpid is interested in waiting for w_pid
	}
	splx(s);
	return 0;
}

int syswaitpid(pid_t pid, int *status, int options,int *retval){
	int s=splhigh();
	DEBUG(1,"Waiting for: %x , %x\n", pid, curthread->ppid );
	if(options!=0){
		splx(s);
		return EINVAL;
	}

	if(status==NULL){
		splx(s);
		return EFAULT;
	}

	int fail;
	//show interest by writing own pid in an array at waiting_pids index
	//check if the interest goes other way around too and if it does then dont pass
	//also check if ur waiting on ur own immediate child
	fail=checkRestrictions(pid, curthread->ppid);
	if(fail){
		splx(s);
		return 1;//cant wait on this guy return with proper error code
	}
	switch (exit_array[pid]) {
		case 1234:
			//no such thread exists
			splx(s);
			*retval = pid;
			//errno=3;
			return 3;//ESRCH
			break;

		case 1111:
			//DEBUG(1,"i am: %d and waiting on pid: %d\n",curthread->ppid,pid);
			lock_acquire(wait_lock[pid]);
			cv_wait(wait_array[pid],wait_lock[pid]);
			*status=exit_array[pid];
			//DEBUG(1,"M BACK and m done waiting on %d\n",pid);
			lock_release(wait_lock[pid]);
			//wait on CV cause thread exists but hasnt exited yet
			break;

		default:
			//Pid has exited with some error code
			/*if(curthread->ppid==0 && curthread->counter==0){
				thread_exit();
			}*/
			*status=exit_array[pid];
			break;
	};
	*retval=pid;

	//DEBUG(1, "WAITPID kill:%d option:%d status:%d\n",pid,options,*status);
	splx(s);

	return 0;
}

int sysexit(int *retval, struct trapframe *tf){
	(void)retval;
	int s=splhigh();
	DEBUG(1, "EXIT code: %d pid:%d\n", tf->tf_v0,curthread->ppid);

	while((curthread->counter)!=0){
		thread_yield();

	}

	pid_array[curthread->ppid]=NULL;
	exit_array[curthread->ppid] = tf->tf_v0;
	cv_broadcast(wait_array[curthread->ppid], wait_lock[curthread->ppid]);
	parent_array[curthread->ppid]=-1; // set parent back to nothing
	if(curthread->ppid!=0)
		((curthread->my_parent)->counter)--;
	splx(s);
	thread_exit();

	return 0;
}

int sysexecv(char *program, char **args){

	int s = splhigh();
	int actual;
	char **mem_args;
	char *name;

	if(program==NULL){
		splx(s);
		return EFAULT;
	}

	name = (char *)kmalloc(sizeof(char)*128);
	assert(name!=NULL);

	copyinstr((userptr_t)program,name,(strlen(program))+1,&actual);

	int i,nargs;

	i=0;
	nargs=0;
	while(args[i]!=NULL){
		nargs++;
		i++;
	}
	DEBUG(1,"Nargs: %d\n",nargs);

	mem_args = (char **)kmalloc(sizeof(char *)*nargs);

	assert(mem_args!=NULL);

	for(i=0;i<nargs;i++){
		mem_args[i] = (char *)kmalloc(sizeof(char)*128);
		assert(mem_args[i]!=NULL);
	}

	for(i=0;i<nargs;i++){
		//strcpy(mem_args[i],args[i]);
		copyinstr((userptr_t)args[i],mem_args[i],strlen(args[i])+1,&actual);
		DEBUG(1,"prg args:%s\n",mem_args[i]);
	}

	as_destroy(curthread->t_vmspace);
	curthread->t_vmspace=NULL;
	DEBUG(1,"Program: %s\n",program);

	splx(s);
	return runprogram(name,mem_args,nargs);
}

#define USER_HEAP_LIMIT 65536
//16777216 - 16mb

int sys_sbrk(intptr_t inc, int32_t *retval){

	int s=splhigh();

	//kprintf("%d\n",inc);

	*retval=(int32_t)(curthread->t_vmspace->heaptop);


    if(inc==0){
    	//*retval=(void *)(curthread->t_vmspace->heaptop);
    	splx(s);
    	return 0;
    }else{

		//inc += 0x4;
		//inc &= 0xFFFFFFF4;


		if(((curthread->t_vmspace->heaptop)+inc) >= (curthread->t_vmspace->as_stackpbase)){
			*retval = (void*)-1;
			splx(s);
			return ENOMEM;
		}
		if (((curthread->t_vmspace->heaptop) + inc) < curthread->t_vmspace->heapbase) {
			*retval = (void*)-1;
			splx(s);
			return EINVAL;
		}

		if(((curthread->t_vmspace->heaptop)+inc-((curthread->t_vmspace->heapbase))) > USER_HEAP_LIMIT){
			*retval = (void*)-1;
			splx(s);
			return ENOMEM;
		}

		(curthread->t_vmspace->heaptop)+=inc;

		splx(s);
		return 0;
    }

}
