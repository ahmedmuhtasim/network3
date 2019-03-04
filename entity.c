/******************************************************************************/
/*                                                                            */
/* ENTITY IMPLEMENTATIONS                                                     */
/*                                                                            */
/******************************************************************************/

// Student names: Muhtasim Ahmed, Annie Khuu
// Student computing IDs: ma2qf, ak6eh
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
#include <stddef.h>
#include <stdlib.h>
#include "simulator.h"


#define SIZE 3000				// actual size of array
#define ALLOWABLE 10		// size of N vector, the vecotr representing the messages to resend at any given time; NOTE: we can still keep buffering more


/**** A ENTITY ****/
struct pkt * buffer[SIZE];						// circular buffer
int head;
int tail;
int bufferSize;									// keeps track of size of buffer... incremented with tail++, decremented with head++
int seqnum;

float inc;

void A_init() {
	head = 0;
	tail = 0;
	seqnum = 0;
	bufferSize = 0;			
	inc = 200.0;
}	


void A_output(struct msg message) {
	
	if (bufferSize >= (SIZE-3))	// we don't add anymore if our virtual buffer is full, the -3 is there for some breathing room... we don't want head to intersect the tail since we have circular buffer
		return;
	
	struct pkt * send =  (struct pkt *) malloc(sizeof(struct pkt));
	int checksum = 0;
	
	send->acknum = 0;		// no need to add to checksum since sum + 0 = sum
	
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
	
	if (head == tail){			// If head == tail, this is first element in N vector and so we start timer 
		stoptimer_A();
		starttimer_A(inc);
	} 
	else {							// need to indicate when we are sending even though our buffer isn't empty (we've already sent some)
		if (tail - head < ALLOWABLE)
			printf("Sending packet seq: %d but still waiting on ack from packet seq: %d \n", buffer[tail]->seqnum, buffer[head]->seqnum); 
	}
	
	if (tail - head < ALLOWABLE)	// only send if we're within N vector... or within N spaces of head, eg.. head = 80, tail upto 89 will be sent
		tolayer3_A(*send);
	
	tail = (tail+1)%SIZE;
	bufferSize++;
}

void A_input(struct pkt packet) {
	if (packet.acknum == 0){				// acknum has no significance 
		if (packet.seqnum == buffer[head]->seqnum){	// any way to make sure packet.seqnum is not corrupted?
			free(buffer[head]);
			head = (head+1)%SIZE;
			bufferSize--;
			stoptimer_A();
			starttimer_A(inc);
		}
	}
	
	
}

void A_timerinterrupt() {		// we waited long enough, resend entire buffer

	if (head == tail)				// nothing ahead of head
		return;
	
	printf("Timeout, resending packets: \n");
	if (tail > head){				// our N vector is not circling the buffer
		int end = tail;
		
		if (tail > head + ALLOWABLE)		// we are resending [head -> min(tail, head + N)] packets
			end = head + ALLOWABLE;
		
		stoptimer_A();
		starttimer_A(inc);
		for (int i = head; i < end; i++){
			printf("Resending seq: %d \n", buffer[i]->seqnum);
			tolayer3_A(*(buffer[i]));
		}
	}
	else{						// we are circling the buffer, just resend [head -> SIZE] packets for this rare case and our head should soon circle back to 0
			for (int i = head; i < SIZE; i++){
			printf("Resending seq: %d \n", buffer[i]->seqnum);
			tolayer3_A(*(buffer[i]));
		}
	}
	
}


/**** B ENTITY ****/
int seqnum2;


void B_init() {
	seqnum2 = 0;
}

void B_input(struct pkt packet) {
	
	if (packet.length > 20 || packet.length < 0)		// automatically know its corrupted
		return;
	
	int verSum = 0;
	
	verSum += packet.length;
	verSum += packet.seqnum;
	for (int i = 0; i < packet.length; i++)
		verSum += (int)(packet.payload[i]);
	
	if (verSum != packet.checksum)
		return;
			
	// successful transfer

	packet.acknum = 0;		// success code
	tolayer3_B(packet);		// send success message
	
	if (packet.seqnum != seqnum2) // we sent ack message, but we still discard if it is out of order
		return;
	
	struct msg send;
	
	send.length = packet.length;
	
	for (int i = 0; i < packet.length; i++)
		send.data[i] = packet.payload[i];
	
	tolayer5_B (send);
	
	seqnum2++;
}

void B_timerinterrupt() {
}