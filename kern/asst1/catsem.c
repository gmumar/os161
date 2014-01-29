/*
 * catsem.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: Please use SEMAPHORES to solve the cat syncronization problem in 
 * this file.
 */


/*
 * 
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>


/*
 * 
 * Constants
 *
 */

/*
 * Number of food bowls.
 */

#define NFOODBOWLS 2

/*
 * Number of cats.
 */

#define NCATS 6

/*
 * Number of mice.
 */

#define NMICE 2


/*
 * 
 * Function Definitions
 * 
 */

struct semaphore *sem_mouse_count;
struct semaphore *sem_cat_count;
struct semaphore *bowl;
struct semaphore *sem_taken;

struct semaphore *room;

int mouse_count=0;
int cat_count=0;
int taken1;
int taken2;

/* who should be "cat" or "mouse" */
	static void
sem_eat(const char *who, int num, int bowl, int iteration)
{
	kprintf("%s: %d starts eating: bowl %d, iteration %d\n", who, num, 
			bowl, iteration);
	clocksleep(1);
	kprintf("%s: %d ends eating: bowl %d, iteration %d\n", who, num, 
			bowl, iteration);
}

/*
 * catsem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds the cat identifier from 0 to NCATS - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using semaphores.
 *
 */

static
	void
catsem(void * unusedpointer, 
		unsigned long catnumber)
{
	int iteration = 0;

	//kprintf("cat %d %x\n",catnumber,unusedpointer);

	while(iteration<4){
		//P(sem_cat_count);

		P(sem_cat_count);
		cat_count++;
		if(cat_count==1)
			P(room);
		V(sem_cat_count);

		P(bowl);

		if(taken1==0){
			taken1=1;
			sem_eat("cat",catnumber,1,iteration);
			taken1=0;
		}
		else if(taken2==0){
			taken2=1;
			sem_eat("cat",catnumber,2,iteration);
			taken2=0;
		}
		iteration++;
		V(bowl);
		P(sem_cat_count);
		cat_count--;
		if(cat_count==0)
			V(room);
		V(sem_cat_count);
		//V(sem_cat_count);
	}
	return;
}


/*
 * mousesem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds the mouse identifier from 0 to 
 *              NMICE - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using semaphores.
 *
 */

static
	void
mousesem(void * unusedpointer, 
		unsigned long mousenumber)
{
	int iteration = 0;

	//kprintf("mouse %d %x\n",mousenumber,unusedpointer);

	while(iteration<4){

		P(sem_mouse_count);
		mouse_count++;
		if(mouse_count==1)
			P(room);
		V(sem_mouse_count);

		P(bowl);
		if(taken1==0){
			taken1=1;
			sem_eat("mouse",mousenumber,1,iteration);
			iteration++;
			taken1=0;
			V(bowl);
		}
		else if(taken2==0){
			taken2=1;
			sem_eat("mouse",mousenumber,2,iteration);
			iteration++;
			taken2=0;
			V(bowl);
		}

		P(sem_mouse_count);
		mouse_count--;
		if(mouse_count==0)
			P(room);
		V(sem_mouse_count);
	}
	return;
}


/*
 * catmousesem()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up catsem() and mousesem() threads.  Change this 
 *      code as necessary for your solution.
 */

void init_sem(){

	sem_mouse_count = sem_create("mouse_sem",1);
	sem_cat_count = sem_create("cat_sem",6);
	bowl= sem_create("bowl_sem",2);
	room= sem_create("room_sem",1);
	//sem_taken= sem_create("taken",1);

}

	int
catmousesem(int nargs,
		char ** args)
{
	int index, error;
	taken1=0;
	taken2=0;
	kprintf("main\n");


	init_sem();

	/*
	 * Start NCATS catsem() threads.
	 */

	for (index = 0; index < NCATS; index++) {

		error = thread_fork("catsem Thread", 
				NULL, 
				index, 
				catsem, 
				NULL
				);

		/*
		 * panic() on error.
		 */

		if (error) {

			panic("catsem: thread_fork failed: %s\n", 
					strerror(error)
			     );
		}
	}

	/*
	 * Start NMICE mousesem() threads.
	 */

	for (index = 0; index < NMICE; index++) {

		error = thread_fork("mousesem Thread", 
				NULL, 
				index, 
				mousesem, 
				NULL
				);

		/*
		 * panic() on error.
		 */

		if (error) {

			panic("mousesem: thread_fork failed: %s\n", 
					strerror(error)
			     );
		}
	}

	return 0;
}


/*
 * End of catsem.c
 */
