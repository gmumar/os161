/*
 * vm.c
 *
 *  Created on: 2013-03-25
 *      Author: umarghul
 */
#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/spl.h>
#include <machine/tlb.h>
#include <elf.h>
#include <vnode.h>
#include <vfs.h>
#include <kern/unistd.h>
#include <uio.h>
#include <synch.h>

#include <machine/trapframe.h>

#define DUMBVM_STACKPAGES    12

struct thread *threadhere;

int load_segment(struct vnode *v, off_t offset, vaddr_t vaddr, size_t memsize,
		size_t filesize, int is_executable);

//---------------------------
//COREMAP functions

int c_getpagenum(paddr_t addr) {
	int i;

	for (i = 0; i < mynpages; i++) {
		if (coremap[i].chunk_start == addr) {
			return i;
		}
	}
	return 0;
}

int c_getlastnum(paddr_t addr) {
	int i, j;

	for (i = 0; i < mynpages; i++) {
		if (coremap[i].chunk_start == addr) {
			break;
		}
	}

	for (j = i; j < mynpages; j++) {
		if (coremap[j].chunk_start == addr) {
			;
		} else {
			break;
		}
	}

	return j;
}

//-----------MY TLB Functions------------

void TLBwrite(vaddr_t faultaddress, paddr_t paddr) {
	u_int32_t ehi, elo;
	int i;

	for (i = 0; i < NUM_TLB; i++) {
		TLB_Read(&ehi, &elo, i);
		if (elo & TLBLO_VALID) {
			continue;
		}
		ehi = faultaddress;
		elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
		DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
		DEBUG(1,"Writing TLB direct\n");
		TLB_Write(ehi, elo, i);
		return;
	}
}

void invalidate(){
	int i;
	for (i=0; i<NUM_TLB; i++) {
		TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}
}
//---------Disk IO---------
int diskinit(){

	char path[] = "lhd0raw:";//"lhd0raw:"

	DEBUG(1,"opening: %s\n",path);

	//return vfs_open("lhd0raw:", O_RDWR ,&swap);

	return vfs_open("lhd0raw:", O_RDWR, &swap);
}

//------------------------------

void vm_bootstrap(void) {
	paddr_t ramStart;
	paddr_t temp;
	paddr_t ramEnd;
	int i;
	int j;

	replacecount=0;
	diskmap = (struct diskpage*)kmalloc(sizeof(struct diskpage)*MAX_DISK_PAGES);

	for(i=0;i<MAX_DISK_PAGES;i++){
		diskmap[i].off=0;
		diskmap[i].paddr=0;
		diskmap[i].vaddr=0;
		diskmap[i].taken=0;
	}

	sema_swap = sem_create("swap_sema",1);

	ram_getsize(&ramStart,&ramEnd);
	mynpages = ((ramEnd-ramStart)/PAGE_SIZE) - 10;
	temp = ram_stealmem(mynpages);
	temp += ramStart;
	coremap = (struct page*)PADDR_TO_KVADDR(ramStart);
	diskpages=0;

	for (i = 0; i < mynpages; i++) {
		coremap[i].lo = temp;
		coremap[i].free = 0;
		coremap[i].count = 0;

		for (j = 0; j < 10; j++) {
			coremap[i].pagetables[j] = NULL;
		}
		//kprintf("core:%d free:%d %d\n",i,coremap[i].free,coremap[i].lo);
		temp += PAGE_SIZE;
	}

	diskready = 0;
	vm_made = 1;
	swap = 0;
	kprintf("pages in ram: %d\n", mynpages);
}

