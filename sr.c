#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "emulator.h"
#include "gbn.h"

/* ******************************************************************
   Go Back N protocol.  Adapted from J.F.Kurose
   ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.2  

   Network properties:
   - one way network delay averages five time units (longer if there
   are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
   or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
   (although some can be lost).

   Modifications: 
   - removed bidirectional GBN code and other code not used by prac. 
   - fixed C style to adhere to current programming style
   - added GBN implementation
**********************************************************************/

#define RTT  16.0       /* round trip time.  MUST BE SET TO 16.0 when submitting assignment */
#define WINDOWSIZE 6    /* the maximum number of buffered unacked packet */
#define SEQSPACE 7      /* the min sequence space for GBN must be at least windowsize + 1 */
#define NOTINUSE (-1)   /* used to fill header fields that are not being used */

/****************************************************
 *Adding some global varables for the selective repeat functions
 
 ****************************************************/
struct sender_packet {
  struct pkt packet;
  int acked;
  int active;
};

static struct sender_packet sender_buffer[SEQSPACE];
static int base = 0;

struct receiver_slot {
  struct pkt packet;
  int received;
};

static struct receiver_slot receiver_buffer[SEQSPACE];



/* generic procedure to compute the checksum of a packet.  Used by both sender and receiver  
   the simulator will overwrite part of your packet with 'z's.  It will not overwrite your 
   original checksum.  This procedure must generate a different checksum to the original if
   the packet is corrupted.
*/
int ComputeChecksum(struct pkt packet)
{
  int checksum = 0;
  int i;

  checksum = packet.seqnum;
  checksum += packet.acknum;
  for ( i=0; i<20; i++ ) 
    checksum += (int)(packet.payload[i]);

  return checksum;
}

bool IsCorrupted(struct pkt packet)
{
  if (packet.checksum == ComputeChecksum(packet))
    return (false);
  else
    return (true);
}


/********* Sender (A) variables and functions ************/

/*static struct pkt buffer[WINDOWSIZE];  array for storing packets waiting for ACK */
static int windowfirst, windowlast;    /* array indexes of the first/last packet awaiting ACK */
static int windowcount;                /* the number of packets currently awaiting an ACK */
static int nextseqnum = 0;               /* the next sequence number to be used by the sender */

/* called from layer 5 (application layer), passed the message to be sent to other side */
void A_output(struct msg message)
{
  struct pkt sendpkt;
  int i;

  /*figure 3.23 page 255 idea*/
  int window_end = (base + WINDOWSIZE) % SEQSPACE;
  int in_window = 0; 

  /*IF there is no wrap around */
  if (base < window_end){
    if (nextseqnum >= base && nextseqnum < window_end){
      in_window = 1;
    }
  }

  /*IF there is wrap around*/
  else{
    if(nextseqnum >= base || nextseqnum < window_end){
      in_window = 1;
    }
  }

  /*Fig 3.24 "if next sequence number is within the window, create a packet, send it, and store it"*/
  if (in_window){
    if (TRACE > 1)
      printf("----A: New message arrives, send window is not full, send new messge to layer3!\n");

    /* create packet */
    sendpkt.seqnum = nextseqnum;
    sendpkt.acknum = NOTINUSE;
    for ( i=0; i<20 ; i++ ) 
      sendpkt.payload[i] = message.data[i];
    sendpkt.checksum = ComputeChecksum(sendpkt); 

    /*Fig 3.24 "THE packet is stored in the senders buffer for potential retransmission"*/
    /*put packet in window buffer*/
    sender_buffer[nextseqnum].packet = sendpkt;
    sender_buffer[nextseqnum].active = 1;
    sender_buffer[nextseqnum].acked = 0; 

    

    /* send out packet */
    if (TRACE > 0)
      printf("Sending packet %d to layer 3\n", sendpkt.seqnum);
    tolayer3 (A, sendpkt);

    /*fig 3.24 if base == nextseqnum (first unACKed pacekt), start timer*/
    /* start timer if first packet in window */
    if (base == nextseqnum)
      starttimer(A,RTT);

    /* get next sequence number, wrap back to 0 */
    nextseqnum = (nextseqnum + 1) % SEQSPACE;  
  }
  /* if blocked,  window is full */
  else {
    if (TRACE > 0)
      printf("----A: New message arrives, send window is full\n");
    window_full++;
  }
}


/* called from layer 3, when a packet arrives for layer 4 
   In this practical this will always be an ACK as B never sends data.
*/
void A_input(struct pkt packet)
{
  
  /* if received ACK is  corrupted */ 
  if (!IsCorrupted(packet)) {
    if (TRACE > 0){
      printf("----A: uncorrupted ACK %d is received\n",packet.acknum);
    }

    /*Check if this ACK is within the window*/
    if (sender_buffer[packet.acknum].active && !sender_buffer[packet.acknum].acked){
      /* packet is a new ACK */
      if (TRACE > 0){
        printf("----A: ACK %d is not a duplicate\n",packet.acknum);
      }

      sender_buffer[packet.acknum].acked = 1; /*marsk as acked*/
      new_ACKs++;
    
      /*Slide window base forward only if the base is acked*/
      while ( sender_buffer[base].acked && sender_buffer[base].active){
        sender_buffer[base].active = 0; /*clears the buffer slot*/
        base = (base + 1) % SEQSPACE;
      }

      stoptimer(A);
      if (base != nextseqnum){
        starttimer(A, RTT);
      }
    }else{
      if (TRACE > 0){
        printf ("----A: duplicate ACK received, do nothing!\n");
      }
    }
  }else{
    if (TRACE > 0){
      printf ("----A: corrupted ACK is received, do nothing!\n");
    }
  }
}

