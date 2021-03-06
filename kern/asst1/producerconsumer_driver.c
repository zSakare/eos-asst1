/*
 * Driver file for the producer / consumer simulation.
 *
 * This starts up a number of producer and consumer threads and has them 
 * communicate via the API defined in producerconsumer_driver.h, which is
 * implemented by you in producerconsumer.c.
 *
 * NOTE: DO NOT RELY ON ANY CHANGES YOU MAKE TO THIS FILE, BECAUSE
 * IT WILL BE OVERWRITTEN DURING TESTING.
 */

#include <types.h> /* required by lib.h */
#include <lib.h> /* for kprintf */
#include <synch.h> /* for P(), V(), sem_* */
#include <thread.h> /* for thread_fork() */
#include <test.h>

#include "producerconsumer_driver.h"

/* The number of producers
 * This will be changed during testing
 */
#define NUM_PRODUCERS 2

/* The number of consumer threads
 * This number will be changed during testing
 */
#define NUM_CONSUMERS 5

/* Number of items each producer thread generates before
 * exiting. This number will be changed during testing.
 */
#define ITEMS_TO_PRODUCE 30

/* If a consumer receives more than the following number of
 * data items, it will automatically exit. This is to help
 * you during testing. Do not rely on it!
 */
#define CONSUMER_BORED_COUNT 10000

/* Semaphores which the simulator uses to determine when all
 * producer threads and all consumer threads have finished.
 */
static struct semaphore *consumer_finished;
static struct semaphore *producer_finished;

/* The producer thread's only function. This function calls
 * producer_produce ITEMS_TO_PRODUCE times and then exits. NUM_PRODUCERS
 * threads are started to run the function.
 */
static void
producer_thread(void *unused_ptr, unsigned long thread_num)
{
	struct pc_data thedata;
	int items_to_go = ITEMS_TO_PRODUCE;
        
	(void)unused_ptr; /* Avoid compiler warnings */
        
	kprintf("Producer started\n");
        
	while(--items_to_go) {
		thedata.item1 = items_to_go + (1000 * thread_num);
		/* Set second data item as related to the first so that
		 * the consumer can check both numbers are valid
                 */
		thedata.item2 = thedata.item1 + 1;
                
		producer_produce(thedata);
	}
        
	/* No more items... signal that we're done. */
	kprintf("Producer finished\n");
	V(producer_finished);
}

/* The consumer thread's only function. NUM_CONSUMERS threads are started,
 * each of which runs this function. The function continuously calls
 * consumer_consume() until it receives a special data item containing
 * two zero integers. NOTE: Don't rely on this protocol when designing
 * your producer_produce() and consumer_consume() functions!
 */
static void
consumer_thread(void *unused_ptr, unsigned long thread_num)
{
	struct pc_data thedata;
	int bored_count = 0;
        
	(void)unused_ptr;
	(void)thread_num;
        
	kprintf("Consumer started\n");
        
	thedata = consumer_consume();
        
	while (thedata.item1 != 0 && thedata.item2 != 0) {
		if (++bored_count == CONSUMER_BORED_COUNT) {
			break;
		}
		if(thedata.item1 +1 != thedata.item2) {
			kprintf("*** Error! Unexpected data %d and %d\n",
                                thedata.item1, thedata.item2);
		}
                
		thedata = consumer_consume();
	} 

	if (bored_count == CONSUMER_BORED_COUNT) {
		kprintf("*** Error! Consumer bored, exiting...\n");
	} else {
		kprintf("Consumer finished normally\n");
	}
        
	/* Signal that we're done. */
	V(consumer_finished);
        
}

/* Create a bunch of threads to consume data. */
static void
start_consumer_threads()
{
	int i;
	int result;
        
	for(i = 0; i < NUM_CONSUMERS; i++) {
                result = thread_fork("consumer thread", NULL,
				consumer_thread, NULL, i);
		if(result) {
			panic("start_consumer_threads: couldn't fork (%s)\n",
                              strerror(result));
		}
	}
}

/* Create a bunch of threads to produce data. */
static void
start_producer_threads()
{
	int i;
	int result;
        
	for(i = 0; i < NUM_PRODUCERS; i++) {
                result = thread_fork("producer thread", NULL,
				producer_thread, NULL, i);
		if(result) {
			panic("start_producer_threads: couldn't fork (%s)\n",
                              strerror(result));
		}
	}
}

/* Wait for all producer threads to exit.
 * Producers each produce ITEMS_TO_PRODUCE items and then signal
 * a semaphore and exit, so waiting for them to finish means
 * waiting on that semaphore NUM_PRODUCERS times.
 */
static void
wait_for_producer_threads()
{
	int i;
	kprintf("Waiting for producer threads to exit...\n");
	for(i = 0; i < NUM_PRODUCERS; i++) {
		P(producer_finished);
	}
	kprintf("All producer threads have exited.\n");
}

/* Instruct all consumer threads to exit and then wait for them
 * to indicate that they have exited.
 * Consumer threads run until told to stop using a special
 * message, described below.
 */
static void
stop_consumer_threads()
{
	int i;
	struct pc_data thedata;
        
	/* Our protocol for stopping consumer threads is to
	 * enqueue NUM_CONSUMERS sets of 0, 0 data items.
	 * This may change during testing, however.
         */
	thedata.item1 = 0;
	thedata.item2 = 0;
        
	for(i = 0; i < NUM_CONSUMERS; i++) {
		producer_produce(thedata);
	}
        
	/* Now wait for all consumers to signal completion. */
	for(i = 0; i < NUM_CONSUMERS; i++) {
		P(consumer_finished);
	}
	
}

/* The main function for the simulation. */
int
run_producerconsumer(int nargs, char **args)
{
	(void) nargs; /* Avoid "unused variable" warning */
	(void) args;

	kprintf("run_producerconsumer: starting up\n");
        
	/* Initialise synch primitives used in this simulator */
	consumer_finished = sem_create("consumer_finished", 0);
	if(!consumer_finished) {
		panic("run_producerconsumer: couldn't create semaphore\n");
	}

	producer_finished = sem_create("producer_finished", 0);
	if(!producer_finished ) {
		panic("run_producerconsumer: couldn't create semaphore\n");
	}
        
	/* Run any code required to initialise synch primitives etc */
	producerconsumer_startup();

	/* Run the simulation */
	start_consumer_threads();
	start_producer_threads();
        
	/* Wait for all producers and consumers to finish
	 * NOTE! Make sure you also handle the case where
	 * consumers finish before producers! */
	wait_for_producer_threads();
	stop_consumer_threads();
        
	/* Run any code required to shut down the simulation */
	producerconsumer_shutdown();
        
	/* Done! */
	sem_destroy(producer_finished);
	sem_destroy(consumer_finished);
	return 0;
}

