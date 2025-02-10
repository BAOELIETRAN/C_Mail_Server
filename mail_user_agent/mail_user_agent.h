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

#define RESET_ALL "\033[0m"
#define BUFFER_SIZE 4096

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
*/
typedef struct{
    struct Header{
        char* title;
        char* sender;
        char* receiver;
    }; 
    struct Header header;
    struct Mail_Content{
        char* content;
    };
    struct Mail_Content mail_content;
} Mail;

Mail* create_email(char*, char*, char*, char*);
void free_email(Mail*);
void greeting(char[]);
void spawn_terminal();
int main(int, char*[]);