#include "mail_submission_agent.h"

/*
    expect the socket of MSA opens continuously
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


Mail* create_socket_and_receive_email_from_MUA(int socket_fd){
    Mail* received_email = (Mail*)malloc(sizeof(Mail));
    /*
        create socket 
        bind the socket to IP and port
        listen for incoming traffic
        accept the client traffic
        send and receive data
        close the connection
    */
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0){
        perror("Socket creation failed!");
        free_email(received_email);
        return NULL;
    }
    struct addrinfo server;
    struct addrinfo* res;
    struct sockaddr_in client_addr;
    const char* port = "PORT OF MSA";
    const char* address = "IP ADDRESS OF MSA";
    memset(&server, 0, sizeof(server));
    server.ai_family = AF_INET;
    server.ai_socktype = SOCK_STREAM;
    server.ai_protocol = 0;
    /*
        listen to all address
    */
    server.ai_flags = AI_PASSIVE;
    int status = getaddrinfo(address, port, &server, &res);
    if (status < 0){
        perror("Server socket is not available");
        free_email(received_email);
        return NULL;
    }
    /*
        print server address
    */
    char ip_str[INET_ADDRSTRLEN];
    struct sockaddr_in* information = (struct sockaddr_in*)res->ai_addr;
    inet_ntop(AF_INET, &information->sin_addr, ip_str, sizeof(ip_str));
    printf("IP Address: %s\n", ip_str);
    int bind_status = bind(socket_fd, res->ai_addr, res->ai_addrlen);
    if (bind_status < 0){
        perror("Binding unsuccessfully");
        free_email(received_email);
        return NULL;
    }
    printf("Listening ...\n");
    int listen_status = listen(socket_fd, 15);
    if (listen_status < 0){
        perror("Listening unsuccessfully");
        free_email(received_email);
        return NULL;
    }
    /*
        resolve one client at a time
        for loop won't work since accept() is blocking call
        it will wait until the client connects before proceeding
    */
    socklen_t addr_size = sizeof(client_addr);
    int new_fd = accept(socket_fd, (struct sockaddr*)&client_addr, &addr_size);
    if (new_fd < 0) {
        perror("Accept failed");
        free_email(received_email); // Free allocated memory
        return NULL;
    }
    // Fix buffer size issue with recv()
    ssize_t recv_status = recv(new_fd, received_email, sizeof(Mail), 0);
    if (recv_status < 0){
        perror("Receiving unsuccessfully");
        free_email(received_email);
        return NULL;
    }
    
    const char* message = "Hello SIGMA";
    ssize_t send_status = send(new_fd, message, strlen(message), 0);
    if (send_status < 0){
        perror("Sending message failed");
        free_email(received_email);
        return NULL;
    }
    printf("Sent message: %s\n", message);
    printf("Closing session ....\n");
    close(new_fd);
    close(socket_fd);
    return received_email;
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