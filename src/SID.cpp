/**
 * @file SID.cpp
 *
 * @brief System Identification Process
 *
 * Performs the SINDy algorithm on the input buffer
 * 
 * @author Stefan Bichlmaier, <bichlmaier.stef@gmail.com>
 *
 */

// ------------------------------------------------------------------------------
//   Includes
// ------------------------------------------------------------------------------

#include "SID.h"
#include "interpolate.h"

void* start_SID_compute_thread(void *args);

// ------------------------------------------------------------------------------
//   Con/De structors
// ------------------------------------------------------------------------------
SID::
SID(Buffer *input_buffer_)
{
    input_buffer = input_buffer_;
}

SID::
~SID()
{
}

void SID::
compute_thread()
{
    compute_status = true;
    while ( ! time_to_exit )
	{
        data = input_buffer->clear();
		interpolate(data); // Bring data to a common timebase and interpolate it
		// might cause a race condition where the SINDy thread checks disarmed just before the main thread
		// sets the disarmed flag. Worst case scenario the SINDy thread logs one more buffer
		if(!disarmed)
		{
			log_buffer_to_csv(data, filename);
		}
		usleep(5000000);
	}
	compute_status = false;

	return;
}

void SID::
start()
{
	printf("START SINDy COMPUTE THREAD \n");
	int result = pthread_create( &compute_tid, NULL, &start_SID_compute_thread, this);
	if ( result ) throw result;
}

void SID::
stop()
{
	// --------------------------------------------------------------------------
	//   CLOSE THREADS
	// --------------------------------------------------------------------------
	printf("STOP SINDy THREAD\n");

	// signal exit
	time_to_exit = true;

	// wait for exit
	pthread_join(compute_tid ,NULL);

	// now the read and write threads are closed
	printf("\n");
}

// ------------------------------------------------------------------------------
//   Quit Handler
// ------------------------------------------------------------------------------
// Separate quit handler for interrupt
void
SID::
handle_quit( int sig )
{
	// 
	try {
		stop();
	}
	catch (int error) {
		fprintf(stderr,"Warning, could not stop SINDy\n");
	}

}

// ------------------------------------------------------------------------------
//  Pthread Starter Helper Functions
// ------------------------------------------------------------------------------

void*
start_SID_compute_thread(void *args)
{
	// takes a SID object
	SID *SINDy = (SID *)args;

	// run the object's read thread
	SINDy->compute_thread();

	// done!
	return NULL;
}