#ifndef _SYSCALL_H_
#define _SYSCALL_H_

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);
void md_forkentry(struct trapframe *tf);
void forkentry(void *, unsigned long);
int sysfork(int32_t *retval, struct trapframe *tf);
int sysgetpid(int *retval);
int syswrite(int *retval);
int syswaitpid(pid_t pid, int *status, int options, int *retval);
int sysexit(int *retval, struct trapframe *);
int my_get_pid();
int checkRestrictions(pid_t wpid, pid_t curpid);
int sysexecv(char *program, char **args);

int sys_sbrk(intptr_t inc, int32_t *retval);

#endif /* _SYSCALL_H_ */