/*page 256 fig 3.24*/
/* called when A's timer goes off */
void A_timerinterrupt(void)
{ 
  int i;
  
  if (TRACE > 0){
    printf("----A: time out,resend packets!\n");
  }
  
  stoptimer(A);
  
  
  for ( i = 0; i <SEQSPACE; i++){
    if( sender_buffer[i].active && !sender_buffer[i].acked){
      tolayer3(A, sender_buffer[i].packet);
      if (TRACE > 0){
        printf ("---A: resending packet %d\n", sender_buffer[i].packet.seqnum);
      }
    }
  }

  /*Restart the timer for uncack packets*/
  for ( i = 0; i < SEQSPACE; i++){
    if (sender_buffer[i].active && !sender_buffer[i].acked){
      starttimer(A,RTT);
      break;
    }
  }
}
  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(void)
{
  /* initialise A's window, buffer and sequence number */
  nextseqnum = 0;  /* A starts with seq num 0, do not change this */
  windowfirst = 0;
  windowlast = -1;   /* windowlast is where the last packet sent is stored.  
		     new packets are placed in winlast + 1 
		     so initially this is set to -1
		   */
  windowcount = 0;
}



/********* Receiver (B)  variables and procedures ************/

static int expectedseqnum; /* the sequence number expected next by the receiver */
static int B_nextseqnum;   /* the sequence number for the next packets sent by B */
static int rcv_base = 0; /*first seqnum B is expecting*/
int window_end;
bool in_window;

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
  struct pkt sendpkt;
  int i;

  int last_inorder = (rcv_base + SEQSPACE - 1) % SEQSPACE;

  /* if is corrupted resent the last ack to avoid sender timeout */
  if  ( IsCorrupted(packet)) {
    if (TRACE > 0){
      printf("----B: packet corrupted or not expected sequence number, resend ACK!\n");
    }

    /*fig 3.25 SR receiver events and actions point 1) "Packet with sequence number 
    in [rcv_base, rcv_base+N-1] is corectly received....."*/
    sendpkt.seqnum = 0;
    sendpkt.acknum = last_inorder;

    for (i=0; i < 20; i++){
      sendpkt.payload[i] = '0';
    }
    
    sendpkt.checksum = ComputeChecksum(sendpkt);
    tolayer3(B, sendpkt);
    return;
  }

  /*Fig 3.23 Receiver view of sequence numbers*/
    /*Checks if packet is within the reciecver window or not */
  window_end = (rcv_base + WINDOWSIZE) % SEQSPACE;
  in_window = false;

  
  if (rcv_base < window_end){
    if (packet.seqnum >= rcv_base && packet.seqnum < window_end){
      in_window = true ;
    }
  }else {
    if (packet.seqnum >= rcv_base || packet.seqnum < window_end){
      in_window = true;
    }
  }

  /*fig3.25 "If the packet was not previously received, it is buffered. If this packet has a sequence number equal to
    the base of the receive window (rcv_base in Figure 3.22), then this packet,
    and any previously buffered and consecutively numbered (beginning with
    rcv_base) packets are delivered to the upper layer.""*/
  if (in_window && !receiver_buffer[packet.seqnum].received){
    if (TRACE > 0){
      printf("----B: packet %d is correctly received, send ACK!\n",packet.seqnum);
    }

    /*Create buffer*/
    receiver_buffer[packet.seqnum].packet = packet;
    receiver_buffer[packet.seqnum].received = 1;
    packets_received++;

    /*ACK this packet*/
    sendpkt.seqnum = 0;
    sendpkt.acknum = packet.seqnum;

    for ( i = 0; i < 20; i++){
      sendpkt.payload[i] = '0';
    }

    sendpkt.checksum = ComputeChecksum(sendpkt);
    tolayer3(B, sendpkt);

    /*if n == rcv_base, deliver bufferedm in order packets to layer 5 and advance rcv_base fig 3.25*/
    while (receiver_buffer[rcv_base].received){
      tolayer5(B, receiver_buffer[rcv_base].packet.payload);
      receiver_buffer[rcv_base].received = 0;
      rcv_base = (rcv_base + 1) % SEQSPACE;
    }

  }else {
    /* packet is corrupted or out of order resend last ACK */
    if (TRACE > 0){
      printf("----B: packet corrupted or not expected sequence number, resend ACK!\n");
    }

    sendpkt.seqnum = 0;
    sendpkt.acknum = last_inorder;


    /* we don't have any data to send.  fill payload with 0's */
    for ( i=0; i<20 ; i++ ){
      sendpkt.payload[i] = '0';  
    }

    /* computer checksum */
    sendpkt.checksum = ComputeChecksum(sendpkt); 

    /* send out packet */
    tolayer3 (B, sendpkt);
      
  } 
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init(void)
{
  expectedseqnum = 0;
  B_nextseqnum = 1;
}

/******************************************************************************
 * The following functions need be completed only for bi-directional messages *
 *****************************************************************************/

/* Note that with simplex transfer from a-to-B, there is no B_output() */
void B_output(struct msg message)  
{
}

/* called when B's timer goes off */
void B_timerinterrupt(void)
{
}

