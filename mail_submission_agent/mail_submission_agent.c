#include "mail_submission_agent.h"

/*
    Sent message --> MSA --> MTA --> MDA --> "sent successfully".
    "sent successfully" --> MTA --> MSA --> MUA
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
    16/3/2025:
    DONE:
        Create a simple socket to listen and send with MUA
        Receive email from MUA, print it out, and send response back to MUA
        Implement the server socket to be multithread, and each will send to the MTA.
        127.0.0.1 : 2000 --> MSA
        127.0.0.2 : 2001 --> MTA
        127.0.0.3 : 2002 --> MDA
        Implement the queue to store email from MUA
        --> if we don't do that here, we need to do that in MTA
        --> when should we send mails in the queue?
            If the number of receiving emails hit the max size of the queue
            --> temporarily decline new emails and send old emails to MTA
        ONLY STORE AND SEND COPIES!
        Set up the condition to send email to MTA
        Change the code so that MSA will be a client connect to MTA
        Ensure that each field is non empty
    TODO:     
        (if the email can reach to the MDA, then the MUA will receive the message
        --> need a chain of response).
        Ensure that each field is in correct format
        Send mail by mail to MTA
*/

/*
    SocketFD that is used to connect to MTA
*/
int connect_socketFD = -1;
/*
    SocketFD that is used to listen to clients
*/
int server_socketFD = -1;
/*
    server_address that contains information about the server
*/
struct sockaddr_in* server_address = NULL;
/*
    Array of client accepted sockets
*/
AcceptedSocket accepted_sockets[10];
/*
    Number of accepted socket
*/
int accepted_socket_counter = 0;
/*
    Count number of receving mails
*/
int mail_counting = 0;
/*
    Declare mutex lock
*/
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;

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
        accept connections from clients
    @param: 
        server_socketFD
    @return: 
        pointer to the struct that contains the information about a client
*/
AcceptedSocket* acceptIncomingConnection(int server_socketFD){
    /*
        accept() will return the socketFD of the connecting client
    */
    struct sockaddr_in client_address;
    socklen_t client_addr_size = sizeof(struct sockaddr_in);
    int client_socketFD = accept(server_socketFD, (struct sockaddr*)&client_address, &client_addr_size);
    AcceptedSocket* accepted_socket = (AcceptedSocket*)malloc(sizeof(AcceptedSocket));
    accepted_socket->address = client_address;
    accepted_socket->accepted_socketFD = client_socketFD;
    accepted_socket->accepted_successfully = client_socketFD > 0;
    if (!accepted_socket->accepted_successfully){
        accepted_socket->error = client_socketFD;
    }
    return accepted_socket;
}

/*
    @brief: 
        send the received email to MTA
    @param: 
        received email
        socketFD of the client that we received the message
    @return: 
        void
*/
void send_the_received_message_to_the_MTA(Mail* received_email, int client_socketFD){
    pthread_mutex_lock(&queue_mutex);
    Mail mail_copy = *received_email;
    send(connect_socketFD, &mail_copy, sizeof(mail_copy), 0);
    mail_counting ++;
    printf("Current mail counting: %d\n", mail_counting);
    // free email after sending copy
    free_email(received_email);
    pthread_mutex_unlock(&queue_mutex);
}

