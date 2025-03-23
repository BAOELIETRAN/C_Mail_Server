#ifndef MAIL_SUBMISSION_AGENT_H
#define MAIL_SUBMISSION_AGENT_H
#define _XOPEN_SOURCE 500
#define _POSIX_C_SOURCE 200112L

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <time.h>  
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <signal.h>

#define BUFFER_SIZE 4096

/*
    Email content define
*/
#define MAX_TITLE_LEN 100
#define MAX_SENDER_LEN 100
#define MAX_RECEIVER_LEN 100
#define MAX_CONTENT_LEN 1024
/*
    define an email
    Two components of an email:
    1. Header
        a. title of the email
        b. sender of that email
        c. receiver of that email
    2. Mail content
        a. content of that email
    Update: Fix the string inside the mail to using array instead of using pointer
*/
typedef struct{
    int id;
    struct Header{
        char title[MAX_TITLE_LEN];
        char sender[MAX_SENDER_LEN];
        char receiver[MAX_RECEIVER_LEN];
    }; 
    struct Header header;
    struct Mail_Content{
        char content[MAX_CONTENT_LEN];
    };
    struct Mail_Content mail_content;
} Mail;

/*
    MIME header struct
    this is a simplified version of mime header
    there are other fields to implement:
        Content-Transfer-Encoding
        Content-Disposition
        Boundary (for multipart)
    Due to its complexity and the email is expected
    to be plain text, it is not currently used now
*/
typedef struct{
    /*
        whether the mail follows MIME standard or not
    */
    bool mime_stad;
    /*
        define the type of content    
    */
    char* content_type;
} MIME;

/*
    struct that stores information about the socket that
    is accepted by the server socket
*/
typedef struct{
    int accepted_socketFD;
    struct sockaddr_in address;
    int error;
    bool accepted_successfully;
} AcceptedSocket;

struct sockaddr_in* createIPv4Address(const char*, int);
int CreateTCPIPv4Socket();
AcceptedSocket* acceptIncomingConnection(int);
void* receive_and_print_incoming_data(void*);
void start_accepting_incoming_connections(int);
void receive_and_print_incoming_data_on_seperate_thread(AcceptedSocket*);
void send_the_received_message_to_the_MTA(Mail*, int);
void free_email(Mail*);
void free_mail_array(Mail**);
void print_email(Mail);
void sigint_handler(int);
int main();

#endif