paddr_t replace(void) {

	paddr_t addr;
	vaddr_t vaddr;
	struct PTE *current;
	struct uio u;
	int result, i, j, moved,s;

	result = 0;
	moved = 0;
	//s=splhigh();
	P(sema_swap);

	if (diskready == 0) {
		diskready = 1;
		result = diskinit();

		assert(result==0);
		if(result){
			DEBUG(1,"Disk init failed\n");
			//splx(s);
			V(sema_swap);
			return (paddr_t) NULL;
		}
	}

	while (1) {

		replacecount++;
		if (replacecount > mynpages) {
			replacecount = 1;
		}

		addr = coremap[replacecount].lo;
		vaddr = PADDR_TO_KVADDR(coremap[replacecount].lo);

		DEBUG(1,"returning page %d v:%x : ",replacecount,addr);

		if ((coremap[replacecount].cas == NULL) || (coremap[replacecount].kern==1)) {
			DEBUG(1,"kernel page\n");
			continue;
		}

		DEBUG(1,"process page\n");

		coremap[replacecount].free=1;
		coremap[replacecount].cas = curthread->t_vmspace;
		coremap[replacecount].chunk_start = addr;

		for(j=0;j<10 && moved==0;j++){
			current = coremap[replacecount].pagetables[j];

			if (current != NULL) {

				if (current->next == NULL) {
					if (current->as_pbase == 0) {
						DEBUG(1,"Page table paddr null");
						break;
					}

					if(current->page==addr && current->disk!=1){
						current->disk=1;
						//move page to disk
						if(moved==0){
							for(i=0;i<MAX_DISK_PAGES;i++){
								if((diskmap[i].taken)==0){
									diskmap[i].off=(i*PAGE_SIZE);
									diskmap[i].paddr=current->as_pbase;
									diskmap[i].vaddr=current->as_vbase;
									diskmap[i].taken=1;

									break;
								}
							}
							if(i==MAX_DISK_PAGES-1){
								V(sema_swap);
								return 0;
							}
							DEBUG(1,"before write %d v:%x\n",i,diskmap[i].vaddr);
							mk_kuio(&u, (void *)PADDR_TO_KVADDR(diskmap[i].paddr), PAGE_SIZE, diskmap[i].off, UIO_WRITE);
							VOP_WRITE(swap,&u);
							DEBUG(1,"Page %d written\n",i);
							diskpages++;
							moved =1;
							invalidate();
						}
					}
				}else{
					while(current!=NULL){
						if(current->page==addr && current->disk!=1){
							current->disk=1;
							//move page to disk
							if(moved==0){
								for(i=0;i<MAX_DISK_PAGES;i++){
									if((diskmap[i].taken)==0){
										diskmap[i].off=(i*PAGE_SIZE);
										diskmap[i].paddr=current->as_pbase;
										diskmap[i].vaddr=current->as_vbase;
										diskmap[i].taken=1;

										break;
									}
								}
								if(i==MAX_DISK_PAGES-1){
									V(sema_swap);
									return 0;
								}
								DEBUG(1,"before write %d v:%x\n",i,diskmap[i].vaddr);
								mk_kuio(&u,(void *)PADDR_TO_KVADDR(diskmap[i].paddr), PAGE_SIZE, (diskmap[i].off), UIO_WRITE);
								VOP_WRITE(swap,&u);
								DEBUG(1,"Page %d written \n",i);
								diskpages++;
								moved =1;
								invalidate();
							}
							break;
						}
						current = current->next;
					}
				}

			}
		}
		for(j=0;j<10;j++){
			coremap[replacecount].pagetables[j]=NULL;
		}
		coremap[replacecount].pagetables[0] = curthread->t_vmspace->pagetable;
		//kfree((void*)vaddr);
		break;
	}

	//splx(s);
	V(sema_swap);
	return coremap[replacecount].lo;

}

