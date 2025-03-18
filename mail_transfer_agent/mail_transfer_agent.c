#include "mail_transfer_agent.h"

/*
    18/3/2025: 
    BUILD MTA:
        MTA will have 2 sockets: 
            one for connecting to MSA
            one for connecting to MDA
        (no need to multithread)
        MTA: 127.0.0.2 : 2001
        MTA will receive the queue, each entry will be a mail
        Create a queue to store mails from MSA
        Take emails from this queue and send that to MDA seperately
*/

/*
    SocketFD that is used to connect to MDA
*/
int connect_socketFD = -1;
/*
    SocketFD that is used to listen to MSA
*/
int server_socketFD = -1;
/*
    server_address that contains information about the server
*/
struct sockaddr_in* server_address = NULL;
/*
    Queue to store MSA' incoming mails
*/
Mail mailing_queue[QUEUE_SIZE];

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
    @brief: 
        create a sockaddr_in structure with given ip address and port 
    @param: 
        ip - string ip address
        port - int port number
    @return: 
        the pointer to the sockaddr_in structure with given ip address and port
*/
struct sockaddr_in* createIPv4Address(const char* ip, int port){
    /*
        sockaddr_in --> structure designed for IPv4 address
    */
    struct sockaddr_in* address = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
    if (address == NULL){
        perror("Allocation failed!\n");
        return NULL;
    }
    /*
        port that used to listen to incoming connection
    */
    address->sin_port = htons(port);
    address->sin_family = AF_INET;
    /*
        if the ip is empty --> it is server ip --> set it to INADDR_ANY
        --> we tell the (client) socket that accept connections from any IPAddress
        that the machine can listen on 
        address->sin_addr.s_addr == destination address
    */
    if (strlen(ip) == 0){
        address->sin_addr.s_addr = INADDR_ANY;
    }
    else{
        /*
            convert IP addr from string format into binary representation
        */
        if (inet_pton(AF_INET, ip, &address->sin_addr.s_addr) <= 0){
            perror("Invalid IP Address\n");
            free(address);
            return NULL;
        }
    }
    return address;
}

/*
    @brief: 
        return a file descriptor for a new IPv4 socket 
*/
int CreateTCPIPv4Socket(){
    return socket(AF_INET, SOCK_STREAM, 0);
}

/*
    @brief: 
        print the content of an email
*/
void print_email(Mail mail){
    printf("---------------------START---------------------------\n");
    printf("ID: %d\n", mail.id);
    printf("Header:\n");
    printf("Title: %s\n", mail.header.title);
    printf("Sender: %s\n", mail.header.sender);
    printf("Receiver: %s\n", mail.header.receiver);
    printf("------------------\n");
    printf("Mail Content:\n");
    printf("Content:\n");
    printf("%s\n", mail.mail_content.content);
    printf("----------------------END---------------------------\n");
}

/*
    @brief: 
        handle Ctrl C --> instead of quitting, clean up first
    @param: 
        signum
    @return: 
        void
*/
void sigint_handler(int signum){
    printf("\n");
    printf("Closing session ....\n");
    close(connect_socketFD);
    close(server_socketFD);
    free(server_address);
    exit(EXIT_SUCCESS);
}

int main(){
    // handle Ctrl + C
    signal(SIGINT, sigint_handler);
    // handle kill command
    signal(SIGTERM, sigint_handler);
    return 0;
}