#include "mail_user_agent.h"

/*
    counting the number of sending mails
*/
int mail_counting = 0;
/*
    logged-in email
*/
char logged_in_email[BUFFER_SIZE] = "";
/*
    socket to connect to MSA
*/
int send_mail_socket_FD = -1;
/*
    socket to connect to MDA
*/
int get_mail_socket_FD = -1;
/*
    array to receive email from MDA
*/
Mail* MDA_array = NULL;

/*
    22/3/2025:
    DONE: 
        Create some commands in terminal:
        - Lowercase the tokens
        - list + (send/receive) --> list email
        - make mail --> create draft email
        - draft status/ delete/ print
        - send mail --> send mail
        - Create socket to connect to MSA for sending email
        - Create socket to connect to MDA for receiving email
        - Implementing get email command
        - Renaming and Throwing emails into folder (the commented code)
        - Test the code with creating multiple mails
        - Make the socket connection from MUA to MDA seemless (always open).      
        - When start the program, each part will start up and connect to each other
        through sockets
        - Sent message --> MSA --> MTA --> "sent successfully".
    TODO: 
        - THE COMMAND BETWEEN SERVERS NEED TO BE CAPITALIZED
        - Decide the IP address and port for each server (MSA, MTA, MDA)
    FIX (for send email socket):
        - Login: email and password
        - Correct --> terminal
            - the sender of the mail will be fixed with the logged in email
        - Wrong --> exit
        - Right now, every time the client connect to a server, it will 
        spawn a new connection if it get/send mail --> not good
        Change the code in MSA, MTA, and MDA such that if the connection come from 
        the same IP --> replace it with the old one
        - after printing adios, wait for a while, then print fomdj, and do nothing
        (being blocked?).
        (if MDA match email --> sleep 5 seconds --> send number)
*/