//static
paddr_t getppages(unsigned long npages,int kern) {
	int spl, i, j, first;
	paddr_t addr = 0;
	unsigned long count;
	i = 1;
	count = 0;
	first = 1;

	spl = splhigh();

	if (!vm_made) {
		addr = ram_stealmem(npages);
		//kprintf("addr given by ram:%d\n",addr);
	} else {
		while (count < npages) {
			if (i < mynpages) {
				if (coremap[i].free == 0) {
					if (first) {
						addr = coremap[i].lo;
						first = 0;
					}
					coremap[i].chunk_start = addr;
					coremap[i].free = 1;
					//coremap[i].count++;//take out move back down
					coremap[i].cas = curthread->t_vmspace;
					coremap[i].kern=kern;

					for (j = 0; j < 10; j++) {
						if (coremap[i].pagetables[j] == NULL) {

							//if (curthread->t_vmspace != NULL){
							if(kern==0){
								//Process asking for page
								coremap[i].pagetables[j]
										= curthread->t_vmspace->pagetable;
								coremap[i].count++;
								DEBUG(1,"Pagetable with %x in coremap page %d count:%d ppid:%d\n",curthread->t_vmspace->pagetable->as_pbase,i,coremap[i].count,curthread->ppid);
								break;
							} else {
								DEBUG(1,"M a kernel page %d\n", i);
								//Kernel asking for page
							}



						}//else case make core map pagetable NULL
					}

					//DEBUG(1,"addr given by coremap:%x page:%d\n",addr,i);
					count++;
				}
				i++;
			} else {
				DEBUG(1,"No empty page gona replace\n");
				splx(spl);// REPLACE inturrpts enabled
				//addr=replace();
				return 0;

			}
		}
	}

	splx(spl);
	return addr;
}

paddr_t handle_cow_fault(struct PTE *entery, struct PTE *head) {

	// something was wrong with count

	paddr_t newpaddr, oldpaddr;
	int i, j, found = 0, temp = 0;
	//struct PTE *current;

	oldpaddr = entery->page;
	newpaddr=0;

	for (i = 0; (i < mynpages) && !found; i++) {
		if (coremap[i].lo == oldpaddr) {
			for (j = 0; j < 10; j++) {
				if (coremap[i].pagetables[j] == head) {
					found = 1;
					if (coremap[i].count > 1) {
						newpaddr = getppages(1,0);
						if(newpaddr==0)
							return 0;
						memmove((void *)PADDR_TO_KVADDR(newpaddr),(const void *)PADDR_TO_KVADDR(oldpaddr),PAGE_SIZE);
						entery->as_pbase = newpaddr;
						entery->page = newpaddr;
						entery->permission = 110;
						coremap[i].count--;
						coremap[i].pagetables[j] = NULL;
					} else {
						newpaddr = oldpaddr;
						entery->permission = 110;

						/*if(coremap[i].count==0){
							coremap[i].count=0;
							coremap[i].free=0;
							coremap[i].chunk_start=0;
						}else{
							newpaddr = oldpaddr;
							entery->permission = 110;
						}*/
					}

					/*if(coremap[i].count>0){
					 ;
					 }else{
					 coremap[i].count=0;
					 coremap[i].free=0;
					 coremap[i].chunk_start=0;
					 }*/
					break;
				}
			}
			if (coremap[i].count == 1) {
				for (j = 0; j < 10 && !temp; j++) {
					if (coremap[i].pagetables[j] != NULL) {
						if (coremap[i].pagetables[j]->next == NULL) {
							if (coremap[i].pagetables[j]->page == oldpaddr) {
								coremap[i].pagetables[j]->permission = 110;
								temp = 1;
								break;
							}
						} else {
							struct PTE* current;
							current = coremap[i].pagetables[j];
							while (current != NULL) {
								if (current->page == oldpaddr) {
									current->permission = 110;
									temp = 1;
									break;
								}
								current = current->next;
							}
						}

					}
				}
			}
		}
	}
	return newpaddr & PAGE_FRAME;
}

