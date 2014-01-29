#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include <thread.h>
#include <curthread.h>
#include <machine/spl.h>
#include <machine/tlb.h>
#include <list.h>
#include <vfs.h>

#define DUMBVM_STACKPAGES    12

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

struct PTE *getlastnode(struct PTE *head){

	struct PTE *current;

	current = head;

	while(current->next!=NULL){
		current = current->next;
	}

	return current;
}


struct addrspace *
as_create(void)
{

	struct addrspace *as = kmalloc(sizeof(struct addrspace));
	if (as==NULL) {
		return NULL;
	}

	//as->as_vbase1 = 0;
	//as->as_pbase1 = 0;
	//as->as_npages1 = 0;
	//as->as_vbase2 = 0;
	//as->as_pbase2 = 0;
	//as->as_npages2 = 0;
	as->as_stackpbase = USERSTACK;
	as->allbase=0;
	as->firstpage=0;

	as->firstrun=0;

	as->pagetable = (struct PTE*)kmalloc(sizeof(struct PTE));
	//as->pagetable = getppages(1);
	as->pagetable->next=NULL;
	as->pagetable->as_pbase=0;
	as->pagetable->as_vbase=0;



	as->segment = (struct seg*)kmalloc(sizeof(struct seg)*2);

	as->segcount=0;


	return as;
}

int add_to_coremap(paddr_t paddr, struct PTE *pagetable){

	int i, j,s,found=0;

	s=splhigh();
	for(i=0;(i<mynpages) && !found;i++){
		if(coremap[i].lo==paddr){
			for(j=0;j<10;j++){
				if(coremap[i].pagetables[j]==NULL){
					coremap[i].pagetables[j]=pagetable;
					coremap[i].count++;
					DEBUG(1,"Adding. Pagetable with %x in coremap page %d count:%d ppid:%d\n",curthread->t_vmspace->pagetable->as_pbase,i,coremap[i].count,curthread->ppid);
					found=1;
					break;
				}
			}
		}
	}
	splx(s);

}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	int i=0,s;

	s=splhigh();

	struct PTE *old_current, *new_current;

	struct addrspace *new;
	new = as_create();
	if(new==NULL){
		return ENOMEM;
	}

	new->firstrun=old->firstrun;//change
	new->as_stackpbase=old->as_stackpbase;
	new->firstpage=old->firstpage;
	new->v= old->v;

	//kfree(new->segment);
	//kfree(new->pagetable);

	new->segcount=old->segcount;
	//new->segment=(struct seg*)kmalloc(sizeof(struct seg)*old->segcount);
	for(i=0;i<old->segcount;i++){
		new->segment[i].loaded=old->segment[i].loaded;
		new->segment[i].offset=old->segment[i].offset;
		new->segment[i].pages=old->segment[i].pages;
		new->segment[i].permission=old->segment[i].permission;
		new->segment[i].seg_start=old->segment[i].seg_start;
		new->segment[i].flsize=old->segment[i].flsize;
		new->segment[i].memsize=old->segment[i].memsize;
	}


	old_current= old->pagetable;
	if(old_current->next==NULL){
		//new->pagetable=(struct PTE*)kmalloc(sizeof(struct PTE));
		new->pagetable->as_pbase=old_current->as_pbase;
		new->pagetable->as_vbase=old_current->as_vbase;
		new->pagetable->disk=old_current->disk;
		new->pagetable->next=NULL;
		new->pagetable->permission=100;
		new->pagetable->page=old_current->page;
		old_current->permission=100;
		add_to_coremap(new->pagetable, new->pagetable);
	}else{
		//new->pagetable=(struct PTE*)kmalloc(sizeof(struct PTE));
		new_current=new->pagetable;
		while(old_current!=NULL){

			new_current->as_pbase=old_current->as_pbase;
			new_current->as_vbase=old_current->as_vbase;
			new_current->page=old_current->page;
			new_current->disk=old_current->disk;
			new_current->permission=100;
			old_current->permission=100;
			if(old_current->next!=NULL){
				new_current->next=(struct PTE*)kmalloc(sizeof(struct PTE));
				//add_to_coremap(new_current->page, new->pagetable);

				//new_current=new_current->next;
			}else{
				new_current->next=NULL;

			}
			add_to_coremap(new_current->page, new->pagetable);

			old_current=old_current->next;
			new_current=new_current->next;
		}
	}


	for (i=0; i<NUM_TLB; i++) {
		TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}


	*ret=new;
	splx(s);
	return 0;
}

void
as_destroy(struct addrspace *as)
{
	struct PTE *current, *temp;
	int s=splhigh(),i,j;

	as->firstrun=0;
	as->allbase=0;
	as->as_stackpbase=USERSTACK;
	as->segcount=0;
	//vfs_close(as->v);

	for(i=0;(i<mynpages);i++){
		current = as->pagetable;
		if(current->next==NULL){
			if(coremap[i].lo==current->as_pbase){
				for(j=0;j<10;j++){
					if(coremap[i].pagetables[j]==as->pagetable){
						coremap[i].pagetables[j]=NULL;
						break;
						//count decrement;
					}
				}
			}
		}else{
			while(current!=NULL){
				temp = current->next;
				if(coremap[i].lo==current->as_pbase){
					for(j=0;j<10;j++){
						if(coremap[i].pagetables[j]==as->pagetable){
							coremap[i].pagetables[j]=NULL;
							break;
						}
					}
				}
				current = temp;
			}
		}
	}



	current = as->pagetable;

	if(current->next==NULL){
		kfree(PADDR_TO_KVADDR(current->page));
	}else{
		while(current!=NULL){
			temp = current->next;
			kfree(PADDR_TO_KVADDR(current->page));
			current = temp;
		}
	}

	current = as->pagetable;
	if(current->next==NULL){
		kfree((current));
	}else{
		while(current!=NULL){
			temp = current->next;
			kfree((current));
			current = temp;
		}
	}

	kfree(as->segment);
	kfree((as));

	for (i=0; i<NUM_TLB; i++) {
		TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(s);
}

void
as_activate(struct addrspace *as)
{
	int i, spl;

	(void)as;

	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t fsz,
		 int readable, int writeable, int executable, struct vnode *v, off_t offset, size_t msz)
{size_t npages;

as->segment[as->segcount].flsize = fsz;
as->segment[as->segcount].memsize = msz;
/* Align the region. First, the base... */
msz += vaddr & ~(vaddr_t)PAGE_FRAME;
vaddr &= PAGE_FRAME;

/* ...and now the length. */
msz = (msz + PAGE_SIZE - 1) & PAGE_FRAME;

npages = msz / PAGE_SIZE;

if(readable)
	as->segment[as->segcount].permission+=100;
if(writeable)
	as->segment[as->segcount].permission+=10;
if(executable)
	as->segment[as->segcount].permission+=1;

as->v = v;

as->segment[as->segcount].pages = npages;
as->segment[as->segcount].loaded = 0;
as->segment[as->segcount].seg_start = vaddr;
as->segment[as->segcount].offset = offset;
as->segment[as->segcount].memsize = msz;
as->segcount+=1;



/*
 * Support for more than two regions is not available.
 */
//kprintf("dumbvm: Warning: too many regions\n");
//return EUNIMP;
return 0;
}

int
as_prepare_load(struct addrspace *as)
{
	as->heaptop = as->heapbase = as->segment[(as->segcount)-1].seg_start + as->segment[(as->segcount)-1].pages*PAGE_SIZE;

	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */

	(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	(void)as;
	//assert(as->as_stackpbase != 0);


	*stackptr = USERSTACK;
	return 0;
}

