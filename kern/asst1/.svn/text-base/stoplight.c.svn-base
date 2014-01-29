/* 
 * stoplight.c
 *
 * 31-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: You can use any synchronization primitives available to solve
 * the stoplight problem in this file.
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
 * Number of cars created.
 */

#define NCARS 20

struct lock *lock1;

int nw=0,ne=0,sw=0,se=0;
int anw=0,ane=0,asw=0,ase=0;

/*
 *
 * Function Definitions
 *
 */

static const char *directions[] = { "N", "E", "S", "W" };

static const char *msgs[] = {
	"approaching:",
	"region1:    ",
	"region2:    ",
	"region3:    ",
	"leaving:    "
};

/* use these constants for the first parameter of message */
enum { APPROACHING, REGION1, REGION2, REGION3, LEAVING };

	static void
message(int msg_nr, int carnumber, int cardirection, int destdirection)
{
	kprintf("%s car = %2d, direction = %s, destination = %s\n",
			msgs[msg_nr], carnumber,
			directions[cardirection], directions[destdirection]);
}

/*
 * gostraight()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement passing straight through the
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
	void
gostraight(unsigned long cardirection,
		unsigned long carnumber)
{
	/*
	 * Avoid unused variable warnings.
	 */

	(void) cardirection;
	(void) carnumber;
}


/*
 * turnleft()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a left turn through the 
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
	void
turnleft(unsigned long cardirection,
		unsigned long carnumber)
{
	/*
	 * Avoid unused variable warnings.
	 */

	(void) cardirection;
	(void) carnumber;
}


/*
 * turnright()
 rness and so that we will all fit in the room and have enough copies of the test, you must write the test with your section. If you write the test with the other section and did not get permission in advance, your grade will be zero. *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a right turn through the 
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
	void
turnright(unsigned long cardirection,
		unsigned long carnumber)
{
	/*
	 * Avoid unused variable warnings.
	 */

	(void) cardirection;
	(void) carnumber;
}


/*
 * approachintersection()
 *
 * Arguments: 
 *      void * unusedpointer: currently unused.
 *      unsigned long carnumber: holds car id number.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Change this function as necessary to implement your solution. These
 *      threads are created by createcars().  Each one must choose a direction
 *      randomly, approach the intersection, choose a turn randomly, and then
 *      complete that turn.  The code to choose a direction randomly is
 *      provided, the rest is left to you to implement.  Making a turn
 *      or going straight should be done by calling one of the functions
 *      above.
 */
/*
 * 1=NW
 * 2=NE
 * 3=SW
 * 4=SE
 * */
static void set_path(int path[],int cardir,int destdir){
	if(cardir==0 && destdir == 1){ //north to east
		path[0]=1;
		path[1]=3;
		path[2]=4;
		path[3]=5;
	}else if(cardir==0 && destdir == 2){ //north to south
		path[0]=1;
		path[1]=3;
		path[2]=5;
	}else if(cardir==0 && destdir == 3){//north to west
		path[0]=1;
		path[1]=5;
	}else if(cardir==1 && destdir == 0){//east to north
		path[0]=2;
		path[1]=5;
	}else if(cardir==1 && destdir == 2){//east to south
		path[0]=2;
		path[1]=1;
		path[2]=3;
		path[3]=5;
	}else if(cardir==1 && destdir == 3){//east to west
		path[0]=2;
		path[1]=1;
		path[2]=5;
	}else if(cardir==2 && destdir == 0){//south to north
		path[0]=4;
		path[1]=2;
		path[2]=5;
	}else if(cardir==2 && destdir == 1){//south to east
		path[0]=4;
		path[1]=5;
	}else if(cardir==2 && destdir == 3){//south to west
		path[0]=4;
		path[1]=2;
		path[2]=1;
		path[3]=5;
	}else if(cardir==3 && destdir == 0){//west to north
		path[0]=3;
		path[1]=4;
		path[2]=2;
		path[3]=5;
	}else if(cardir==3 && destdir == 1){//west to east
		path[0]=3;
		path[1]=4;
		path[2]=5;
	}else if(cardir==3 && destdir == 2){//west to south
		path[0]=3;
		path[1]=5;
	}
}

static
	void
