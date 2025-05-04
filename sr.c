#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "emulator.h"
#include "sr.h"

/* ******************************************************************
   Selective repeat  -using the textbook computer network up and down appoarch as reference 
   3.4.4 Slective Repeat (SR) page  254

   Brainstorming: 
   1) Understanding the difference between GBN and SR 
   2) Spot the code where they are different 
   3) change the code to the correct method/apporach 

   Removels: 
   - all gbn implementations 
   - all gbn comments 


**********************************************************************/
extern int total_ACKs_received; 

#define RTT  16.0       /* round trip time.  MUST BE SET TO 16.0 when submitting assignment */
#define WINDOWSIZE 6    /* the maximum number of buffered unacked packet */
#define SEQSPACE (WINDOWSIZE*2)   /* the min sequence space for SR must be double of windowsize */
#define NOTINUSE (-1)   /* used to fill header fields that are not being used */
#define MAX_BUFFERED_MSGS 100

static struct  msg msg_buffer[MAX_BUFFERED_MSGS];
static int buffer_first = 0;
static int buffer_last = 0;


/* generic procedure to compute the checksum of a packet.  Used by both sender and receiver  
   the simulator will overwrite part of your packet with 'z's.  It will not overwrite your 
   original checksum.  This procedure must generate a different checksum to the original if
   the packet is corrupted.
*/

