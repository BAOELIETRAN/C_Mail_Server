/*
Each version of a standard (like POSIX or X/Open) 
defines a set of functions, symbols, and behaviors that 
should be available in your program. The feature test macro 
makes sure that only the functions and features from the 
specified version are available.
*/

#define _XOPEN_SOURCE 500

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <time.h>  
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#pragma once

#define TEMPLATE_FILE "template.txt"
#define RESET_ALL "\033[0m"
#define BUFFER_SIZE 4096
#define MAX_TOKENS 10

#define LINE_LENGTH 1024

/*
    Email content define
*/
#define MAX_TITLE_LEN 100
#define MAX_SENDER_LEN 100
#define MAX_RECEIVER_LEN 100
#define MAX_CONTENT_LEN 1024


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
#define DIM "\033[2m"
#define ITALIC "\033[3m"
#define UNDERLINE "\033[4m"
#define STRIKETHROUGH "\033[9m" 
/*
    clear everything on the console
*/
#define CLEAR_SCREEN "\033[H\033[J"

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
    Data structure:
        Send array - keep sending email
            need an index to keep track the current position
        Recv array - keep receiving email
            need an index to keep track the current position
*/
int cur_send_index;
int cur_recv_index;
Mail** send_arr;
Mail** recv_arr;


Mail* create_email(char*, char*, char*, char*);
void free_email(Mail*);
Mail* parse_user_input_and_create_mail(char*);
void print_email(Mail*);
void greeting(char[]);
void spawn_terminal();
int main(int, char*[]);