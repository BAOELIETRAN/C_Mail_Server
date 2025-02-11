#include "mail_user_agent.h"

/*
    @todo:
    function for casting input from the user 
    --> create an email with that information
    create sockets, both for recv and send
    send an email --> record that into array
    receive an email --> record that into array
    FINAL: terminal
*/

/*
    Need to allocate memories for the strings inside each struct
    Since they are stored by reference
*/
Mail* create_email(char* title, char* sender, char* receiver, char* content){
    Mail* email = (Mail*)malloc(sizeof(Mail));
    if (email == NULL){
        printf("Value of errno: %d\n", errno);
        perror("Error message in create_email():");
        exit(EXIT_FAILURE);
    }
    email->header.title = strdup(title);
    email->header.sender = strdup(sender);
    email->header.receiver = strdup(receiver);
    email->mail_content.content = strdup(content);
    if (!email->header.title || !email->header.sender || !email->header.receiver || !email->mail_content.content){
        free(email->header.title);
        free(email->header.sender);
        free(email->header.receiver);
        free(email->mail_content.content);
        free(email);
        printf("Value of errno: %d\n", errno);
        perror("Error message in create_email() - content:");
        exit(EXIT_FAILURE);
    }
    return email;
}

/*
    free the email after done using
*/
void free_email(Mail* email){
    if (email){
        free(email->header.title);
        free(email->header.sender);
        free(email->header.receiver);
        free(email->mail_content.content);
        free(email);
    }
}

void init_arrs(){
    cur_send_index = 0;
    cur_recv_index = 0;
    send_arr = (Mail**)malloc(BUFFER_SIZE * sizeof(Mail*));
    if (send_arr == NULL){
        printf("Value of errno: %d\n", errno);
        perror("Error message from init_arrs() - send_arr:");
        exit(EXIT_FAILURE);
    }
    recv_arr = (Mail**)malloc(BUFFER_SIZE * sizeof(Mail*));
    if (recv_arr == NULL){
        printf("Value of errno: %d\n", errno);
        perror("Error message from init_arrs() - recv_arr:");
        exit(EXIT_FAILURE);
    }
}

void free_arrs(){
    cur_recv_index = 0;
    cur_send_index = 0;
    free(send_arr);
    free(recv_arr);
}

void greeting(char intro[]) {
    int len = strlen(intro);
    int index = 0;

    while (index < len) {
        printf(CLEAR_SCREEN);  // Clear the console
        printf(GREEN BOLD);    // Set text color and style

        for (int i = 0; i < len; i++) {
            if (i == index) {
                printf("%c", toupper(intro[i]));  // Uppercase the current character
            } else {
                printf("%c", intro[i]);
            }
        }

        printf(RESET_ALL);  // Reset text attributes
        fflush(stdout);     // Flush the output buffer
        // Create a timespec structure for nanosleep
        struct timespec req;
        req.tv_sec = 0;
        req.tv_nsec = 100000000L;  // 100 milliseconds
        nanosleep(&req, NULL);     // Sleep for the specified time
        index++;            // Move to the next character
    }
    printf("\n");
}

void spawn_terminal(){
    do{
        char* buffer = (char*)malloc(BUFFER_SIZE);
        printf("Enter an input: ");
        if (scanf("%s", buffer) != 1){
            perror("Failed to read the input string!");
            exit(EXIT_FAILURE);
        }
        printf("Input: %s\n", buffer);
        free(buffer);
    }while(1);
}

int main(int argc, char* argv[]){
    char intro[] = "Welcome to Mogwarts University";
    greeting(intro);
    spawn_terminal();
    return 0;
}