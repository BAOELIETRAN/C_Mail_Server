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
        127.0.0.1 --> MSA
        127.0.0.2 --> MTA
        Implement the queue to store email from MUA
        --> if we don't do that here, we need to do that in MTA
        --> when should we send mails in the queue?
            If the number of receiving emails hit the max size of the queue
            --> temporarily decline new emails and send old emails to MTA
        ONLY STORE AND SEND COPIES!
        Set up the condition to send email to MTA
    TODO: 
        Change the code so that MSA will be a client connect to MTA
        (if the email can reach to the MDA, then the MUA will receive the message
        --> need a chain of response).
*/

/*
    Array of client accepted sockets
*/
AcceptedSocket accepted_sockets[10];
/*
    Number of accepted socket
*/
int accepted_socket_counter = 0;
/*
    Queue to store clients' incoming mails
*/
Mail mailing_queue[QUEUE_SIZE];
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
        send the received email to other clients
    @param: 
        received email
        socketFD of the client that we received the message
    @return: 
        void
*/
void send_the_received_message_to_the_MTA(Mail* received_email, int client_socketFD){
    pthread_mutex_lock(&queue_mutex);
    if (mail_counting >= QUEUE_SIZE){
        //release after building MTA
        // for (int i = 0; i < accepted_socket_counter; i ++){
        //     /*
        //         we only want to send email to the MTA server
        //     */
        //     struct in_addr target_ip;
        //     inet_pton(AF_INET, "127.0.0.2", &target_ip);
        //     if (accepted_sockets[i].address.sin_addr.s_addr == target_ip.s_addr){
        //         // send the whole mailing queue
        //         send(accepted_sockets[i].accepted_socketFD, mailing_queue, sizeof(mailing_queue), 0);
        //     }
        // }
        //flush the whole queue and send them to MTA
        memset(mailing_queue, '\0', sizeof(mailing_queue));
        mail_counting = 0;
        printf("The queue is flushed successfully!\n");
        printf("Refreshing our mind .....\n");
    }
    Mail mail_copy = *received_email;
    mailing_queue[mail_counting] = mail_copy;
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
        Mail* received_email = (Mail*)malloc(sizeof(Mail));
        ssize_t amountWasReceived = recv(client_socketFD, received_email, sizeof(Mail), 0);
        if (amountWasReceived > 0){
            print_email(received_email);
            send_the_received_message_to_the_MTA(received_email, client_socketFD);
            const char* message = "I got the email cuh";
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
    /*
        server's socket for listening
    */
    int server_socketFD = CreateTCPIPv4Socket();
    /*
        listen to all incoming traffic on port 2000
    */
    struct sockaddr_in* server_address = createIPv4Address("127.0.0.1", 2000);
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
    /*
        After binding, start listening to the incoming sockets
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
    printf("Closing session ....\n");
    close(server_socketFD);
    free(server_address);
    return 0;
}