approachintersection(void * unusedpointer,
		unsigned long carnumber)
{




	//struct

	volatile int path[4];
	volatile int i=0;

	int cardirection, destdirection;

	cardirection = random() % 4;
	destdirection=random() % 4;
	while(destdirection==cardirection){
		destdirection = random() % 4;
	}


	// NW 1
	// NE 2
	// SW 3
	// SE 4

	set_path(path,cardirection,destdirection);

	while(1){
		lock_acquire(lock1);

		if(nw+ne+se+sw >= 2){
			lock_release(lock1);
			continue;
		}

		if(i==0){
			if(path[i]==1 && anw==0 ){
				anw=1;
				message(APPROACHING,carnumber,cardirection, destdirection);
				lock_release(lock1);
				break;
			}else if(path[i]==2 && ane==0){
				ane=1;
				message(APPROACHING,carnumber,cardirection, destdirection);
				lock_release(lock1);
				break;
			}else if(path[i]==3 && asw==0){
				asw=1;
				message(APPROACHING,carnumber,cardirection, destdirection);
				lock_release(lock1);
				break;
			}else if(path[i]==4 && ase==0){
				ase=1;
				message(APPROACHING,carnumber,cardirection, destdirection);
				lock_release(lock1);
				break;
			}
		}
		lock_release(lock1);
	}

	//message(APPROACHING,carnumber,cardirection, destdirection);

	while(path[i]!=5){

		lock_acquire(lock1);

		if(nw+ne+se+sw >= 3 && i==0){
			lock_release(lock1);
			continue;
		}

		if(path[i]==1 && nw==0){
			nw=1;
			if(i==0)
				anw=0;
			else{
				if(path[i-1]==1 && nw==1){
					nw=0;
				}else if(path[i-1]==2 && ne==1){
					ne=0;
				}else if(path[i-1]==3 && sw==1){
					sw=0;
				}else if(path[i-1]==4 && se==1){
					se=0;
				}
			}
			i++;
			message(i,carnumber,cardirection, destdirection);

		}else if(path[i]==2 && ne==0){
			ne=1;
			if(i==0)
				ane=0;
			else{
				if(path[i-1]==1 && nw==1){
					nw=0;
				}else if(path[i-1]==2 && ne==1){
					ne=0;
				}else if(path[i-1]==3 && sw==1){
					sw=0;
				}else if(path[i-1]==4 && se==1){
					se=0;
				}
			}
			i++;
			message(i,carnumber,cardirection, destdirection);

		}else if(path[i]==3 && sw==0){
			sw=1;
			if(i==0)
				asw=0;
			else{
				if(path[i-1]==1 && nw==1){
					nw=0;
				}else if(path[i-1]==2 && ne==1){
					ne=0;
				}else if(path[i-1]==3 && sw==1){
					sw=0;
				}else if(path[i-1]==4 && se==1){
					se=0;
				}
			}
			i++;
			message(i,carnumber,cardirection, destdirection);

		}else if(path[i]==4 && se==0){
			se=1;
			if(i==0)
				ase=0;
			else{

				if(path[i-1]==1 && nw==1){
					nw=0;
				}else if(path[i-1]==2 && ne==1){
					ne=0;
				}else if(path[i-1]==3 && sw==1){
					sw=0;
				}else if(path[i-1]==4 && se==1){
					se=0;
				}
			}
			i++;
			message(i,carnumber,cardirection, destdirection);
		}
		lock_release(lock1);
	}


	lock_acquire(lock1);

	if(path[i]==5){
		if(path[i-1]==1 && nw==1){
			nw=0;
		}else if(path[i-1]==2 && ne==1){
			ne=0;
		}else if(path[i-1]==3 && sw==1){
			sw=0;
		}else if(path[i-1]==4 && se==1){
			se=0;
		}
		message(LEAVING,carnumber,cardirection, destdirection);
	}
	lock_release(lock1);

	//kprintf("Car %d and direction %d \n", );

}


/*
 * createcars()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up the approachintersection() threads.  You are
 *      free to modiy this code as necessary for your solution.
 */

	int
createcars(int nargs,
		char ** args)
{
	int index, error;
	lock1=lock_create("test");


	/*
	 * Avoid unused variable warnings.
	 */

	(void) nargs;
	(void) args;

	/*
	 * Start NCARS approachintersection() threads.
	 */

	for (index = 0; index < NCARS; index++) {

		error = thread_fork("approachintersection thread",
				NULL,
				index,
				approachintersection,
				NULL
				);

		/*
		 * panic() on error.
		 */

		if (error) {

			panic("approachintersection: thread_fork failed: %s\n",
					strerror(error)
			     );
		}

	}
	/*while(1){
	  lock_acquire(lock1);
	  kprintf("%d %d %d %d ",nw,ne,sw,se);
	  lock_release(lock1);


	  }*/
	return 0;
}