/*Checksum - used to detect bit errors in a transmitted packet*/
int ComputeChecksum(struct pkt packet)
{
  int checksum = packet.seqnum + packet.acknum;
  int i;

  for ( i=0; i<20 ; i++ )checksum += packet.payload[i];
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

static struct pkt buffer[SEQSPACE];  /* array for storing packets waiting for ACK */
static int send[SEQSPACE];   /* 0 = not send, i = send*/
int acked[SEQSPACE];  /* 0 = not acked, 1 = acked*/
static int windowfirst = 0;                /* the number of packets currently awaiting an ACK */
static int A_nextseqnum = 0;               /* the next sequence number to be used by the sender */
static int timer_running = 0; 

bool isInWindow(int base, int x){
  return((x-base+SEQSPACE) % SEQSPACE) < WINDOWSIZE;
}

/* called from layer 5 (application layer), passed the message to be sent to other side */
void A_output(struct msg message)
{
  struct pkt sendpkt;
  int i;
  int index = A_nextseqnum;

  /* if not blocked waiting on ACK */
  if ( isInWindow(windowfirst, A_nextseqnum)){
    if (TRACE > 1)
      printf("----A: New message arrives, send window is not full, send new messge to layer3!\n");

    /* create packet */
    sendpkt.seqnum = A_nextseqnum;
    sendpkt.acknum = -1;
    for ( i=0; i<20 ; i++ ) 
      sendpkt.payload[i] = message.data[i];
    sendpkt.checksum = ComputeChecksum(sendpkt); 

    /* WE save in send window */
    buffer[index] = sendpkt;
    send[index] = 1;
    acked[index] = 0;

    /* send out packet to layer 3 */
    if (TRACE > 0)
      printf("Sending packet %d to layer 3\n", sendpkt.seqnum);
    tolayer3 (A, sendpkt);

    /* start timer if its the base packet */
    if (windowfirst == A_nextseqnum)
      starttimer(A,RTT);

    /* get next sequence number, wrap back to 0 */
    A_nextseqnum = (A_nextseqnum + 1) % SEQSPACE;  
  }
  /* if blocked,  window is full */
  else {
    if((buffer_last + 1 ) % MAX_BUFFERED_MSGS != buffer_first){
      msg_buffer[buffer_last] = message;
      buffer_last = (buffer_last + 1) % MAX_BUFFERED_MSGS;
    }
    if (TRACE > 0)
      printf("----A: New message arrives, send window is full\n");
    /*window_full++;*/
  }
}


/* called from layer 3, when a packet arrives for layer 4 
   In this practical this will always be an ACK as B never sends data.
*/
void A_input(struct pkt packet)
{
  int index;

  /* if received ACK is not corrupted */ 
  if (!IsCorrupted(packet) && packet.acknum >= 0 && packet.acknum < SEQSPACE) {
    index = packet.acknum;
    if (TRACE > 0)
      printf("----A: uncorrupted ACK %d is received\n",packet.acknum);
    total_ACKs_received++;

    /* Mark the packet as ACKed*/
    if (!acked[index]){
        acked[index] = 1;
    }

    /* Slide base only if the base packet is now ACked*/
    while (acked[windowfirst]){
        acked[windowfirst] = 0; /* RESET AFTER SLIDING*/
        send[windowfirst] = 0; 
        windowfirst = (windowfirst + 1) % SEQSPACE;
    }
    /* packet is a new ACK */
    if (TRACE > 0)
        printf("----A: ACK %d is not a duplicate\n",packet.acknum);
    new_ACKs++;

    /* start timer again if there are still more unacked packets in window */
    stoptimer(0);
    timer_running = 0; 

    if (windowfirst != A_nextseqnum){
        starttimer(A, RTT);
        timer_running = 1;
    }

    while (isInWindow(windowfirst, A_nextseqnum) && buffer_first != buffer_last){
      struct  msg next_msg = msg_buffer[buffer_first];
      buffer_first = (buffer_first + 1) % MAX_BUFFERED_MSGS;
      A_output(next_msg);
    }
      }if (TRACE > 0)
          printf ("----A: duplicate ACK received, do nothing!\n");
      
        else 
            if (TRACE > 0)
            printf ("----A: corrupted ACK is received, do nothing!\n");
}

/* called when A's timer goes off */
void A_timerinterrupt(void)
{
  int i;

  if (TRACE > 0)
    printf("----A: time out,resend packets!\n");

  for(i=0; i<WINDOWSIZE; i++) {
    int index = (windowfirst + i) % SEQSPACE;
    if (send[index] && !acked[index]){
      if (TRACE > 0)
        printf ("---A: resending packet %d\n", (buffer[(windowfirst+i) % WINDOWSIZE]).seqnum);
      tolayer3(A, buffer[index]);

    }
  }
  starttimer(A,RTT);
}       



/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init(void)
{
    int i ; 
    /* initialise A's window, buffer and sequence number */
    A_nextseqnum = 0;  /* A starts with seq num 0, do not change this */
    windowfirst = 0; 
    buffer_first = 0;
    buffer_last= 0;
  
    for ( i = 0; i < SEQSPACE; i++){
        send[i] = 0;
        acked[i] = 0;
    }
}



/********* Receiver (B)  variables and procedures ************/
struct pkt received_window[SEQSPACE]; 
static int received[SEQSPACE]; /* 0 = not received, 1 = received */
static int rcv_base = 0 ; 

void send_ack(int acknum){
  int i;
  struct  pkt ackpkt;
  ackpkt.seqnum = 0;
  ackpkt.acknum = acknum;
  for ( i = 0; i < 20; i++) ackpkt.payload[i] = '0';
  ackpkt.checksum = ComputeChecksum(ackpkt);
  tolayer3(B, ackpkt);
}

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
  int seq = packet.seqnum;

  /* if not corrupted and received packet is in order */
  if  ( !IsCorrupted(packet) ) {
    if (TRACE > 0)
      printf("----B: packet %d is correctly received, send ACK!\n",packet.seqnum);
    if(isInWindow(rcv_base, seq)){
      if(!received[seq]){
        received_window[seq] =packet;
        received[seq] = 1;
        packets_received++;
      }
      send_ack(seq);
      
      while (received[rcv_base]){
        tolayer5(B, received_window[rcv_base].payload);
        received[rcv_base] = 0;
        rcv_base = (rcv_base +1 ) % SEQSPACE;
      }
    } else if ((rcv_base - seq + SEQSPACE) % SEQSPACE < WINDOWSIZE){
      send_ack(seq);
    }
  }else{
    if (TRACE > 0) 
        printf("----B: packet corrupted or not expected sequence number, resend ACK!\n");

  }
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init(void)
{
    int i;
    rcv_base = 0;
    
    for (i = 0; i < SEQSPACE; i++){
        received[i] = 0;
    }
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
