#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "emulator.h"
#include "sr.h"



#define RTT  16.0       /* round trip time.  MUST BE SET TO 16.0 when submitting assignment */
#define WINDOWSIZE 6    /* the maximum number of buffered unacked packet */
#define SEQSPACE 12      /* the min sequence space for sr is 2 * windowsize  */
#define NOTINUSE (-1)   /* used to fill header fields that are not being used */

/****************************************************
 *Adding some global varables for the selective repeat functions
 
 ****************************************************/
/*SENDER*/
 struct sender_packet {
  struct pkt packet;
  int acked;
  int active;
};

static struct sender_packet sender_buffer[SEQSPACE];
static int base = 0;
static int nextseqnum = 0; 

/*RECEIVER*/
struct receiver_slot {
  struct pkt packet;
  int received;
};

static struct receiver_slot receiver_buffer[SEQSPACE];
static int rcv_base = 0;


/* generic procedure to compute the checksum of a packet.  Used by both sender and receiver  
   the simulator will overwrite part of your packet with 'z's.  It will not overwrite your 
   original checksum.  This procedure must generate a different checksum to the original if
   the packet is corrupted.
*/
int ComputeChecksum(struct pkt packet)
{
  int sum = packet.seqnum + packet.acknum;
  int i;

  for ( i=0; i<20; i++ ) 
    sum += (int)(packet.payload[i]);
  return sum;
}

bool IsCorrupted(struct pkt packet){
  if (packet.checksum == ComputeChecksum(packet))
    return (false);
  else
    return (true);
}


/********* Sender (A) variables and functions ************/