/*
    @brief: 
        create an email based on the content
    @attention: 
        Need to allocate memories for the strings inside each struct
        Since they are stored by reference
        @attention: No need to create pointers for mail_content and header structs 
                    since they are part of the Mail struct and allocated together.
        Update: copy src -> dst, copy len - 1 byte, assign the last byte to be null
        strcpy:
            Copies a string without checking the destination buffer size.
            Stops copying at the null terminator (\0).
            Risk: If the destination buffer is too small, it leads to buffer overflows.
        strncpy: 
            Copies up to n characters, even if the source string is shorter.
            If the source string is smaller than n, it fills the remaining space with garbage 
            (before C99) or zeros (after C99).
            If the source string is longer than n, it does not add a null terminator (\0).
        When copy, we need to ensure that the final byte of the string is null byte.
        When copying a string from src to dest, we need to ensure that.
        the null terminator ('\0') of the source string is properly copied as well.
        strlen(src) does not count the null byte in src.
        strncpy(dest, src, n_bytes);
        if len(src) < n --> copy the entire string including the null byte.
        if len(src) > n --> copy the n characters of src and not adding null byte to dest
        Ex: len(src) < len(dest)
            char src[] = "Hello";
            char dest[10];
            strncpy(dest, src, 10);  // dest will be "Hello\0\0\0\0\0"
        Ex: len(src) > len(dest)
            char src[] = "HelloWorld";
            char dest[5];
            strncpy(dest, src, 5);  // dest will be "Hello", but NOT null-terminated    
*/
Mail* create_email(int id, char* title, char* sender, char* receiver, char* content) {
    Mail* email = (Mail*)malloc(sizeof(Mail));
    if (email == NULL) {
        perror("Error allocating memory for Mail");
        return NULL;
    }

    email->id = id;

    strncpy(email->header.title, title, MAX_TITLE_LEN - 1);
    email->header.title[MAX_TITLE_LEN - 1] = '\0';  // Ensure null termination

    strncpy(email->header.sender, sender, MAX_SENDER_LEN - 1);
    email->header.sender[MAX_SENDER_LEN - 1] = '\0';

    strncpy(email->header.receiver, receiver, MAX_RECEIVER_LEN - 1);
    email->header.receiver[MAX_RECEIVER_LEN - 1] = '\0';

    strncpy(email->mail_content.content, content, MAX_CONTENT_LEN - 1);
    email->mail_content.content[MAX_CONTENT_LEN - 1] = '\0';

    return email;
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
        init cur_send_index and cur_recv_index arrays
*/
void init_arrs(){
    cur_send_index = 0;
    cur_recv_index = 0;
    send_arr = (Mail**)malloc(BUFFER_SIZE * sizeof(Mail*));
    if (send_arr == NULL){
        printf("Value of errno: %d\n", errno);
        perror("Error message from init_arrs() - send_arr:");
        return;
    }
    recv_arr = (Mail**)malloc(BUFFER_SIZE * sizeof(Mail*));
    if (recv_arr == NULL){
        printf("Value of errno: %d\n", errno);
        perror("Error message from init_arrs() - recv_arr:");
        return;
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
        free the content of cur_send and cur_receive
        reset send and receive information to default
*/
void free_arrs() {
    /*
        Free every email in the array and then free the array itself
    */
    for (int i = 0; i < cur_send_index; i++) {
        if (send_arr[i]) {  
            // Free dynamically allocated fields inside Mail (if any)
            free_email(send_arr[i]);  
        }
    }
    for (int i = 0; i < cur_recv_index; i++) {
        if (recv_arr[i]) {  
            free_email(recv_arr[i]);  
        }
    }
    
    // Reset indices
    cur_recv_index = 0;
    cur_send_index = 0;

    // Free arrays only if dynamically allocated
    free(send_arr);
    free(recv_arr);
    
    // Prevent dangling pointer issues
    send_arr = NULL;
    recv_arr = NULL;
}


/*
    @brief: 
        create a file, open it in an editor, and return the filename
*/
char* create_and_edit_file(){
    int file_descriptor;
    char* file_name = (char*)malloc(256);
    if (!file_name){
        perror("Creating a file failed - create_and_edit_file()");
        return NULL;
    }
    /*
        snprintf is a function in C that is used to format a string
        and store it in a character array (buffer)
    */
    snprintf(file_name, 256, "email-XXXXXX");
    file_descriptor = mkstemp(file_name);       // Create a unique temp file
    /*
        Error of mkstemp:
        @brief:
        The mkstemp() function expects the file_name parameter to be 
        a template string that ends with at least 6 X characters, 
        which will be replaced with a unique suffix to generate the actual file name.
        @solution:
        snprintf(file_name, 256, "/tmp/tempfileXXXXXX");
    */
    if (file_descriptor == -1){
        perror("Failed to create a temp file");
        return NULL;
    }
    // Open template file
    FILE* template = fopen(TEMPLATE_FILE, "r");
    if (!template){
        perror("Failed to open template file");
        close(file_descriptor);
        free(file_name);
        return NULL;
    }
    // Open temp file for writing
    /*
        fdopen returns a pointer to the file that 
        associated with the file_descriptor.
        fdopen allows users to use standard IO functions
        like fopen(), fgets(), fputs() 
    */
    FILE* temp_file = fdopen(file_descriptor, "w");
    if (!temp_file) {
        perror("Failed to open temp file");
        fclose(template);
        close(file_descriptor);
        free(file_name);
        return NULL;
    }
    // copy the content from template.txt to temp file
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), template)) {
        if (strncmp(buffer, "FROM:", 5) == 0) {  
            fprintf(temp_file, "FROM: %s\n", logged_in_email);
        } else {
            fputs(buffer, temp_file);
        }
    }
    fclose(template);
    fclose(temp_file);
    char command[512];
    snprintf(command, sizeof(command), "nano %s", file_name);
    int system_return = system(command);
    if (system_return == -1){
        perror("Can not execute the command - create_and_edit_file()");
        free(file_name);
        return NULL;
    }
    return file_name;
}

