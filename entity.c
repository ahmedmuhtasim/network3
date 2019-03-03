/******************************************************************************/
/*                                                                            */
/* ENTITY IMPLEMENTATIONS                                                     */
/*                                                                            */
/******************************************************************************/

// Student names:
// Student computing IDs:
//
//
// This file contains the actual code for the functions that will implement the
// reliable transport protocols enabling entity "A" to reliably send information
// to entity "B".
//
// This is where you should write your code, and you should submit a modified
// version of this file.
//
// Notes:
// - One way network delay averages five time units (longer if there are other
//   messages in the channel for GBN), but can be larger.
// - Packets can be corrupted (either the header or the data portion) or lost,
//   according to user-defined probabilities entered as command line arguments.
// - Packets will be delivered in the order in which they were sent (although
//   some can be lost).
// - You may have global state in this file, BUT THAT GLOBAL STATE MUST NOT BE
//   SHARED BETWEEN THE TWO ENTITIES' FUNCTIONS. "A" and "B" are simulating two
//   entities connected by a network, and as such they cannot access each
//   other's variables and global state. Entity "A" can access its own state,
//   and entity "B" can access its own state, but anything shared between the
//   two must be passed in a `pkt` across the simulated network. Violating this
//   requirement will result in a very low score for this project (or a 0).
//
// To run this project you should be able to compile it with something like:
//
//     $ gcc entity.c simulator.c -o myproject
//
// and then run it like:
//
//     $ ./myproject 0.0 0.0 10 500 3 test1.txt
//
// Of course, that will cause the channel to be perfect, so you should test
// with a less ideal channel, and you should vary the random seed. However, for
// testing it can be helpful to keep the seed constant.
//
// The simulator will write the received data on entity "B" to a file called
// `output.dat`.

#include <stdio.h>
#include "simulator.h"

#define BUFFERSIZE 5		// size of virtual buffer
#define SIZE 100				// actual size of array

/**** A ENTITY ****/
struct pkt * buffer[SIZE];						// circular buffer
int head;
int tail;
int bufferSize;									// keeps track of size of buffer... incremented with tail++, decremented with head++
int seqnum;

float inc;
//struct pkt * copy[BUFFERSIZE];		// used to copy elements, if buffer reaches end of array 

void A_init() {
	head = 0;
	tail = 0;
	seqnum = 0;
	bufferSize = 0;			
	inc = 10.0;
}

// this method may not be necessary with a circuclar array
//void shiftBuffer()								// eventually, buffer will shift all the way to end of array, we want to move elements back then
	


void A_output(struct msg message) {
	
	if (bufferSize >= (BUFFERSIZE))	// we don't add anymore if our virtual buffer is full
		return;
	
	struct pkt * send =  (struct pkt *) malloc(sizeof(struct pkt));
	int checksum = 0;
	
	send->acknum = 0;		// success code, no need to add to checksum since sum + 0 = sum
	
	send->length = message.length;
	checksum += send->length;
	
	for (int i = 0; i < send->length; i++){
		send->payload[i] = message.data[i];
		checksum += (int)(send->payload[i]);
	}
	
	send->seqnum = seqnum;	
	checksum += seqnum;
	seqnum++;
	
	send->checksum = checksum;
	
	buffer[tail] = send;
	tail = (tail+1)%SIZE;
	bufferSize++;
}

void A_input(struct pkt packet) {
	
	if (packet.acknum == 0){				///not checking for corruption of acknum for now, its pretty silly, because we'd have to be very very unlucky to get acknum=0 when receiver wanted to send acknum=1... maybe give acknum=1 a more complex value: 0b111111111?
		if (packet.seqnum == buffer[head]->seqnum){
			free(buffer[head]);
			head = (head+1)%SIZE;
			bufferSize--;
			stoptimer_A();
			starttimer_A(inc);
		}
		else {
			// guessing we do nothing if its out of order?
		}
	}
	
	
}

void A_timerinterrupt() {
	
	
}


/**** B ENTITY ****/
int seqnum2;
struct msg send;

void B_init() {
	seqnum2 = 0;
}

void B_input(struct pkt packet) {
	int verSum = 0;
	
	verSum += packet.length;
	verSum += packet.seqnum;
	for (int i = 0; i < packet.length; i++)
		verSum += (int)(packet.payload[i]);
	
	if (verSum != packet.checksum){	//corrupted
		packet.checksum = verSum + 1;	// do we need this? will entity_A do the same check?.... the +1 accounts for the acknum value in versum... aka: versum += acknum
		packet.acknum = 1;		// corrupt code
		tolayer3_B(packet);		// send corrupt message (do we still care about receiver checksum?
		return;
	}
		
	// what do I do if it is out of order (seqnum != seqnum2), also need to verify that this is not a duplicate
	if (packet.seqnum != seqnum2)
		return;
	
	// if it came here, success... 
	//packet.checksum = verSum;	// we don't need this since checksum has to be equal to versum for it to be successful
	packet.acknum = 0;		// success code
	tolayer3_B(packet);		// send success message
	
	send.length = packet.length;
	
	for (int i = 0; i < packet.length; i++)
		send.data[i] += (int)(packet.payload[i]);
	
	tolayer5_B (send);
	
	seqnum2++;
	
	
}

void B_timerinterrupt() {
	
	
}