/* called from layer 5 (application layer), passed the message to be sent to other side */
void A_output(struct msg message)
{
  struct pkt sendpkt;
  bool in_window;
  int window_end = (base + WINDOWSIZE) % SEQSPACE;
  int i;
  

  /*IF there is no wrap around */
  if (base < window_end)
    in_window = (nextseqnum >= base && nextseqnum < window_end);
  else
    in_window = (nextseqnum >= base || nextseqnum < window_end);
  
  if(in_window){  /*Fig 3.24 "if next sequence number is within the window, create a packet, send it, and store it"*/
    if (TRACE > 1)
      printf("----A: New message arrives, send window is not full, send new messge to layer3!\n");
    
    /* create packet */
    sendpkt.seqnum = nextseqnum;
    sendpkt.acknum = NOTINUSE;

    for ( i=0; i<20 ; i++ ) 
      sendpkt.payload[i] = message.data[i];
    sendpkt.checksum = ComputeChecksum(sendpkt); 

    /*Fig 3.24 "THE packet is stored in the senders buffer for potential retransmission"*/
    /*put packet in  buffer*/
    sender_buffer[nextseqnum].packet = sendpkt;
    sender_buffer[nextseqnum].active = 1;
    sender_buffer[nextseqnum].acked = 0; 


    /* send out packet */
    tolayer3 (A, sendpkt);
    if (TRACE > 0)
      printf("Sending packet %d to layer 3\n", sendpkt.seqnum);
    

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
void A_input(struct pkt packet){
  
  if (IsCorrupted(packet)) {
    if (TRACE > 0)
      printf("----A: corrupted ACK is received, do nothing!\n");
    return;
  }
  
  if (TRACE > 0)
    printf("----A: uncorrupted ACK %d is received\n", packet.acknum);
  
  
  /*if we have a new ack for an active packet*/
  /*Check if this ACK is within the window*/
  
  if (sender_buffer[packet.acknum].active && !sender_buffer[packet.acknum].acked){
      
      if (TRACE > 0)
        printf("----A: ACK %d is not a duplicate\n",packet.acknum);
      new_ACKs++;
      sender_buffer[packet.acknum].acked = 1;
  }else {
    if (TRACE > 0)
      printf("----A: duplicate ACK received, do nothing!\n");
    return;
  }

    /*Slide base forward*/
    /*Slide window base forward only if the base is acked*/
    while (sender_buffer[base].active && sender_buffer[base].acked){
      sender_buffer[base].active = 0; /*clears the buffer slot*/
      base = (base + 1) % SEQSPACE;
    }

    stoptimer(A);
    if (base != nextseqnum)
    starttimer(A, RTT);
   
}


/*page 256 fig 3.24*/
/* called when A's timer goes off */
void A_timerinterrupt(void){ 

  if (TRACE > 0)
    printf("----A: time out,resend packets!\n");
  tolayer3(A, sender_buffer[base].packet);

  if (TRACE > 0)
    printf ("----A: resending packet %d\n", sender_buffer[base].packet.seqnum);

  starttimer(A, RTT); /*Restarts its timer*/
}
  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(void)
{
 int i; 
 base = 0;
 nextseqnum = 0; 
 
 for ( i = 0; i < SEQSPACE; i++){
    sender_buffer[i].active = 0;
    sender_buffer[i].acked = 0; 
 }
}


/********* Receiver (B)  variables and procedures ************/

static int expectedseqnum; /* the sequence number expected next by the receiver */
static int B_nextseqnum;   /* the sequence number for the next packets sent by B */
bool in_window;

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
  struct pkt ackpkt;
  int last_inorder = (rcv_base + SEQSPACE - 1) % SEQSPACE;
  int window_end = (rcv_base + WINDOWSIZE) % SEQSPACE;
  bool in_window; 

  /*Fig 3.23 Receiver view of sequence numbers*/
  if (rcv_base < window_end)
    in_window = (packet.seqnum >= rcv_base && packet.seqnum < window_end);
  else
    in_window = (packet.seqnum >= rcv_base || packet.seqnum < window_end);
  

  /* if is corrupted resent the last ack to avoid sender timeout */
  if  ( IsCorrupted(packet) || !in_window) {
    /* packet is corrupted or out of order resend last ACK */
    if (TRACE > 0){
      printf("----B: packet corrupted or not expected sequence number, resend ACK!\n");
    }
    /*fig 3.25 SR receiver events and actions point 1) "Packet with sequence number 
    in [rcv_base, rcv_base+N-1] is corectly received....."*/
    ackpkt.seqnum = 0;
    ackpkt.acknum = last_inorder;    
    ackpkt.checksum = ComputeChecksum(ackpkt);
    tolayer3(B, ackpkt);
    return;
  }
     
  /*fig3.25 "If the packet was not previously received, it is buffered. If this packet has a sequence number equal to
    the base of the receive window (rcv_base in Figure 3.22), then this packet,
    and any previously buffered and consecutively numbered (beginning with
    rcv_base) packets are delivered to the upper layer.""*/
  if (!receiver_buffer[packet.seqnum].received){
    if (TRACE > 0)
      printf("----B: packet %d is correctly received, send ACK!\n",packet.seqnum);
    
    /*Create buffer*/
    receiver_buffer[packet.seqnum].packet = packet;
    receiver_buffer[packet.seqnum].received = 1;
    packets_received++;

    /*ACK this packet*/
    ackpkt.seqnum = 0;
    ackpkt.acknum = packet.seqnum;
    ackpkt.checksum = ComputeChecksum(ackpkt);
    tolayer3(B, ackpkt);

    /*if n == rcv_base, deliver bufferedm in order packets to layer 5 and advance rcv_base fig 3.25*/
    while (receiver_buffer[rcv_base].received){
      tolayer5(B, receiver_buffer[rcv_base].packet.payload);
      receiver_buffer[rcv_base].received = 0;
      rcv_base = (rcv_base + 1) % SEQSPACE;
    } 
  } 
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init(void)
{
  int i;
  rcv_base = 0;
  for ( i = 0; i < SEQSPACE; i++)
    receiver_buffer[i].received = 0; 
  
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
