#include "./mail_delivery_agent.h"

/*
    22/3/2025:
    Build MDA:
        - 1 socket to listen to all clients (get mail from clients)
        --> implement like MSA
        - 1 socket to listen to MTA 
        - 3 arrays: receiver, mail queue, and count --> seperate by index
            - receiver: store receiver of email
            - mail queue: store a queue containing receiving mails of that receiver
            - count: count the number of each queue of each receiver
*/

int main(){
    return 0;
}