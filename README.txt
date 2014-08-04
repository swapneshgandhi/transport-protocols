3 reliable transport protocols - Alternating Bits, Go-Back-N, and Selective Repeat

Structures used:
struct pkt_info{
	pkt packet;
	bool acknowleged;
	bool sent;
};
Used to buffer the packets at receiver and sender side in GBN. Sent field is used to check if the packet has been sent at least one.

struct pkt_info{
	pkt packet;
	bool acknowleged;
	float send_time;
};
Used to buffer the packets at receiver and sender side in SR. Sent time saves the time at which the message was sent; used to start various timers in SR.

int next_exp_ack_no;
int recv_exp_seq_no;

These are used at senders and receivers to keep track of next packet to arive on either side.

I have included information about other functions and variables in the respective files.

I have set buffer size same as number of messsages that will be transmitted form layer 5. Although this may seem waste of space I didn't want to complicate things.

Window_size is the variable storing the window_size.

For implementing multiple timers, I included sent time of the message while transmitting for SR, and every time timer goes off or a message is reveived,
I find the earliest message sent and put timer on for it with value timeout-(time_local-sent_time_of_packet).

For finding correct timeouts I used trial and error approach, and used whatever suited the best.
I have included values of timeout in the analysis-A-2 document.
