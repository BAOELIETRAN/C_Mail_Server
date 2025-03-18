#ifndef MAIL_TRANSFER_AGENT_H
#define MAIL_TRANSFER_AGENT_H
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

/* List of colors */

#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGNETA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"
#define DEFAULT_COLOR "\033[39m"

/* List of formats */

#define BOLD "\033[1m"
#define RESET_BOLD "\033[22m"  

#define DIM "\033[2m"
#define RESET_DIM "\033[22m"  

#define ITALIC "\033[3m"
#define RESET_ITALIC "\033[23m"  

#define UNDERLINE "\033[4m"
#define RESET_UNDERLINE "\033[24m"  

#define STRIKETHROUGH "\033[9m"
#define RESET_STRIKETHROUGH "\033[29m" 

/*
    clear everything on the console
*/
#define CLEAR_SCREEN "\033[H\033[J"

#define BUFFER_SIZE 4096
#define QUEUE_SIZE 3
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
void start_receiving_from_MSA(int);
void free_email(Mail*);
void free_mail_array(Mail**);
void print_email(Mail);
void sigint_handler(int);

#endif