#ifndef _VM_H_
#define _VM_H_

#include <machine/vm.h>
#include <addrspace.h>

/*
 * VM system-related definitions.
 *
 * You'll probably want to add stuff here.
 */


/* Fault-type arguments to vm_fault() */
#define VM_FAULT_READ        0    /* A read was attempted */
#define VM_FAULT_WRITE       1    /* A write was attempted */
#define VM_FAULT_READONLY    2    /* A write to a readonly page was attempted*/

#define MAX_DISK_PAGES		256

typedef struct PTE *pte_ptr;

struct page{
	paddr_t lo;
	paddr_t chunk_start;
	pte_ptr pagetables[10];
	struct addrspace *cas;
	int count;
	int free;
	int kern;
};

struct diskpage{

	paddr_t paddr;
	vaddr_t vaddr;
	off_t off;
	int taken;
};

struct page *coremap;
struct lock *corelock;
int mynpages;
int vm_made;
int replacecount;

vaddr_t faultadd_outer;

int diskready;
int diskpages;
struct diskpage *diskmap;
struct vnode *swap;
struct semaphore *sema_swap;

/* Initialization function */
void vm_bootstrap(void);

/* Fault handling function called by trap code */
int vm_fault(int faulttype, vaddr_t faultaddress);
paddr_t handle_cow_fault(struct PTE *entery, struct PTE *head);

/* Allocate/free kernel heap pages (called by kmalloc/kfree) */
vaddr_t alloc_kpages(int npages);
void free_kpages(vaddr_t addr);
//static
paddr_t getppages(unsigned long npages,int);

#endif /* _VM_H_ */
