#include "mail_submission_agent.h"

/*
    Sent message --> MSA --> MTA --> MDA --> "sent successfully".
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
    4/3/2025:
    DONE:
        Create a simple socket to listen and send with MUA
        Receive email from MUA, print it out, and send response back to MUA
    TODO: 
        Implement the server socket to be the broadcast one.
        Implement the queue to store email from MUA
        Set up the condition to send email to MTA
        (if the email can reach to the MDA, then the MUA will receive the message
        --> need a chain of response).

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
    @todo: 
    First, implement a simple socket to hear and response to MUA
    Then, implement the server socket here so that 
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
    /*
        server's socket for listening
    */
    int server_socketFD = CreateTCPIPv4Socket();
    /*
        listen to all incoming traffic on port 2000
    */
    struct sockaddr_in* server_address = createIPv4Address("", 2000);
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
    struct sockaddr_in client_address;
    socklen_t client_addr_size = sizeof(struct sockaddr_in);
    int client_socketFD = accept(server_socketFD, (struct sockaddr*)&client_address, &client_addr_size);
    Mail* received_email = (Mail*)malloc(sizeof(Mail));
    ssize_t amountWasReceived = recv(client_socketFD, received_email, sizeof(Mail), 0);
    if (amountWasReceived <= 0){
        close(client_socketFD);
        close(server_socketFD);
        free(server_address);
        free_email(received_email);
        exit(EXIT_FAILURE);
    }
    print_email(received_email);
    const char* message = "I got the email cuh";
    ssize_t send_status = send(client_socketFD, message, strlen(message), 0);
    if (send_status < 0){
        perror("Sending message failed");
        close(client_socketFD);
        close(server_socketFD);
        free(server_address);
        exit(EXIT_FAILURE);
    }
    printf("Sent message: %s\n", message);
    printf("Closing session ....\n");
    close(client_socketFD);
    close(server_socketFD);
    free_email(received_email);
    free(server_address);
    return 0;
}