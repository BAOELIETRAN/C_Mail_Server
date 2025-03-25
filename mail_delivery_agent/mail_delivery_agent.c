#include "./mail_delivery_agent.h"

/*
    24/3/2025:
    Build MDA:
        - sockets to listen to all clients (get mail from clients)
        --> implement like MSA
        - 1 socket to listen to MTA 
        - 1 struct:
            - mail
            - queue to store receiving mails
            - count the number of emails in each queue
        - 1 array of struct
    TODO:
        redo the print entry fucntion to print table format: email | entries
        let client connect to MDA, MDA will send:
        - number of waiting email for that entry
        - the queue
        clients are expected to send their email as message to MDA
*/

/*
    array that store User_Mail_List
*/
User_Mail_List* waiting_mail_array;
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
    Count number of entries in the queue
*/
int entry_counting = 0;
/*
    Declare mutex lock
*/
pthread_mutex_t waiting_mutex = PTHREAD_MUTEX_INITIALIZER;

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
        function that helps server receive message from clients/MTA
        and handle them
    @param: 
        void* arg
    @return
        void*
*/
void* receive_and_push_incoming_data(void *arg){
    AcceptedSocket* client_socket = (AcceptedSocket*)arg;
    int client_socketFD = (int)client_socket->accepted_socketFD;
    while (true){
        /*
            server listens to multiple clients
            - listen to MTA --> mail + push to queue
            - listen to clients --> reply with entry
        */
        /*
            listen message (not mail) here
            This message is expected to be the email
        */
        char listen_buffer[BUFFER_SIZE];
        bool is_MTA = false;
        Mail* received_email = (Mail*)malloc(sizeof(Mail));
        ssize_t amountWasReceived = 0;
        // check for the IP Address --> MTA or clients
        char client_ip[INET_ADDRSTRLEN]; 
        inet_ntop(AF_INET, &client_socket->address.sin_addr, client_ip, INET_ADDRSTRLEN);
        if (strcmp(client_ip, "127.0.0.2") != 0){
            amountWasReceived = recv(client_socketFD, received_email, sizeof(Mail), 0);
            is_MTA = true;
        }
        else{
            amountWasReceived = recv(client_socketFD, listen_buffer, BUFFER_SIZE, 0);
        }
        if (amountWasReceived > 0){
            /*
                since we will work on the queue at the same time --> need mutex when 
                add entry to the queue or take entry from the queue
            */
            if (is_MTA == true){
                /*
                    create entry and put that into queue
                    care about the size of the mailing queue as well as entry queue
                    --> resize
                */
                bool is_entry_exist = false;
                Mail mail_copy = *received_email;
                pthread_mutex_lock(&waiting_mutex);
                for (int index = 0; index < entry_counting; index ++){
                    User_Mail_List* entry = &waiting_mail_array[index];
                    if (strcmp(entry->receiver, mail_copy.header.receiver) == 0){
                        is_entry_exist = true;
                        entry->mailing_queue[entry->mail_count] = mail_copy;
                        entry->mail_count ++;
                        if (entry->mail_count >= QUEUE_SIZE){
                            Mail* new_queue = (Mail*)realloc(entry->mailing_queue, 2 * BUFFER_SIZE * sizeof(Mail));
                            if (new_queue == NULL) {
                                perror("Allocate memory for send array unsuccessfully");
                                break;
                            }
                            entry->mailing_queue = new_queue;
                        }
                    }
                }
                if (is_entry_exist == false){
                    // create a new entry in the queue
                    // --> malloc mailing queue
                    User_Mail_List new_entry;
                    memset(&new_entry, '\0', sizeof(new_entry));
                    new_entry.mailing_queue = (Mail*)malloc(QUEUE_SIZE * sizeof(Mail));
                    strcpy(new_entry.receiver, mail_copy.header.receiver);
                    new_entry.mail_count = 0;
                    new_entry.mailing_queue[new_entry.mail_count] = mail_copy;
                    new_entry.mail_count ++;
                    waiting_mail_array[entry_counting] = new_entry;
                    entry_counting ++;
                }
                for (int index = 0; index < entry_counting; index ++){
                    print_entry(waiting_mail_array[index]);
                }
                pthread_mutex_unlock(&waiting_mutex);
            }
            else{
                /*
                    take the entry from the queue and send it to the client
                */
                listen_buffer[amountWasReceived] = '\0';
                const char* message = "";
                if (entry_counting == 0){
                    message = "There is no email for you right now!";
                    ssize_t send_status = send(client_socketFD, message, strlen(message), 0);
                    if (send_status < 0){
                        perror("Sending message failed");
                        free_email(received_email);
                        break;
                    }
                }
                else{
                    pthread_mutex_lock(&waiting_mutex);
                    for (int index = 0; index < entry_counting; index ++){
                        User_Mail_List entry = waiting_mail_array[index];
                        if (strcmp(entry.receiver, listen_buffer) == 0){
                            // making the noti green
                            printf("\033[0;32m%s: Match!\n\033[0m", listen_buffer);
                            // Send the number of emails first
                            ssize_t send_status = send(client_socketFD, &entry.mail_count, sizeof(entry.mail_count), 0);
                            if (send_status < 0) {
                                perror("Sending mail count failed");
                                free_email(received_email);
                                break;
                            }
                            // Create a copy of the mailing queue
                            Mail mailing_queue_copy[entry.mail_count];
                            memcpy(mailing_queue_copy, entry.mailing_queue, entry.mail_count * sizeof(Mail));
                            // Send the entire mailing queue
                            send_status = send(client_socketFD, mailing_queue_copy, entry.mail_count * sizeof(Mail), 0);
                            if (send_status < 0) {
                                perror("Sending mailing queue failed");
                                free_email(received_email);
                                break;
                            }

                            printf("\033[0;32m%s: Mailing queue is sent!\n\033[0m", listen_buffer);
                        }
                    }
                    pthread_mutex_unlock(&waiting_mutex);
                }
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
    pthread_create(&id, NULL, receive_and_push_incoming_data, (void *)client_socket);
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
        receive_and_push_incoming_data_to_the_queue_on_seperate_thread(client_socket);
    }
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
    close(server_socketFD);
    free(server_address);
    free(waiting_mail_array);
    exit(EXIT_SUCCESS);
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
    @brief: print each entry in the waiting array
    @param: none
    @return: void
*/
void print_entry(User_Mail_List entry){
    printf("---------------------START_ENTRY---------------------------\n");
    printf("Receiver: %s\n", entry.receiver);
    printf("Mail counting: %d\n", entry.mail_count);
    for (int i = 0; i < entry.mail_count; i ++){
        print_email(entry.mailing_queue[i]);
    }
    printf("----------------------END_ENTRY---------------------------\n");
}

int main(){
    waiting_mail_array = (User_Mail_List*)malloc(WAITING_NUM * sizeof(User_Mail_List));
    if (waiting_mail_array == NULL){
        printf("Memory allocation for waiting_mail_array failed!\n");
        return 1;
    }
    // handle Ctrl + C
    signal(SIGINT, sigint_handler);
    // handle kill command
    signal(SIGTERM, sigint_handler);
    /*
        server's socket for listening
    */
    server_socketFD = CreateTCPIPv4Socket();
    /*
        listen to all incoming traffic on port 2002
    */
    server_address = createIPv4Address("127.0.0.3", 2002);
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