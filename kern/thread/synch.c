/*
 * Synchronization primitives.
 * See synch.h for specifications of the functions.
 */

#include <types.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <curthread.h>
#include <machine/spl.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *namearg, int initial_count)
{
	struct semaphore *sem;

	assert(initial_count >= 0);

	sem = kmalloc(sizeof(struct semaphore));
	if (sem == NULL) {
		return NULL;
	}

	sem->name = kstrdup(namearg);
	if (sem->name == NULL) {
		kfree(sem);
		return NULL;
	}

	sem->count = initial_count;
	return sem;
}

void
sem_destroy(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	spl = splhigh();
	assert(thread_hassleepers(sem)==0);
	splx(spl);

	/*
	 * Note: while someone could theoretically start sleeping on
	 * the semaphore after the above test but before we free it,
	 * if they're going to do that, they can just as easily wait
	 * a bit and start sleeping on the semaphore after it's been
	 * freed. Consequently, there's not a whole lot of point in 
	 * including the kfrees in the splhigh block, so we don't.
	 */

	kfree(sem->name);
	kfree(sem);
}

void 
P(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete the P without blocking.
	 */
	assert(in_interrupt==0);

	spl = splhigh();
	while (sem->count==0) {
		thread_sleep(sem);
	}
	assert(sem->count>0);
	sem->count--;
	splx(spl);
}

void
V(struct semaphore *sem)
{
	int spl;
	assert(sem != NULL);
	spl = splhigh();
	sem->count++;
	assert(sem->count>0);
	thread_wakeup(sem);
	splx(spl);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name)
{
	struct lock *lock;

	lock = kmalloc(sizeof(struct lock));
	if (lock == NULL) {
		return NULL;
	}

	lock->name = kstrdup(name);
	if (lock->name == NULL) {
		kfree(lock);
		return NULL;
	}

	lock->owner = NULL;
	//This area is only for initializing the lock struct
	// add stuff here as needed
	
	return lock;
}

void
lock_destroy(struct lock *lock)
{
	assert(lock != NULL);

	assert((lock->owner)==NULL);
	// add stuff here as needed
	//Release memory set in lock_create
	
	kfree(lock->name);
	kfree(lock);
}

void
lock_acquire(struct lock *lock)
{
	// Write this
	// splhigh() to disable ints
	 int s = splhigh();


	 while(lock->owner != NULL)thread_sleep(lock);
	 lock->owner = curthread;
	// spl0() to enable ints
	// Only by enabling and disabling interuppts

	 splx(s);
}

void
lock_release(struct lock *lock)
{
	// Write this
	int s = splhigh();
	// splhigh() to disable ints

	lock->owner = NULL;

	thread_wakeup(lock);

	splx(s);
	// splx() to enable ints

}

int
lock_do_i_hold(struct lock *lock)
{
	// Write this

	//(void)lock;  // suppress warning until code gets written

	if(lock->owner == curthread)
		return 1;
	else
		return 0;    // dummy until code gets written
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
	struct cv *cv;

	cv = kmalloc(sizeof(struct cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->name = kstrdup(name);
	if (cv->name==NULL) {
		kfree(cv);
		return NULL;
	}
	
	// add stuff here as needed
	
	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

	// add stuff here as needed
	
	kfree(cv->name);
	kfree(cv);
}
/*
*    cv_wait      - Release the supplied lock, go to sleep, and, after
*                   waking up again, re-acquire the lock.
*/

/*
 * The "signal" operation on a condition variable allows a thread executing in a monitor
 *  (which implies it must be holding the same lock that the waiting thread was holding
 *  when it called cv_wait)
 */
/*struct list_of_threads{
	thread *thread_addr;
	struct lock *lock;
};*/

void
cv_wait(struct cv *cv, struct lock *lock)
{
	//save thread with its lock

	int s = splhigh();
	//cv->lock=lock;
	lock_release(lock);
	thread_sleep(cv);
	lock_acquire(lock);
	splx(s);
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	// Write this

	//using lock get the thread saved and wake it
	int s = splhigh();
	//if(lock == cv->lock){
		thread_wakeup_single(cv);
	//}
	splx(s);

}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	int s = splhigh();
	//if(lock == cv->lock){
		thread_wakeup(cv);
	//}
	splx(s);
}
