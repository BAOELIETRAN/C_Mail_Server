#include "mail_submission_agent.h"

/*
    Sent message --> MSA --> MTA --> "sent successfully".
*/

/*
    expect the socket of MSA opens continuously
    --> seemless connection --> socket always open
    receive the email --> put it in the queue
    if the MTA is working --> send that
    not working --> send to client that break down
    --> dont need the send status in MUA
*/

/*
    23/2/2025:
    DONE:
        Receive email from MUA
    TODO: 
        Fix the socket so that it will open seemlessly.
        Put that in a queue
        Send that to MTA
*/

/*
    @brief: 
        the email after done using
*/
void free_email(Mail* email){
    if (email){
        free(email);
    }
}

/*
    @brief: 
        free the content inside the array and the whole array
*/
void free_mail_array(Mail** Mail_array){
    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (Mail_array[i]) {  
            free_email(Mail_array[i]);  
        }
    }
    free(Mail_array);  
}

/*
    @todo: implement the server socket here so that 
    it will broadcast the email from client to other server
*/
Mail* create_socket_and_receive_email_from_MUA(int socket_fd){
    
}

/*
    @brief: 
        print the content of an email
*/
void print_email(Mail* mail){
    printf("---------------------START---------------------------\n");
    printf("ID: %d\n", mail->id);
    printf("Header:\n");
    printf("Title: %s\n", mail->header.title);
    printf("Sender: %s\n", mail->header.sender);
    printf("Receiver: %s\n", mail->header.receiver);
    printf("------------------\n");
    printf("Mail Content:\n");
    printf("Content:\n");
    printf("%s\n", mail->mail_content.content);
    printf("----------------------END---------------------------\n");
}

int main(){
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0){
        perror("Socket creation failed!");
        exit(EXIT_FAILURE);
    }
    Mail* received_email;
    return 0;
}