/*
    @brief: 
        function that helps server receive message from client
        and send that to the others client
    @param: 
        void* arg
    @return
        void*
*/
void* receive_and_print_incoming_data(void *arg){
    int client_socketFD = (int)(intptr_t)arg;
    while (true)
    {
        /*
            server listens to multiple clients --> need to specify client_socketFD
        */
        const char* message = "";
        Mail* received_email = (Mail*)malloc(sizeof(Mail));
        ssize_t amountWasReceived = recv(client_socketFD, received_email, sizeof(Mail), 0);
        if (amountWasReceived > 0){
            //bonus: if any required field is empty --> reject
            if (strlen(received_email->header.sender) == 0 && strlen(received_email->header.receiver) == 0){
                printf("The SENDER and RECEIVER fields are empty, try again!\n");
                message = "The SENDER and RECEIVER fields are empty, try again!";
            }
            else if (strlen(received_email->header.sender) == 0){
                printf("The SENDER field is empty, try again!\n");
                message = "The SENDER field is empty, try again!";
            }
            else if (strlen(received_email->header.receiver) == 0){
                printf("The RECEIVER field is empty, try again!\n");
                message = "The RECEIVER field is empty, try again!";
            }
            else{
                const char* domain = "@gmail.com";
                size_t sender_mail_len = strlen(received_email->header.sender);
                size_t receiver_mail_len = strlen(received_email->header.receiver);
                size_t domain_len = strlen(domain);
                if ((sender_mail_len <= domain_len || (strcmp(received_email->header.sender + (sender_mail_len - domain_len), domain) != 0))
                    && (receiver_mail_len <= domain_len || (strcmp(received_email->header.receiver + (receiver_mail_len - domain_len), domain) != 0))){
                        printf("The SENDER and RECEIVER fields are in wrong format, try again!\n");
                        message = "The SENDER and RECEIVER fields are in wrong format, try again!";
                }
                else if (sender_mail_len <= domain_len || (strcmp(received_email->header.sender + (sender_mail_len - domain_len), domain) != 0)){
                    printf("The SENDER field is in wrong format, try again!\n");
                    message = "The SENDER field is in wrong format, try again!";
                }
                else if (receiver_mail_len <= domain_len || (strcmp(received_email->header.receiver + (receiver_mail_len - domain_len), domain) != 0)){
                    printf("The RECEIVER field is in wrong format, try again!\n");
                    message = "The RECEIVER field is in wrong format, try again!";
                }
                else{
                    send_the_received_message_to_the_MTA(received_email, client_socketFD);
                    message = "I got the email cuh";
                }
            }
            ssize_t send_status = send(client_socketFD, message, strlen(message), 0);
            if (send_status < 0){
                perror("Sending message failed");
                free_email(received_email);
                break;
            }
        }
        else if (amountWasReceived <= 0){
            free_email(received_email);
            break;
        }
    }
    close(client_socketFD);
    return NULL;
}

/*
    @brief: 
        create a new thread to handle a connection
    @param:
        pointer to the struct that contains information about a client
    @return: 
        void
*/
void receive_and_print_incoming_data_on_seperate_thread(AcceptedSocket* client_socket){
    pthread_t id;
    pthread_create(&id, NULL, receive_and_print_incoming_data, (void *)(intptr_t)client_socket->accepted_socketFD);
    pthread_detach(id);
}

/*
    @brief: 
        accept connection from client and put the connection to the 
        accept_sockets array.
        1 connection = 1 thread
        after accepting 1 connection --> spawn a thread to handle that connection
    @param: 
        server_socketFD
    @return: 
        void
*/
void start_accepting_incoming_connections(int server_socketFD){
    while (true){
        /*
            create seperate thread to send and receive data
        */
        AcceptedSocket* client_socket = acceptIncomingConnection(server_socketFD); 
        accepted_sockets[accepted_socket_counter] = *client_socket;
        accepted_socket_counter ++;
        receive_and_print_incoming_data_on_seperate_thread(client_socket);
    }
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
    /*
        server's socket for listening
    */
    server_socketFD = CreateTCPIPv4Socket();
    /*
        MSA's socket to connect to the MTA
    */
    connect_socketFD = CreateTCPIPv4Socket();
    /*
        listen to all incoming traffic on port 2000
    */
    server_address = createIPv4Address("127.0.0.1", 2000);
    /*
        bind the server to an address
    */
    int bind_result = bind(server_socketFD, (struct sockaddr*)server_address, sizeof(*server_address));
    if (bind_result < 0){
        perror("Server binding is unsuccessful!\n");
        free(server_address);
        close(server_socketFD);
        exit(EXIT_FAILURE);
    }
    printf("Server is bound successfully!\n");
    struct sockaddr_in* address = createIPv4Address("127.0.0.2", 2001);
    /*
        connect to the server
    */
    int result = connect(connect_socketFD, (struct sockaddr*)address, sizeof(*address));
    if (result < 0){
        perror("Connection is unsuccessful!\n");
        free(address);
        close(connect_socketFD);
    }
    printf("Connection is successful!\n");
    /*
        After binding and connecting to MTA, start listening to the incoming sockets
        in this situation, server socket can queue up to 10 connections
    */
    int listen_result = listen(server_socketFD, 10);
    if (listen_result < 0){
        perror("Server listening is unsuccessful!\n");
        free(server_address);
        close(server_socketFD);
        exit(EXIT_FAILURE);
    }
    printf("Server is listening.....\n");
    /*
        receive data from client
    */
    start_accepting_incoming_connections(server_socketFD);
    return 0;
}