/*
    @brief: 
        the user input and create an email based on that information
    @attention: 
        strcspn --> calculate the length of the initial segment of the string 
        that does not contain any character from the set 
        strncmp --> compare N characters of S1 to S2
        strncmp will stop comparing when it hits the null byte or source or destination
        strncat --> safe string concatenation
        --> append at most n characters from src to dest
        --> ensure dest remains null-terminated
        --> if the length of the source string exceeds the remaining space --> overflow
*/
Mail* parse_user_input_and_create_mail(char* file_name){
    FILE* file_pointer = fopen(file_name, "r");
    int index = 0;
    int id = mail_counting;
    char title[MAX_TITLE_LEN] = "";
    char sender[MAX_SENDER_LEN] = "";
    char receiver[MAX_RECEIVER_LEN] = "";
    char content[MAX_CONTENT_LEN] = "";
    char* array[] = {title, sender, receiver, content};
    /*
        hold the content of the line
    */
    char line[LINE_LENGTH];
    if (file_pointer == NULL){
        perror("Error opening file");
        return NULL;
    }
    /*
        read a line from a file that is pointed to by file_pointer 
        and store it into line
    */
    int content_started = 0;
    while (fgets(line, LINE_LENGTH, file_pointer) != NULL) { 
        /*
            remove new line character from the line
        */
        line[strcspn(line, "\n")] = '\0';
        if (strncmp(line, "TITLE:", 6) == 0){
            strncpy(title, line + 6, MAX_TITLE_LEN - 1);
        }
        else if (strncmp(line, "FROM:", 5) == 0){
            strncpy(sender, line + 5, MAX_SENDER_LEN - 1);
        }
        else if (strncmp(line, "TO:", 3) == 0){
            strncpy(receiver, line + 3, MAX_RECEIVER_LEN - 1);
        }
        else if (strncmp(line, "CONTENT:", 8) == 0){
            content_started = 1;
            strncat(content, line + 8, MAX_CONTENT_LEN - strlen(content) - 1);
        }
        else if (content_started == 1){
            strncat(content, "\n", MAX_CONTENT_LEN - strlen(content) - 1);
            strncat(content, line, MAX_CONTENT_LEN - strlen(content) - 1);
        }
    }   
    fclose(file_pointer);
    Mail* new_email = create_email(mail_counting, title, sender, receiver, content);
    return new_email;
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

/*
    @brief:
        turn the string in the memory to lowercase
*/
void lower_the_string(char* string){
    char* pointer = string;
    while(*pointer){
        *pointer = tolower((unsigned char)*pointer);
        pointer ++;
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
        listen to message and print message on new thread
    @param: 
        file descriptor of the socket of a client
    @return: 
        None
*/
void start_listening_and_print_messages_on_new_thread(int socketFD){
    pthread_t id;
    pthread_create(&id, NULL, listen_and_print, (void *)(intptr_t)socketFD);
    pthread_detach(id);
}

/*
    @brief: 
        listen and print message
    @param: 
        void pointer to argument 
    @return: 
        void pointer
*/
void* listen_and_print(void *arg){
    int socketFD = (int)(intptr_t)arg;
    char buffer[1024];
    while (true){
        /*
            server listens to multiple clients --> need to specify client_socketFD
        */
        ssize_t amountWasReceived = recv(socketFD, buffer, 1024, 0);
        if (amountWasReceived > 0)
        {
            buffer[amountWasReceived] = '\0';
            printf("Response was: %s\n", buffer);
        }
        else if (amountWasReceived <= 0)
        {
            break;
        }
    }
    close(socketFD);
    return NULL;
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
        spawn the terminal to ask for user inputs and display outputs
*/
void spawn_terminal(){
    /*
        receive the input --> turn that input into array of arguments
        --> if len >= 3 --> continue to the next prompt
        --> if the first argument != allowed ones --> continue to next prompt
        --> if == --> process the next argument (dependently)
        @attention:  
            for send and get, activate the socket when finishing the servers (MDA, MSA, MTA)
            decide address for servers as well
    */
    do{
        char buffer[BUFFER_SIZE];
        memset(buffer, '\0', BUFFER_SIZE);
        printf("Enter an input: ");
        /*
            fgets reads full line of text
            fgets will store the \n when user presses enter --> need to remove it
            scanf reads until whitespace
        */
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL){
            perror("Failed to read the input string!");
            return;
        }
        buffer[strcspn(buffer, "\n")] = '\0';
        /*
            tokens will store the list of addresses of the first character
            of tokens
        */
        char* tokens[MAX_TOKENS];
        int token_count = 0;
        char* pointer = buffer;
        /*
            in the loop, *pointer will return false if *pointer = \0
            (reach to the end of the string)
        */
        while (*pointer && token_count < MAX_TOKENS){
            /*
                skip the whitespaces
                char can contains negative value, but isspace accepts positive 
                --> cast char to unsigned char
            */
            while (isspace((unsigned char)*pointer)){
                pointer ++;
            }
            if (*pointer == '\0'){
                break;
            }
            if (*pointer == '"'){
                pointer ++;     //skip the opening quote
                tokens[token_count++] = pointer;
                while (*pointer && *pointer != '"'){
                    pointer ++;
                }
                if (*pointer == '"'){
                    *pointer++ = '\0';
                }
            }
            else{
                tokens[token_count++] = pointer;
                while (*pointer && !isspace((unsigned char)*pointer)){
                    pointer ++;
                }
                if (*pointer){
                    *pointer++ = '\0';
                }
            }
        }
        if (token_count > 0 && strcmp(tokens[0], "exit") == 0) {
            break;
        }
        /*
            Standard: 2 arguments
        */
        if (token_count >= 3){
            printf("There are too many arguments, the maximum is 2\n");
            continue;
        }
        if (token_count < 2){
            printf("There are too few arguments, the minimum is 2\n");
            continue;
        }

        char* first_token = tokens[0];
        char* second_token = tokens[1];
        /*
            @brief: 
            list: list email from the storage (arrays)
            --> list send --> list sent email from send array
            --> list receive --> list receive email from receive array 
        */
        if (strcmp(first_token, "list") == 0){
            if (strcmp(second_token, "send") == 0){
                if (cur_send_index == 0){
                    printf("The SEND mail box is currently empty, add some!\n");
                    continue;
                }
                else{
                    for (int i = 0; i < cur_send_index; i ++){
                        Mail* cur_mail = send_arr[i];
                        print_email(cur_mail);
                    }
                }
            }
            else if (strcmp(second_token, "receive") == 0){
                if (cur_recv_index == 0){
                    printf("The RECEIVE mail box is currently empty, add some!\n");
                    continue;
                }
                else{
                    for (int i = 0; i < cur_recv_index; i ++){
                        Mail* cur_mail = recv_arr[i];
                        print_email(cur_mail);
                    }
                }
            }
            else{
                printf("The command is not correct, try again!\n");
                continue;
            }
        }
        /*
            @brief: 
            --> make: let the user make an email and create an email
            --> make mail
        */
        else if (strcmp(first_token, "make") == 0){
            if (strcmp(second_token, "mail") == 0){
                char* file_name = create_and_edit_file();
                if (file_name == NULL){
                    perror("Allocate file for email unsuccessfully");
                    break;
                }
                printf("Create file successfully! %s\n", file_name);
                draft_email = parse_user_input_and_create_mail(file_name);
                if (draft_email == NULL){
                    perror("Can not create email");
                    free(file_name);
                    break;
                }
                /*
                    @brief: redirect newly created mails to folder
                */
                char redirect_command[512];
                snprintf(redirect_command, sizeof(redirect_command), "mv %s Send_Emails", file_name);
                int system_return_redirect = system(redirect_command);
                if (system_return_redirect == -1){
                    perror("Can not execute the command - create_and_edit_file()");
                    free(file_name);
                    break;
                }
                /*
                    must free after done using the file
                */
                free(file_name);
            }
            else{
                printf("The command is not correct, try again!\n");
                continue;
            }
        }
        /*
            @brief: 
                Since we create an email --> we will create a draft email
                --> we need to check whether the draft email exist
                --> if no: tell the user to create an email --> continue
                --> if yes: send or delete
                Additional command --> print draft email
        */
        else if (strcmp(first_token, "draft") == 0){
            if (strcmp(second_token, "status") == 0){
                if (draft_email == NULL){
                    printf("There is no draft email. Create one!\n");
                    continue;
                }   
                else{
                    printf("There is a draft email. Send or Delete?\n");
                }
            }
            else if (strcmp(second_token, "print") == 0){
                if (draft_email == NULL){
                    printf("There is no draft email to print. Create one!\n");
                    continue;
                }   
                else{
                    print_email(draft_email);
                }
            }   
            else if (strcmp(second_token, "delete") == 0){
                if (draft_email == NULL){
                    printf("There is no draft email to delete!\n");
                    continue;
                }   
                else{
                    free_email(draft_email);
                    /*
                        uninitialized pointers can hold garbage values
                        --> it is good to assign NULL to freed pointer
                    */
                    draft_email = NULL;
                    printf("Delete the draft email successfully!\n");
                }
            }
            else{
                printf("The command is not correct, try again!\n");
                continue;
            }
        }
        else if (strcmp(first_token, "send") == 0){
            if (strcmp(second_token, "mail") == 0){
                /*
                    @brief: 
                        create a socket to connect to MSA 
                        --> send the draft email through the socket
                    @attention: 
                        check: return or continue
                        test the socket first by setting a simple server socket on MSA side
                        --> we assume that the email has been sent through socket
                        --> add the draft email to send array
                        --> increase the send index
                        --> if the array is out of space --> realloc
                        --> free the draft email
                */

                /*
                    send email through socket
                    be careful with this one!
                */
                /*
                    return the number of bytes sent. 
                    Otherwise, -1 shall be returned and errno set to indicate the error
                */  
                Mail* email = draft_email;
                ssize_t send_status = send(send_mail_socket_FD, email, sizeof(Mail), 0);
                if (send_status < 0) {
                    perror("Message sending failed");
                    // close(send_mail_socket_FD);
                    continue;
                } 
                printf("Email sent successfully\n");
                /*
                    listen message (not mail) here
                */
                char listen_buffer[BUFFER_SIZE];
                ssize_t amountWasReceived = recv(send_mail_socket_FD, listen_buffer, BUFFER_SIZE, 0);
                if (amountWasReceived > 0){
                    listen_buffer[amountWasReceived] = '\0';
                    printf("Response was: %s\n", listen_buffer);
                }
                else if (amountWasReceived <= 0){
                    perror("Message receiving failed");
                    // close(send_mail_socket_FD);
                    continue;
                }
                // close(send_mail_socket_FD);
                /*
                    --> add the draft email to send array
                    --> increase the send index
                    --> if the array is out of space --> realloc
                    --> free the draft email
                */
                mail_counting ++;
                send_arr[cur_send_index] = draft_email;
                cur_send_index ++;
                // If cur_send_index is greater than or equal to BUFFER_SIZE, we need to reallocate
                if (cur_send_index >= BUFFER_SIZE) {
                    Mail** temp_arr = (Mail**)realloc(send_arr, 2 * BUFFER_SIZE * sizeof(Mail*));
                    if (temp_arr == NULL) {
                        perror("Allocate memory for send array unsuccessfully");
                        cur_send_index --;
                        break;
                    }
                    send_arr = temp_arr;
                }
                draft_email = NULL;
                printf("Delete the draft email successfully!\n");
            }
            else{
                printf("The command is not correct, try again!\n");
                continue;
            }
        }
        /*
            @brief:  
                get the email from the MDA server
                --> need to create socket to connect to MDA
                send the message get --> MDA will send back with an array of mail
                After getting the email, put those email into the receive array list
            @attention: 
                get the number of mail from MDA first
                get the mail array later
        */
        else if (strcmp(first_token, "get") == 0){
            if (strcmp(second_token, "mail") == 0){
                /*
                    array of received email
                */
                MDA_array = (Mail*)malloc(BUFFER_SIZE * sizeof(Mail));
                ssize_t send_status = send(get_mail_socket_FD, logged_in_email, strlen(logged_in_email), 0);
                if (send_status < 0){
                    perror("Message sent failed");
                    free(MDA_array);
                    // close(get_mail_socket_FD);
                    continue;
                }
                /*
                    2 scenarios: 
                        success: mail count + mail queue
                        fail:    message
                */
                /*
                    listen message from MDA here
                */
                char listen_buffer[BUFFER_SIZE];
                ssize_t amountWasReceived = recv(get_mail_socket_FD, listen_buffer, BUFFER_SIZE, 0);
                if (amountWasReceived <= 0){
                    perror("Message receiving failed");
                    free(MDA_array);
                    // close(get_mail_socket_FD);
                    continue;
                }
                listen_buffer[amountWasReceived] = '\0';
                /*
                    if there is a message (nothing to receive)
                */
                if (strcmp(listen_buffer, "There is no email for you right now!") == 0) {
                    printf("\033[1;33m%s\033[0m\n", listen_buffer); // Print in yellow
                    free(MDA_array);
                    // close(get_mail_socket_FD);
                    continue;
                }
                /*
                    if there is mail count + queue (something to receive)
                */
                /*
                    set the email count here and send to MDA: receive it!
                */
                int email_count = 0;
                memcpy(&email_count, listen_buffer, sizeof(int)); // Copy int from buffer
                if (email_count >= BUFFER_SIZE){
                    Mail* temp_arr = (Mail*)realloc(MDA_array, 2 * BUFFER_SIZE * sizeof(Mail));
                    if (temp_arr == NULL) {
                        perror("Allocate memory for MDA array unsuccessfully");
                        free(MDA_array);
                        close(get_mail_socket_FD);
                        break;
                    }
                    MDA_array = temp_arr;
                }
                /*
                    send a message to MDA
                */
                char interlude[BUFFER_SIZE] = "";
                ssize_t interlude_status = send(get_mail_socket_FD, interlude, BUFFER_SIZE, 0);
                if (interlude_status < 0) {
                    perror("Sending message to MDA failed");
                    free(MDA_array);
                    continue;
                }
                /*
                    Receive the queue of mails here
                */
                ssize_t mail_queue_recv_status = recv(get_mail_socket_FD, MDA_array, email_count * sizeof(Mail), 0);
                if (mail_queue_recv_status < 0) {
                    perror("Receive failed");
                    free(MDA_array);
                    // close(get_mail_socket_FD);
                    continue;
                }
                for (int i = 0; i < email_count; i ++){
                    //segfault since MDA free --> recv_arr point to null
                    recv_arr[i] = &MDA_array[i];
                    // print_email(&MDA_array[i]);
                    printf("amigos\n");
                    cur_recv_index ++;
                    // If cur_recv_index is greater than or equal to BUFFER_SIZE, we need to reallocate
                    if (cur_recv_index >= BUFFER_SIZE) {
                        Mail** temp_arr = (Mail**)realloc(recv_arr, 2 * BUFFER_SIZE * sizeof(Mail*));
                        if (temp_arr == NULL) {
                            perror("Allocate memory for send array unsuccessfully");
                            cur_recv_index --;
                            free(MDA_array);
                            close(get_mail_socket_FD);
                            break;
                        }
                        recv_arr = temp_arr;
                    }
                }  
            }
            else{
                printf("The command is not correct, try again!\n");
                continue;
            }
        }
    }while(1);
    free_email(draft_email);
}

/*
    @brief: authenticate user
    @param: username, password
    @return: result
*/
int authenticate(const char *username, const char *password) {
    FILE *file = fopen("usrpassw", "r");
    if (file == NULL) {
        perror("Failed to open the file");
        return -1;  // Error
    }
    char line[BUFFER_SIZE];
    char stored_user[BUFFER_SIZE], stored_pass[BUFFER_SIZE];

    while (fgets(line, BUFFER_SIZE, file)) {
        // Remove newline from the read line
        line[strcspn(line, "\n")] = '\0';

        // Split username and password
        char *token = strtok(line, ":");
        if (token != NULL) {
            strcpy(stored_user, token);
            token = strtok(NULL, ":");
            if (token != NULL) {
                strcpy(stored_pass, token);
            } else {
                continue;  // Skip invalid lines
            }
        } else {
            continue;
        }

        // Check if username and password match
        if (strcmp(username, stored_user) == 0 && strcmp(password, stored_pass) == 0) {
            printf("Welcome %s\n", username);
            fclose(file);
            return 1;  // Authentication successful
        }
    }
    printf("Your Username or Password does not exist, create one or try again!\n");
    fclose(file);
    return 0;  // Authentication failed
}

/*
    @brief: signup user
    @param: username, password
    @return: void
*/
void signup_user(const char *username, const char *password) {
    FILE *file = fopen("usrpassw", "a");  // Open file in append mode
    if (file == NULL) {
        perror("Failed to open the file");
        exit(EXIT_FAILURE);
    }

    // Append username and password in "username:password" format
    fprintf(file, "%s:%s\n", username, password);

    fclose(file);
    printf("Signup successful! Account created.\n");
}

/*
    @brief: ask for user's credential
    @param: none
    @return: void
*/
void log_in(){
    int option;
    while(true){
        printf("Sign Up or Log In\n");
        printf("1. Log In\n");
        printf("2. Sign Up\n");
        printf("Your choice: ");
        if (scanf("%d", &option) != 1){
            perror("Can not read the choice");
            exit(EXIT_FAILURE);
        }
        getchar(); 
        if (option == 1){
            char username[BUFFER_SIZE];
            char password[BUFFER_SIZE];
            printf("Enter login username: ");
            if (fgets(username, BUFFER_SIZE, stdin) == NULL){
                perror("Failed to read the username!");
                continue;
            }
            username[strcspn(username, "\n")] = '\0';
            printf("Enter login password: ");
            if (fgets(password, BUFFER_SIZE, stdin) == NULL){
                perror("Failed to read the password!");
                continue;
            }
            password[strcspn(password, "\n")] = '\0';
            int result = authenticate(username, password);
            if (result == 0){
                continue;
            }
            else if (result = 1){
                strncpy(logged_in_email, username, BUFFER_SIZE);
                return;
            }
            else{
                exit(EXIT_FAILURE);
            }
        }
        else if (option == 2){
            char username[BUFFER_SIZE];
            char password[BUFFER_SIZE];
            printf("Enter signup username: ");
            if (fgets(username, BUFFER_SIZE, stdin) == NULL){
                perror("Failed to read the username!");
                continue;
            }
            username[strcspn(username, "\n")] = '\0';
            printf("Enter signup password: ");
            if (fgets(password, BUFFER_SIZE, stdin) == NULL){
                perror("Failed to read the password!");
                continue;
            }
            password[strcspn(password, "\n")] = '\0';
            signup_user(username, password);
            continue;
        }
        else{
            printf("Incorrect option, try again\n");
            continue;
        }
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

/*
    @brief: 
        main function to run main things
*/
int main(int argc, char* argv[]){
    char intro[] = "Welcome to Mogwarts University";
    greeting(intro);
    // sign-up / log-in, then
    // ask for username + password
    log_in();
    init_arrs();
    /*
        connect to MSA
        if socketFD > 0 --> socket created successfully
    */
    send_mail_socket_FD = CreateTCPIPv4Socket();
    if (send_mail_socket_FD <= 0){
        perror("Socket is created unsuccessfully!\n");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in* address_MSA = createIPv4Address("127.0.0.2", 2000);
    /*
        connect to the server
    */
    int result_MSA = connect(send_mail_socket_FD, (struct sockaddr*)address_MSA, sizeof(*address_MSA));
    if (result_MSA < 0){
        perror("Connection is unsuccessful!\n");
        close(send_mail_socket_FD);
        exit(EXIT_FAILURE);
    }
    printf("Connection is successful!\n");
    /*
        socket to connect to MDA server
    */
    get_mail_socket_FD = CreateTCPIPv4Socket();
    if (get_mail_socket_FD <= 0){
        perror("Socket is created unsuccessfully!\n");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in* address_MDA = createIPv4Address("127.0.0.4", 2002);
    /*
        connect to the server
    */
    int result_MDA = connect(get_mail_socket_FD, (struct sockaddr*)address_MDA, sizeof(*address_MDA));
    if (result_MDA < 0){
        perror("Connection is unsuccessful!\n");
        close(get_mail_socket_FD);
        exit(EXIT_FAILURE);
    }
    printf("Connection is successful!\n");
    spawn_terminal();
    /*
        check lại xem các condition đã đủ chưa
        exit có bị leak memory ko
    */
    free_arrs();
    free(MDA_array);
    close(send_mail_socket_FD);
    close(get_mail_socket_FD);
    return 0;
}