int vm_fault(int faulttype, vaddr_t faultaddress) {
	vaddr_t stackbase, stacktop;
	paddr_t paddr,ptemp;
	int i,t;
	u_int32_t ehi, elo;
	struct addrspace *as;
	struct uio u;
	int spl, found, written, dirty;
	off_t offset;
	vaddr_t temp;
	struct PTE *entery, *new, *head;
	int result, is_executable, value;

	offset = 0;
	found = 0;
	written = 0;
	dirty = 1;
	is_executable = 0;
	spl = splhigh();

	faultaddress &= PAGE_FRAME;
	faultadd_outer = faultaddress;

	//DEBUG(1, "Fault handler: fault: 0x%x\n", faultaddress);

	switch (faulttype) {
	case VM_FAULT_READONLY:
		/* We always create pages read-write, so we can't get this */
		//panic("vm: got VM_FAULT_READONLY\n");
	case VM_FAULT_READ:
	case VM_FAULT_WRITE:
		break;
		splx(spl);
		return EINVAL;
	}

	as = curthread->t_vmspace;
	if (as == NULL) {
		/*
		 * No address space set up. This is probably a kernel
		 * fault early in boot. Return EFAULT so as to panic
		 * instead of getting into an infinite faulting loop.
		 */
		splx(spl);
		return EFAULT;
	}

	//if ((faultaddress >= as->allbase && faultaddress < as->segment[0].seg_start) && found==0) {
	//return faulttype;

	// paddr = (faultaddress - as->allbase) + getppages(1);

	// as->allbase += PAGE_SIZE;
	// new = (struct PTE *)kmalloc(sizeof(struct PTE));
	// new->as_pbase = paddr;
	// new->as_vbase = faultaddress;

	// new->next = as->pagetable;
	// as->pagetable = new;

	// found = 1;

	//TLBwrite(faultaddress,paddr);
	//}


	//Checking Pagetable

	entery = as->pagetable;
	head = as->pagetable;
	temp = entery->as_vbase;

	if(entery->next==NULL){
		if(faultaddress >= temp && faultaddress < temp+PAGE_SIZE){
			if (entery->disk == 0) {
				DEBUG(1,"faultaddress %x not on disk\n", faultaddress);
				//On RAM
				//paddr = (faultaddress - entery->as_vbase) + entery->as_pbase;
				found = 1;
				if ((entery->permission == 100) && ((faulttype
						== VM_FAULT_WRITE) || (faulttype == VM_FAULT_READONLY))) {
					dirty = 1;
					//DEBUG(1,"copy on write fault\n");
					paddr = handle_cow_fault(entery, head);
					if(paddr==0)
						return 3;
					//copy on write stuff
				}else if ((entery->permission == 100) && (faulttype
						== VM_FAULT_READ)) {
					paddr = (faultaddress - entery->as_vbase)
							+ entery->as_pbase;
					dirty = 0;
				} else {
					paddr = (faultaddress - entery->as_vbase)
							+ entery->as_pbase;
					dirty = 1;
				}
			}else{
				DEBUG(1,"read page from disk\n");

				for(i=0;i<MAX_DISK_PAGES;i++){
					DEBUG(1,"Page %d found v:%x f:%x diskp:%x enteryp:%x\n",i,diskmap[i].vaddr,faultaddress,diskmap[i].paddr,entery->as_pbase);
					if(diskmap[i].vaddr==entery->as_vbase && diskmap[i].paddr==entery->as_pbase && diskmap[i].taken==1){
						paddr =  getppages(1,0);
						if(paddr==0)
							return 3;
						splx(spl);
						P(sema_swap);
						entery->as_pbase = paddr;
						entery->page = paddr;
						//diskmap[i].paddr = paddr;
						mk_kuio(&u, (void *)PADDR_TO_KVADDR(paddr), PAGE_SIZE, (diskmap[i].off), UIO_READ);
						//u.uio_segflg = entery->permission==101 ? UIO_USERISPACE : UIO_USERSPACE;
						//u.uio_space=curthread->t_vmspace;
						t = VOP_READ(swap,&u);
						//int is_executable = entery->permission==101 ? 1 : 0;
						//t = load_segment(swap, diskmap[i].off, (char *)PADDR_TO_KVADDR(paddr),PAGE_SIZE,PAGE_SIZE,is_executable);
						assert(t==0);
						if(t){
							DEBUG(1,"Page %d read fail\n",t);
						}else{
							DEBUG(1,"Page %d found\n",i);
						}
						diskmap[i].taken = 0;
						diskmap[i].off=0;
						diskmap[i].paddr=0;
						diskmap[i].vaddr=0;
						entery->disk=0;
						diskpages--;
						V(sema_swap);
						spl=splhigh();
						break;
					}
				}
			}

		}
	} else {
		while (entery != NULL) {
			if (faultaddress >= temp && faultaddress < temp + PAGE_SIZE) {
				if (entery->disk == 0) {
					DEBUG(1,"faultaddress %x not on disk\n", faultaddress);
					//page on ram
					//paddr = (faultaddress - entery->as_vbase) + entery->as_pbase;
					found = 1;
					if ((entery->permission == 100) && ((faulttype
							== VM_FAULT_WRITE) || (faulttype
							== VM_FAULT_READONLY))) {
						dirty = 1;
						//kprintf("copy on write fault\n");
						paddr = handle_cow_fault(entery, head);
						if(paddr==0)
							return 3;
						//copy on write stuff
					} else if ((entery->permission == 100) && faulttype
							== VM_FAULT_READ) {
						paddr = (faultaddress - entery->as_vbase)
								+ entery->as_pbase;
						dirty = 0; //copy on write stuff
					} else {
						paddr = (faultaddress - entery->as_vbase)
								+ entery->as_pbase;
						dirty = 1;
					}
					break;
				}else{
					//DEBUG(1,"read page from disk\n");
					//Page on swap laod from there
					for(i=0;i<MAX_DISK_PAGES;i++){
						DEBUG(1,"Page %d found v:%x f:%x diskp:%x enteryp:%x\n",i,diskmap[i].vaddr,faultaddress,diskmap[i].paddr,entery->as_pbase);
						if(diskmap[i].vaddr==entery->as_vbase && diskmap[i].paddr==entery->as_pbase && diskmap[i].taken==1){
							paddr =  getppages(1,0);
							if(paddr==0)
								return 3;
							splx(spl);
							P(sema_swap);
							entery->as_pbase = paddr;
							entery->page = paddr;
							//diskmap[i].paddr = paddr;
							mk_kuio(&u,(void *)PADDR_TO_KVADDR(paddr), PAGE_SIZE, (diskmap[i].off), UIO_READ);
							//u.uio_segflg = entery->permission==101 ? UIO_USERISPACE : UIO_USERSPACE;
							//u.uio_space=curthread->t_vmspace;
							t = VOP_READ(swap,&u);
							//int is_executable = entery->permission==101 ? 1 : 0;
							//t = load_segment(swap, diskmap[i].off, (char *)PADDR_TO_KVADDR(paddr),PAGE_SIZE,PAGE_SIZE,is_executable);
							assert(t==0);
							if(t){
								DEBUG(1,"Page read fail e:%d\n",t);

							}else{
								DEBUG(1,"Page %d found\n",i);
							}
							diskmap[i].taken = 0;
							diskmap[i].off=0;
							diskmap[i].paddr=0;
							diskmap[i].vaddr=0;
							entery->disk=0;
							diskpages--;
							V(sema_swap);
							spl=splhigh();
							break;
						}
					}
				}
			} else {
				entery = entery->next;
				if (entery != NULL)
					temp = entery->as_vbase;
			}
		}
	}



	//Check Stack
	if (as->firstrun == 0) {
		stackbase = USERSTACK - PAGE_SIZE;
		as->firstrun = 1;
	} else {
		stackbase = as->as_stackpbase;
	}
	stacktop = USERSTACK;

	 if ((faultaddress >=( stackbase-PAGE_SIZE) && faultaddress < stacktop) && found==0) {
		// kprintf("Page needed for stack\n");
		 as->as_stackpbase -= PAGE_SIZE;
		 ptemp = getppages(1,0);
			if(ptemp==0)
				return 3;
		 if(ptemp==0){
			 DEBUG(1,"page error");
		 }
		 paddr = ptemp;//(faultaddress - stackbase) +
		 found = 1;

		if (as->firstpage == 0) {
			as->firstpage = 1;
			as->pagetable->as_pbase = paddr;
			as->pagetable->page = ptemp;
			as->pagetable->as_vbase = faultaddress;
			as->pagetable->permission = 110;
			as->pagetable->disk = 0;

		} else {
			new = (struct PTE *) kmalloc(sizeof(struct PTE));
			new->disk = 0;
			new->as_pbase = paddr;
			new->page = ptemp;
			new->as_vbase = faultaddress;
			new->permission = 110; //read write permission for stack
			new->next = as->pagetable->next;
			as->pagetable->next = new;
		}

		//TLBwrite(faultaddress,paddr);
	}


	//Check Heap
	 if ((faultaddress >=( as->heapbase) && faultaddress < as->heaptop) && found==0) {
		 ptemp = getppages(1,0);
		 if(ptemp==0){
			return 3;
		 }
		 paddr = ptemp;//(faultaddress - stackbase) +
		 found = 1;

		if (as->firstpage == 0) {
			as->firstpage = 1;
			as->pagetable->as_pbase = paddr;
			as->pagetable->page = ptemp;
			as->pagetable->as_vbase = faultaddress;
			as->pagetable->permission = 110;
			as->pagetable->disk = 0;

		} else {
			new = (struct PTE *) kmalloc(sizeof(struct PTE));
			new->disk = 0;
			new->as_pbase = paddr;
			new->page = ptemp;
			new->as_vbase = faultaddress;
			new->permission = 110; //read write permission for stack
			new->next = as->pagetable->next;
			as->pagetable->next = new;
		}

		//TLBwrite(faultaddress,paddr);
	}


	//Check segments
	temp = as->segment[0].seg_start;

	int count_seg = 0;
	if (found == 0) {
		while (count_seg < as->segcount) {
			for (i = 0; i < (as->segment[count_seg].pages); i++) {

				if (faultaddress >= temp && faultaddress < (temp + PAGE_SIZE)) {
					//DEBUG(1,"page found in virtual mem\n");
					paddr = getppages(1,0);
					 if(paddr==0){
						 return 3;
						 DEBUG(1,"page error");
					 }
					found = 1;

					if (as->firstpage == 0) {
						as->firstpage = 1;
						as->pagetable->as_pbase = paddr;
						as->pagetable->page = paddr;
						as->pagetable->as_vbase = faultaddress;
						as->pagetable->permission
								= as->segment[count_seg].permission;
						as->pagetable->disk = 0;
						is_executable = as->segment[count_seg].permission;

					} else {
						new = (struct PTE *) kmalloc(sizeof(struct PTE));
						new->disk = 0;
						new->as_pbase = paddr;
						new->page = paddr;
						new->as_vbase = faultaddress;
						new->permission = as->segment[count_seg].permission;//copy permission into pte
						new->next = as->pagetable->next;
						as->pagetable->next = new;

					}
					is_executable = as->segment[count_seg].permission;
					if (is_executable == 101) {
						is_executable = 1;
					} else {
						is_executable = 0;
					}
					//TLBwrite(faultaddress,paddr);
					//written=1;

					value = (faultaddress - as->segment[count_seg].seg_start);
					offset = (faultaddress - as->segment[count_seg].seg_start)
							+ as->segment[count_seg].offset;

					//kprintf("%d %d %x\n",as->segment[count_seg].flsize,as->segment[count_seg].memsize, faultaddress);

					int pagesBefore = (faultaddress - ((as->segment[count_seg].seg_start)& PAGE_FRAME))/PAGE_SIZE;
					int memSizeLoad = as->segment[count_seg].memsize - (pagesBefore*PAGE_SIZE);
					int fileSizeLoad = as->segment[count_seg].flsize -  (pagesBefore*PAGE_SIZE);

					//kprintf("%d %d %x\n",fileSizeLoad,memSizeLoad, faultaddress);

					memSizeLoad = memSizeLoad >= PAGE_SIZE? PAGE_SIZE: memSizeLoad;
					fileSizeLoad = fileSizeLoad >= PAGE_SIZE? PAGE_SIZE: fileSizeLoad;

					int *tmp = &(memSizeLoad);
					if(*tmp < 0){
						memSizeLoad = 0;
					}

					tmp = &(fileSizeLoad);
					if(*tmp < 0){
							fileSizeLoad = 0;
					}
					assert ( fileSizeLoad <= PAGE_SIZE);
					assert ( memSizeLoad <= PAGE_SIZE);

					//TLBwrite(faultaddress,paddr);
					//written =1;


					result = load_segment(as->v, offset, PADDR_TO_KVADDR(paddr),memSizeLoad,fileSizeLoad,
							is_executable);//PADDR_TO_KVADDR(paddr)



					if (result) {
						DEBUG(1,"load segment failed %d\n",result);
						//TLBwrite(faultaddress,paddr);
						splx(spl);
						return result;
					}
					break;
				} else {
					temp += PAGE_SIZE;
				}
			}
			count_seg++;
			temp = as->segment[count_seg].seg_start;
		}
	}

	if (found == 0) {
		splx(spl);
		return EFAULT;
	}

	/* make sure it's page-aligned */
	//paddr = paddr & PAGE_FRAME;
	assert((paddr & PAGE_FRAME)==paddr);

	int test = TLB_Probe(faultaddress, 0);

	if (test >= 0) {
		DEBUG(1,"VPN found in TLB\n");
		splx(spl);
		return 0;
	} else {
		if (written == 0) {
			for (i = 0; i < NUM_TLB; i++) {
				TLB_Read(&ehi, &elo, i);
				if (elo & TLBLO_VALID) {
					continue;
				}
				ehi = faultaddress;
				if (dirty == 1) {
					elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
				} else {
					elo = paddr | TLBLO_VALID;
				}
				//DEBUG(1,"Writing TLB\n");
				//DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
				TLB_Write(ehi, elo, i);
				splx(spl);
				return 0;
			}
		}
	}

	kprintf("dumbvm: Ran out of TLB entries - cannot handle page fault\n");
	splx(spl);
	return EFAULT;
}

vaddr_t alloc_kpages(int npages) {
	paddr_t pa;
	pa = getppages(npages,1);
	if (pa == 0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
}

void free_kpages(vaddr_t addr) {

	paddr_t pa = KVADDR_TO_PADDR(addr);
	int s = splhigh();
	//kprintf("In kfree %x\n",pa);
	//Add support for multiple page free
	int corepage = c_getpagenum(pa);
	int end = c_getlastnum(pa);
	int i;

	//kprintf("Page starting:%d end:%d\n",corepage,end);

	if (corepage == 0) {
		//kprintf("Page not found while tyring to free\n");
		splx(s);
		return;
	}
	for (i = corepage; i < end; i++) {
		if (coremap[i].count > 0) {
			coremap[i].count--;
		}

		if (coremap[i].count == 0) {
			coremap[i].free = 0; //free depending on count
			coremap[i].chunk_start = 0;
		}
	}

	DEBUG(1,"Freeing page %d for pid %d \n", corepage, curthread->ppid);

	splx(s);
	return;
}
