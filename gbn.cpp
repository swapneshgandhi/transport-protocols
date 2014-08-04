#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <locale>
#include <cstring>
#include <vector>

#define BIDIRECTIONAL 0    /* change to 1 if you're doing extra credit */
/* and write a routine called B_output */

#define  TIMER_INTERRUPT 0
#define  FROM_LAYER5     1
#define  FROM_LAYER3     2

#define  OFF             0
#define  ON              1
#define   A    0
#define   B    1
#define MAX_MSG_LEN 20
#define Window_size 50

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */

struct msg {
	char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
	int seqnum;
	int acknum;
	int checksum;
	char payload[20];
};

//packet info structure at the sender
struct pkt_info{
	pkt packet;
	bool acknowleged;
	bool sent;	//true of packet has been sent at least once. required to make sure only new packets are sent
	//when a new packet is recevied for layer 5
};

typedef struct msg msg;
typedef struct pkt pkt;
typedef struct pkt_info pkt_info;

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/


int TRACE = 1;             /* for my debugging */
int nsim = 0;              /* number of messages from 5 to 4 so far */
int nsimmax = 0;           /* number of msgs to generate, then stop */
float time_local = 0.0;
float lossprob = 0.0; /* probability that a packet is dropped */
float corruptprob = 0.0; /* probability that one bit is packet is flipped */
float lambda = 0.0; /* arrival rate of messages from layer 5 */
int ntolayer3 = 0; /* number sent into layer 3 */
int nlost = 0; /* number lost in media */
int ncorrupt = 0; /* number corrupted by media*/
float timeout=25.0;
pkt_info *sender;
pkt_info *receiver;

//start of sender's window
int send_win_strt;
//index to next empty location in the buffer
int next_empty_sender_loc;
//counts to count the messages sent to either side
int A_side_transport_layer_counter;
int B_side_transport_layer_counter;

//next expected ack at sender
int send_exp_seq_no=-1;
//next expected seqnum at receiver
int recv_exp_seq_no;
//keeps track of next seq no for the packet.
int A_seq_num;
//tells if timer is already on or not
bool timer_on=false;

//last ack. at receiver sent whenever a invalid or duplicate packet received
pkt last_ack;

//moved the declarations up so as to use them
void tolayer5(int AorB,char *datasent);
void starttimer(int AorB,float increment);
void stoptimer(int AorB);
void tolayer3(int AorB,struct pkt packet);

//sends the message form the buffer for given sequence no.
void send(int seq){
	printf("Sending message %d\n",seq);
	tolayer3(A,sender[seq].packet);
	sender[seq].acknowleged=false;
	sender[seq].sent=true;
	A_side_transport_layer_counter++;

}

//called when timer goes off sends all the messages
void re_send(){

	int i=0;
	int count=send_win_strt;
	while(i < Window_size && count < next_empty_sender_loc  && sender[count].sent){

		if(timer_on){
			stoptimer(A);
		}
		starttimer(A,timeout);
		timer_on=true;
		send(count);
		count++;
		i++;
	}

	printf("sent the messages\n");

}

//checks if any new message is waiting in the buffer to be sent
void check_and_send(){
	int i=send_win_strt;
	int count=0;

	while(count<Window_size && i < next_empty_sender_loc && sender[i].packet.acknum==-1){
		if(!sender[i].sent)
		{
			if(timer_on){
				stoptimer(A);
			}
			starttimer(A,timeout);
			timer_on=true;
			send(i);
			sender[i].sent=true;
		}
		i++;
		count++;
		send_exp_seq_no=i-1;
	}

}


void A_output(struct msg message) //ram's comment - students can change the return type of the function from struct to pointers if necessary
{
	pkt packet;
	message.data[MAX_MSG_LEN-1]='\0'; 		//the simulator does not null terminate the message while copying it from packet.
	static int count;
	std::locale loc;                 // the "C" locale
	//calculate the hash
	const std::collate<char>& coll = std::use_facet<std::collate<char> >(loc);
	int myhash = (int)coll.hash(message.data,message.data+strlen(message.data));

	//fill up and buffer the packet
	packet.acknum=-1;
	packet.checksum=(int) myhash;
	packet.seqnum=A_seq_num++;
	strcpy(packet.payload,message.data);
	sender[next_empty_sender_loc].packet=packet;
	sender[next_empty_sender_loc].acknowleged=false;
	sender[next_empty_sender_loc].sent=false;
	next_empty_sender_loc++;
	//check if packet can be sent right away
	check_and_send();
}


