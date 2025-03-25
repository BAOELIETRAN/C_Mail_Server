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
    IDEA:
        Store mails in the queue and pop mail in that queue to send
        to send to MDA at the same time. --> thread safe queue
        BUILD MTA:
            MTA will have 2 sockets: 
                one for connecting to MSA
                one for connecting to MDA
            MTA will receive the mail
            Create a queue to store mails from MSA
            Take emails from this queue and send that to MDA seperately
            Receive mail from MSA and store in a queue (1st thread) --> pop it and 
            --> send to MDA (2nd thread)
            send to MDA every 3 seconds
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
    Array of client accepted sockets
*/
AcceptedSocket accepted_sockets[1];
/*
    Number of accepted socket
*/
int accepted_socket_counter = 0;
/*
    Queue to store MSA' incoming mails
*/
Mailing_Queue queue;

/*
    @brief: init the queue
    @param: Queue* q
    @return: void
*/
void queue_init(Mailing_Queue*){
    memset(queue.mailing_queue, '\0', sizeof(queue.mailing_queue));
    queue.front = queue.rear = queue.count = 0;
    pthread_mutex_init(&queue.lock, NULL);
    pthread_cond_init(&queue.not_empty, NULL);
    pthread_cond_init(&queue.not_full, NULL);
}

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
    // if the connected client is not MSA --> reject
    char client_ip[INET_ADDRSTRLEN]; 
    inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
    if (strcmp(client_ip, "127.0.0.1") != 0){
        printf("You are not allowed to connect to MTA\n");
        return NULL;
    }
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
        function that helps server receive message from client
        and send that to the others client
    @param: 
        void* arg
    @return
        void*
*/
void* receive_and_push_incoming_data(void *arg){
    int client_socketFD = (int)(intptr_t)arg;
    while (true){
        /*
            server listens to multiple clients --> need to specify client_socketFD
        */
        Mail* received_email = (Mail*)malloc(sizeof(Mail));
        ssize_t amountWasReceived = recv(client_socketFD, received_email, sizeof(Mail), 0);
        if (amountWasReceived > 0){
            Mail mail_copy = *received_email;
            // put the email to the queue
            pthread_mutex_lock(&queue.lock);
            while (queue.count == QUEUE_SIZE){
                pthread_cond_wait(&queue.not_full, &queue.lock);
            }
            queue.mailing_queue[queue.rear] = mail_copy;
            queue.rear = (queue.rear + 1)%QUEUE_SIZE;
            queue.count ++;
            // signal that the queue is not empty
            pthread_cond_signal(&queue.not_empty);
            pthread_mutex_unlock(&queue.lock);
            // send to MSA that the email has been received
            const char* message = "Email received";
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
void receive_and_push_incoming_data_to_the_queue_on_seperate_thread(AcceptedSocket* client_socket){
    pthread_t id;
    pthread_create(&id, NULL, receive_and_push_incoming_data, (void *)(intptr_t)client_socket->accepted_socketFD);
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
        if (client_socket == NULL){
            return;
        }
        accepted_sockets[accepted_socket_counter] = *client_socket;
        accepted_socket_counter ++;
        receive_and_push_incoming_data_to_the_queue_on_seperate_thread(client_socket);
    }
}

/*
    @brief: connet to MDA and pop email to send to MDA
    @param: connectFD --> socket to connect to MDA
    @return: void*
*/
void* send_email_to_MDA(void* arg){
    int MTA_socket = *(int*)arg;
    while(true){
        // pop email every 10 seconds
        sleep(10);
        pthread_mutex_lock(&queue.lock);
        while (queue.count == 0){
            pthread_cond_wait(&queue.not_empty, &queue.lock);
        }
        Mail sending_mail = queue.mailing_queue[queue.front];
        queue.front = (queue.front + 1)%QUEUE_SIZE;
        queue.count --;
        pthread_cond_signal(&queue.not_full);
        pthread_mutex_unlock(&queue.lock);
        printf("Mail to send: \n");
        print_email(sending_mail);
        // send the email to MDA 
        // in reality, route email to correct MTA and then send to corresponding MDA
        send(MTA_socket, &sending_mail, sizeof(sending_mail), 0);
    }
    return NULL;
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
    pthread_mutex_destroy(&queue.lock);
    pthread_cond_destroy(&queue.not_empty);
    pthread_cond_destroy(&queue.not_full);
    printf("Closing session ....\n");
    close(connect_socketFD);
    close(server_socketFD);
    free(server_address);
    exit(EXIT_SUCCESS);
}

int main(){
    queue_init(&queue);
    // handle Ctrl + C
    signal(SIGINT, sigint_handler);
    // handle kill command
    signal(SIGTERM, sigint_handler);
    /*
        server's socket for listening
    */
    server_socketFD = CreateTCPIPv4Socket();
    /*
        MTA's socket to connect to the MDA
    */
    connect_socketFD = CreateTCPIPv4Socket();
    /*
        listen to all incoming traffic on port 2001
    */
    server_address = createIPv4Address("127.0.0.2", 2001);
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
    // connect to MDA
    struct sockaddr_in* address = createIPv4Address("127.0.0.3", 2002);
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
        After binding and connecting to MDA, start listening to MSA 
        in this situation, server socket can queue up to 1 connection
        --> only accept MSA
    */
    // a seperate thread to pop email and send that to MDA
    pthread_t connect_to_MDA;
    pthread_create(&connect_to_MDA, NULL, send_email_to_MDA, &connect_socketFD);
    pthread_detach(connect_to_MDA);
    int listen_result = listen(server_socketFD, 1);
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