void B_output(struct msg message)  /* need be completed only for extra credit */
// ram's comment - students can change the return type of this function from struct to pointers if needed
{

}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
	static int count;

	//if a valid ack is received then moves the start of the window
	if((packet.acknum)>= send_win_strt && packet.checksum==packet.acknum){
		//move the window
		send_win_strt+=(packet.acknum-send_win_strt+1);
		//ok_to_send_new_msgs=true;
		printf("recved ack %d", packet.acknum);
		if (timer_on){
			timer_on=false;
			stoptimer(A);
		}
		//checks if buffered messages can be sent now
		check_and_send();
	}
}

/* called when A's timer goes off */
void A_timerinterrupt() //ram's comment - changed the return type to void.
{
	timer_on=false;
	printf("Timer has gone off\n");
	printf("Sending old messages\n");
	re_send();
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() //ram's comment - changed the return type to void.
{
	sender=new pkt_info[nsimmax];

	if(lossprob > 0.4 && lossprob <0.8){
		timeout=20.0;
	}
	else if(lossprob > 0.6 && Window_size==50){
		timeout=16.0;
	}
	else if(lossprob > 0.6){
		timeout=13.0;
	}
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() //ram's comment - changed the return type to void.
{
	receiver=new pkt_info[Window_size];
	last_ack.acknum=-1;
	recv_exp_seq_no=0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{

	//checksum check..
	std::locale loc;                 // the "C" locale
	const std::collate<char>& coll = std::use_facet<std::collate<char> >(loc);
	int myhash = (int)coll.hash(packet.payload,packet.payload+strlen(packet.payload));
	B_side_transport_layer_counter++;

	//if expected packet received then move the expected seq no and send the ack
	if(packet.seqnum == recv_exp_seq_no && myhash == packet.checksum){
		packet.acknum=recv_exp_seq_no;
		packet.checksum=recv_exp_seq_no;
		printf("sending ack %d\n",packet.acknum);
		tolayer3(B,packet);
		tolayer5(B,packet.payload);
		recv_exp_seq_no++;
		last_ack=packet;
	}

	//if higher than expected seq no. packet is received then don't send ack but buffer the packet
	//although sender will send but what if they are dropped??
	else if (packet.seqnum < (recv_exp_seq_no+Window_size) && packet.seqnum > recv_exp_seq_no && myhash == packet.checksum){
		receiver[packet.seqnum%Window_size].packet=packet;
		receiver[packet.seqnum%Window_size].acknowleged=true;
		printf("Buffering packet\n");
	}

	//else send last ack
	else{
		printf("received invlid packet dropping the packet sending last_ack\n");
		tolayer3(B,last_ack);
	}

	//check if any packet buffered earlier can be delivered to layer5 now..
	for (int i=(recv_exp_seq_no%Window_size);i<Window_size;i++){
		if(receiver[i].acknowleged==true){
			receiver[i].acknowleged=false;
			packet.acknum=recv_exp_seq_no;
			packet.checksum=recv_exp_seq_no;
			printf("sending ack %d, for buffered packets\n",packet.acknum);
			tolayer3(B,packet);
			tolayer5(B,packet.payload);
			recv_exp_seq_no++;
			last_ack=packet;
		}
		else{
			break;
		}
	}
}

/* called when B's timer goes off */
void B_timerinterrupt() //ram's comment - changed the return type to void.
{
}



/****************************************************************************/
/* jimsrand(): return a float in range [0,1].  The routine below is used to */
/* isolate all random number generation in one location.  We assume that the*/
/* system-supplied rand() function return an int in therange [0,mmm]        */
/****************************************************************************/
float jimsrand()
{
	double mmm = 2147483647;   /* largest int  - MACHINE DEPENDENT!!!!!!!!   */
	float x;                   /* individual students may need to change mmm */
	x = rand()/mmm;            /* x should be uniform in [0,1] */
	return(x);
}


/*****************************************************************
 ***************** NETWORK EMULATION CODE IS BELOW ***********
The code below emulates the layer 3 and below network environment:
  - emulates the tranmission and delivery (possibly with bit-level corruption
    and packet loss) of packets across the layer 3/4 interface
  - handles the starting/stopping of a timer, and generates timer
    interrupts (resulting in calling students timer handler).
  - generates message to be sent (passed from later 5 to 4)

THERE IS NOT REASON THAT ANY STUDENT SHOULD HAVE TO READ OR UNDERSTAND
THE CODE BELOW.  YOU SHOLD NOT TOUCH, OR REFERENCE (in your code) ANY
OF THE DATA STRUCTURES BELOW.  If you're interested in how I designed
the emulator, you're welcome to look at the code - but again, you should have
to, and you defeinitely should not have to modify
 ******************************************************************/



/* possible events: */


struct event {
	float evtime;           /* event time */
	int evtype;             /* event type code */
	int eventity;           /* entity where event occurs */
	struct pkt *pktptr;     /* ptr to packet (if any) assoc w/ this event */
	struct event *prev;
	struct event *next;
};

struct event *evlist = NULL;   /* the event list */


void insertevent(struct event *p)
{
	struct event *q,*qold;

	if (TRACE>2) {
		printf("            INSERTEVENT: time is %lf\n",time_local);
		printf("            INSERTEVENT: future time will be %lf\n",p->evtime);
	}
	q = evlist;     /* q points to header of list in which p struct inserted */
	if (q==NULL) {   /* list is empty */
		evlist=p;
		p->next=NULL;
		p->prev=NULL;
	}
	else {
		for (qold = q; q !=NULL && p->evtime > q->evtime; q=q->next)
			qold=q;
		if (q==NULL) {   /* end of list */
			qold->next = p;
			p->prev = qold;
			p->next = NULL;
		}
		else if (q==evlist) { /* front of list */
			p->next=evlist;
			p->prev=NULL;
			p->next->prev=p;
			evlist = p;
		}
		else {     /* middle of list */
			p->next=q;
			p->prev=q->prev;
			q->prev->next=p;
			q->prev=p;
		}
	}
}





/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

void generate_next_arrival()
{
	double x,log(),ceil();
	struct event *evptr;
	//    //char *malloc();
	float ttime;
	int tempint;

	if (TRACE>2)
		printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

	x = lambda*jimsrand()*2;  /* x is uniform on [0,2*lambda] */
	/* having mean of lambda        */

	evptr = (struct event *)malloc(sizeof(struct event));
	evptr->evtime =  time_local + x;
	evptr->evtype =  FROM_LAYER5;
	if (BIDIRECTIONAL && (jimsrand()>0.5) )
		evptr->eventity = B;
	else
		evptr->eventity = A;
	insertevent(evptr);
}





void init()                         /* initialize the simulator */
{
	int i;
	float sum, avg;
	float jimsrand();


	printf("-----  Stop and Wait Network Simulator Version 1.1 -------- \n\n");
	printf("Enter the number of messages to simulate: ");
	scanf("%d",&nsimmax);
	printf("Enter  packet loss probability [enter 0.0 for no loss]:");
	scanf("%f",&lossprob);
	printf("Enter packet corruption probability [0.0 for no corruption]:");
	scanf("%f",&corruptprob);
	printf("Enter average time between messages from sender's layer5 [ > 0.0]:");
	scanf("%f",&lambda);
	printf("Enter TRACE:");
	scanf("%d",&TRACE);

	srand(9999);              /* init random number generator */
	sum = 0.0;                /* test random number generator for students */
	for (i=0; i<1000; i++)
		sum=sum+jimsrand();    /* jimsrand() should be uniform in [0,1] */
	avg = sum/1000.0;
	if (avg < 0.25 || avg > 0.75) {
		printf("It is likely that random number generation on your machine\n" );
		printf("is different from what this emulator expects.  Please take\n");
		printf("a look at the routine jimsrand() in the emulator code. Sorry. \n");
		exit(0);
	}

	ntolayer3 = 0;
	nlost = 0;
	ncorrupt = 0;

	time_local=0;                    /* initialize time to 0.0 */
	generate_next_arrival();     /* initialize event list */
}






//int TRACE = 1;             /* for my debugging */
//int nsim = 0;              /* number of messages from 5 to 4 so far */
//int nsimmax = 0;           /* number of msgs to generate, then stop */
//float time = 0.000;
//float lossprob;            /* probability that a packet is dropped  */
//float corruptprob;         /* probability that one bit is packet is flipped */
//float lambda;              /* arrival rate of messages from layer 5 */
//int   ntolayer3;           /* number sent into layer 3 */
//int   nlost;               /* number lost in media */
//int ncorrupt;              /* number corrupted by media*/

int main()
{
	struct event *eventptr;
	struct msg  msg2give;
	struct pkt  pkt2give;

	int i,j;
	char c;

	init();
	A_init();
	B_init();

	while (1) {
		eventptr = evlist;            /* get next event to simulate */
		if (eventptr==NULL)
			goto terminate;
		evlist = evlist->next;        /* remove this event from event list */
		if (evlist!=NULL)
			evlist->prev=NULL;
		if (TRACE>=2) {
			printf("\nEVENT time: %f,",eventptr->evtime);
			printf("  type: %d",eventptr->evtype);
			if (eventptr->evtype==0)
				printf(", timerinterrupt  ");
			else if (eventptr->evtype==1)
				printf(", fromlayer5 ");
			else
				printf(", fromlayer3 ");
			printf(" entity: %d\n",eventptr->eventity);
		}
		time_local = eventptr->evtime;        /* update time to next event time */
		if (nsim==nsimmax)
			break;                        /* all done with simulation */
		if (eventptr->evtype == FROM_LAYER5 ) {
			generate_next_arrival();   /* set up future arrival */
			/* fill in msg to give with string of same letter */
			j = nsim % 26;
			for (i=0; i<20; i++)
				msg2give.data[i] = 97 + j;
			if (TRACE>2) {
				printf("          MAINLOOP: data given to student: ");
				for (i=0; i<20; i++)
					printf("%c", msg2give.data[i]);
				printf("\n");
			}
			nsim++;
			if (eventptr->eventity == A)
				A_output(msg2give);
			else
				B_output(msg2give);
		}
		else if (eventptr->evtype ==  FROM_LAYER3) {
			pkt2give.seqnum = eventptr->pktptr->seqnum;
			pkt2give.acknum = eventptr->pktptr->acknum;
			pkt2give.checksum = eventptr->pktptr->checksum;
			for (i=0; i<20; i++)
				pkt2give.payload[i] = eventptr->pktptr->payload[i];
			if (eventptr->eventity ==A)      /* deliver packet by calling */
				A_input(pkt2give);            /* appropriate entity */
			else
				B_input(pkt2give);
			free(eventptr->pktptr);          /* free the memory for packet */
		}
		else if (eventptr->evtype ==  TIMER_INTERRUPT) {
			if (eventptr->eventity == A)
				A_timerinterrupt();
			else
				B_timerinterrupt();
		}
		else  {
			printf("INTERNAL PANIC: unknown event type \n");
		}
		free(eventptr);
	}

	terminate:
	printf(" Simulator terminated at time %f\n after sending %d msgs from layer5\n",time_local,nsim);
	printf("Protocol: Go Back-N\n");
	printf("%d of packets sent from the Application Layer of Sender A\n",next_empty_sender_loc);
	printf("%d of packets sent from the Transport Layer of Sender A\n",A_side_transport_layer_counter);
	printf("%d packets received at the Transport layer of Receiver B\n",B_side_transport_layer_counter);
	printf("%d of packets received at the Application layer of Receiver B\n",recv_exp_seq_no);
	printf("Total time: %f time units\n",time_local);
	float Throughput=(float)recv_exp_seq_no/time_local;
	printf("Throughput = %.5f packets/time units\n",Throughput);
}


/********************* EVENT HANDLINE ROUTINES *******/
/*  The next set of routines handle the event list   */
/*****************************************************/

/*void generate_next_arrival()
{
   double x,log(),ceil();
   struct event *evptr;
    //char *malloc();
   float ttime;
   int tempint;

   if (TRACE>2)
       printf("          GENERATE NEXT ARRIVAL: creating new arrival\n");

   x = lambda*jimsrand()*2;  // x is uniform on [0,2*lambda]
                             // having mean of lambda
   evptr = (struct event *)malloc(sizeof(struct event));
   evptr->evtime =  time + x;
   evptr->evtype =  FROM_LAYER5;
   if (BIDIRECTIONAL && (jimsrand()>0.5) )
      evptr->eventity = B;
    else
      evptr->eventity = A;
   insertevent(evptr);
} */




void printevlist()
{
	struct event *q;
	int i;
	printf("--------------\nEvent List Follows:\n");
	for(q = evlist; q!=NULL; q=q->next) {
		printf("Event time: %f, type: %d entity: %d\n",q->evtime,q->evtype,q->eventity);
	}
	printf("--------------\n");
}



/********************** Student-callable ROUTINES ***********************/

/* called by students routine to cancel a previously-started timer */
void stoptimer(int AorB)
//AorB;  /* A or B is trying to stop timer */
{
	struct event *q,*qold;

	if (TRACE>2)
		printf("          STOP TIMER: stopping timer at %f\n",time_local);
	/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
	for (q=evlist; q!=NULL ; q = q->next)
		if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) {
			/* remove this event */
			if (q->next==NULL && q->prev==NULL)
				evlist=NULL;         /* remove first and only event on list */
			else if (q->next==NULL) /* end of list - there is one in front */
				q->prev->next = NULL;
			else if (q==evlist) { /* front of list - there must be event after */
				q->next->prev=NULL;
				evlist = q->next;
			}
			else {     /* middle of list */
				q->next->prev = q->prev;
				q->prev->next =  q->next;
			}
			free(q);
			return;
		}
	printf("Warning: unable to cancel your timer. It wasn't running.\n");
}


void starttimer(int AorB,float increment)
// AorB;  /* A or B is trying to stop timer */

{

	struct event *q;
	struct event *evptr;
	////char *malloc();

	if (TRACE>2)
		printf("          START TIMER: starting timer at %f\n",time_local);
	/* be nice: check to see if timer is already started, if so, then  warn */
	/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next)  */
	for (q=evlist; q!=NULL ; q = q->next)
		if ( (q->evtype==TIMER_INTERRUPT  && q->eventity==AorB) ) {
			printf("Warning: attempt to start a timer that is already started\n");
			return;
		}

	/* create future event for when timer goes off */
	evptr = (struct event *)malloc(sizeof(struct event));
	evptr->evtime =  time_local + increment;
	evptr->evtype =  TIMER_INTERRUPT;
	evptr->eventity = AorB;
	insertevent(evptr);
}


/************************** TOLAYER3 ***************/
void tolayer3(int AorB,struct pkt packet)
{
	struct pkt *mypktptr;
	struct event *evptr,*q;
	////char *malloc();
	float lastime, x, jimsrand();
	int i;


	ntolayer3++;

	/* simulate losses: */
	if (jimsrand() < lossprob)  {
		nlost++;
		if (TRACE>0)
			printf("          TOLAYER3: packet being lost\n");
		return;
	}

	/* make a copy of the packet student just gave me since he/she may decide */
	/* to do something with the packet after we return back to him/her */
	mypktptr = (struct pkt *)malloc(sizeof(struct pkt));
	mypktptr->seqnum = packet.seqnum;
	mypktptr->acknum = packet.acknum;
	mypktptr->checksum = packet.checksum;
	for (i=0; i<20; i++)
		mypktptr->payload[i] = packet.payload[i];
	if (TRACE>2)  {
		printf("          TOLAYER3: seq: %d, ack %d, check: %d ", mypktptr->seqnum,
				mypktptr->acknum,  mypktptr->checksum);
		for (i=0; i<20; i++)
			printf("%c",mypktptr->payload[i]);
		printf("\n");
	}

	/* create future event for arrival of packet at the other side */
	evptr = (struct event *)malloc(sizeof(struct event));
	evptr->evtype =  FROM_LAYER3;   /* packet will pop out from layer3 */
	evptr->eventity = (AorB+1) % 2; /* event occurs at other entity */
	evptr->pktptr = mypktptr;       /* save ptr to my copy of packet */
	/* finally, compute the arrival time of packet at the other end.
   medium can not reorder, so make sure packet arrives between 1 and 10
   time units after the latest arrival time of packets
   currently in the medium on their way to the destination */
	lastime = time_local;
	/* for (q=evlist; q!=NULL && q->next!=NULL; q = q->next) */
	for (q=evlist; q!=NULL ; q = q->next)
		if ( (q->evtype==FROM_LAYER3  && q->eventity==evptr->eventity) )
			lastime = q->evtime;
	evptr->evtime =  lastime + 1 + 9*jimsrand();



	/* simulate corruption: */
	if (jimsrand() < corruptprob)  {
		ncorrupt++;
		if ( (x = jimsrand()) < .75)
			mypktptr->payload[0]='Z';   /* corrupt payload */
		else if (x < .875)
			mypktptr->seqnum = 999999;
		else
			mypktptr->acknum = 999999;
		if (TRACE>0)
			printf("          TOLAYER3: packet being corrupted\n");
	}

	if (TRACE>2)
		printf("          TOLAYER3: scheduling arrival on other side\n");
	insertevent(evptr);
}

void tolayer5(int AorB,char *datasent)
{

	int i;
	if (TRACE>2) {
		printf("          TOLAYER5: data received: ");
		for (i=0; i<20; i++)
			printf("%c",datasent[i]);
		printf("\n");
	}

}
