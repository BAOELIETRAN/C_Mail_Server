#include "mail_user_agent.h"

/*
    counting the order of sending mails
*/
int mail_counting = 0;
/*
    define the send status:
        0 --> the email is not sent
        1 --> the email is sent
*/
int send_status = 0;

/*
    17/2/2025:
    DONE: 
        Create some commands in terminal:
        - Lowercase the tokens
        - list + (send/receive) --> list email
        - make mail --> create draft email
        - draft status/ delete/ print
        - send mail --> send mail
        - Create socket to connect to MSA for sending email
    TODO: 
        - THE COMMAND BETWEEN SERVERS NEED TO BE CAPITALIZED
        - Decide the IP address and port for each server (MSA, MTA, MDA)
        - Create socket to connect to MDA for receiving email
        --> Implementing get email command
        - Implementing search command
*/

/*
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
Mail* create_email(char* title, char* sender, char* receiver, char* content) {
    Mail* email = (Mail*)malloc(sizeof(Mail));
    if (email == NULL) {
        perror("Error allocating memory for Mail");
        return NULL;
    }

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
    free the email after done using
*/
void free_email(Mail* email){
    if (email){
        free(email);
    }
}

/*
    if there is no slot left in the array
    --> increase the size using realloc()
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

void free_mail_array(Mail** MDA_array){
    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (MDA_array[i]) {  
            free(MDA_array[i]);  
        }
    }
    free(MDA_array);  
}

void free_arrs() {
    /*
        Free every email in the array and then free the array itself
    */
    for (int i = 0; i < cur_send_index; i++) {
        if (send_arr[i]) {  
            // Free dynamically allocated fields inside Mail (if any)
            free(send_arr[i]);  
        }
    }
    for (int i = 0; i < cur_recv_index; i++) {
        if (recv_arr[i]) {  
            free(recv_arr[i]);  
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
    Function to create a file, open it in an editor, and return the filename
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
        fputs(buffer, temp_file);
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
    /*
        @attention: changing and redirecting 
        the file right now will make us lost track 
        of the file
        --> Solution: after creating a file, we will 
        change the name and redirect it while running the loop in the terminal
    */
    // /*
    //     command created a file on disk
    //     --> need to rename the file on disk
    //     --> cannot use snprintf since the file is not on memory
    //     --> use rename() to change the name on the disk
    // */
    // char new_file_name[256];
    // snprintf(new_file_name, sizeof(new_file_name), "email-%d", mail_counting);
    
    // /*
    //     @brief: Rename the file on disk to match the new name
    // */
    // if (rename(file_name, new_file_name) != 0) {
    //     perror("Failed to rename the file");
    //     free(file_name);
    //     exit(EXIT_FAILURE);
    // }
    // /*
    //     @brief: redirect email to folder
    //     @attention: need to use the new name of the mail
    // */
    // char redirect_command[512];
    // snprintf(redirect_command, sizeof(redirect_command), "mv %s Send_Emails", new_file_name);
    // int system_return_redirect = system(redirect_command);
    // if (system_return_redirect == -1){
    //     perror("Can not execute the command - create_and_edit_file()");
    //     free(file_name);
    //     exit(EXIT_FAILURE);
    // }
    // /*
    //     must free after done using the file
    // */
    return file_name;
}

/*
    @attention: the user input and create an email based on that information
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
    /*
        typedef struct{
        struct Header{
            char* title;
            char* sender;
            char* receiver;
        }; 
        struct Header header;
        struct Mail_Content{
            char* content;
        };
        struct Mail_Content mail_content;
        } Mail;
    */
    FILE* file_pointer = fopen(file_name, "r");
    int index = 0;
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
    Mail* new_email = create_email(title, sender, receiver, content);
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
    --> turn the string in the memory to lowercase
*/
void lower_the_string(char* string){
    char* pointer = string;
    while(*pointer){
        *pointer = tolower((unsigned char)*pointer);
        pointer ++;
    }
}

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
        char* buffer = (char*)malloc(BUFFER_SIZE);
        if (buffer == NULL){
            perror("Failed to allocate buffer!");
            return;
        }
        printf("Enter an input: ");
        /*
            fgets reads full line of text
            fgets will store the \n when user presses enter --> need to remove it
            scanf reads until whitespace
        */
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL){
            perror("Failed to read the input string!");
            free(buffer);
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
        /*
            Lowering every token and print them out
        */
        for (int i = 0; i < token_count; i ++){
            lower_the_string(tokens[i]);
            printf("Token %d: %s\n", i + 1, tokens[i]);
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
                mail_counting ++;
                if (file_name == NULL){
                    perror("Allocate file for email unsuccessfully");
                    return;
                }
                printf("Create file successfully! %s\n", file_name);
                draft_email = parse_user_input_and_create_mail(file_name);
                if (draft_email == NULL){
                    perror("Can not create email");
                    return;
                }
                // print_email(draft_email);
                free(file_name);
                // free_email(draft_email);
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
                        the socket is not in used right now since 
                        I want to wait for the socket from other server as well
                        --> we assume that the email has been sent through socket
                        --> add the draft email to send array
                        --> increase the send index
                        --> if the array is out of space --> realloc
                        --> free the draft email
                */
                // int sockfd = socket(AF_INET, SOCK_STREAM, 0);
                // if (sockfd < 0){
                //     perror("Socket creation failed");
                //     return;
                // }
                // struct addrinfo server;
                // struct addrinfo* res;
                // const char* hostname = "IP ADDRESS OF MSA";
                // const char* port = "PORT OF MSA";
                // memset(&server, 0, sizeof(server));
                // server.ai_family = AF_INET;
                // server.ai_socktype = SOCK_STREAM;
                // server.ai_protocol = 0;
                // int status = getaddrinfo(hostname, port, &server, &res);
                // if (status != 0){
                //     fprintf(stderr, "get address error: %s\n", gai_strerror(status));
                //     return;
                // }
                // struct addrinfo* temp = res;
                // if (temp != NULL){
                //     /*
                //         convert ip address from binary to human readable
                //     */
                //     char ip_str[INET_ADDRSTRLEN];
                //     struct sockaddr_in* information = (struct sockaddr_in*)temp->ai_addr;
                //     inet_ntop(AF_INET, &information->sin_addr, ip_str, sizeof(ip_str));
                //     /*
                //         convert port number from network byte order to host byte order
                //     */
                //     unsigned short int port_number = ntohs(information->sin_port);
                //     printf("Connecting to: %s:%u\n", ip_str, port_number);
                //     /*
                //         connect to the server
                //     */
                //     int connect_status = connect(sockfd, temp->ai_addr, temp->ai_addrlen);
                //     if (connect_status < 0){
                //         perror("Connect failed");
                        // freeaddrinfo(res);
                        // close(sockfd);
                //         return;
                //     }
                //     /*
                //         return the number of bytes sent. 
                //         Otherwise, -1 shall be returned and errno set to indicate the error
                //     */
                //     Mail* email = draft_email;
                //     ssize_t send_status = send(sockfd, email, sizeof(Mail), 0);
                //     if (send_status < 0) {
                //         perror("Message sending failed");
                        // freeaddrinfo(res);
                        // close(sockfd);
                        // return
                //     } else {
                //         printf("Email sent successfully\n");
                //     } 
                // }
                // freeaddrinfo(res);
                // close(sockfd);
                send_arr[cur_send_index] = draft_email;
                cur_send_index ++;
                // If cur_send_index is greater than or equal to BUFFER_SIZE, we need to reallocate
                if (cur_send_index >= BUFFER_SIZE) {
                    Mail** temp_arr = (Mail**)realloc(send_arr, 2 * BUFFER_SIZE * sizeof(Mail*));
                    if (temp_arr == NULL) {
                        perror("Allocate memory for send array unsuccessfully");
                        cur_send_index --;
                        return;
                    }
                    send_arr = temp_arr;
                }
                free(draft_email);
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
                // /*
                //     array of received email
                // */
                // Mail** MDA_array = (Mail**)malloc(BUFFER_SIZE * sizeof(Mail*));
                // /*
                //     socket to connect to MDA server
                // */
                // int sockfd = socket(AF_INET, SOCK_STREAM, 0);
                // if (sockfd < 0){
                //     perror("Socket creation failed");
                //     free_mail_array(MDA_array);
                //     return;
                // }
                // struct addrinfo server;
                // struct addrinfo* res;
                // const char* hostname = "IP ADDRESS OF MDA";
                // const char* port = "PORT OF MDA";
                // memset(&server, 0, sizeof(server));
                // server.ai_family = AF_INET;
                // server.ai_socktype = SOCK_STREAM;
                // server.ai_protocol = 0;
                // int status = getaddrinfo(hostname, port, &server, &res);
                // if (status != 0){
                //     fprintf(stderr, "get address error: %s\n", gai_strerror(status));
                //     free_mail_array(MDA_array);
                //     close(sockfd);
                //     return;
                // }
                // struct addrinfo* temp = res;
                // if (temp != NULL){
                //     /*
                //         convert ip address from binary to human readable
                //     */
                //     char ip_str[INET_ADDRSTRLEN];
                //     struct sockaddr_in* information = (struct sockaddr_in*)temp->ai_addr;
                //     inet_ntop(AF_INET, &information->sin_addr, ip_str, sizeof(ip_str));
                //     /*
                //         convert port number from network byte order to host byte order
                //     */
                //     unsigned short int port_number = ntohs(information->sin_port);
                //     printf("Connecting to: %s:%u\n", ip_str, port_number);
                //     /*
                //         connect to the server
                //     */
                //     int connect_status = connect(sockfd, temp->ai_addr, temp->ai_addrlen);
                //     if (connect_status < 0){
                //         perror("Connect failed");
                //         freeaddrinfo(res);
                //         free_mail_array(MDA_array);
                //         close(sockfd);
                //         return;
                //     }

                //     const char* message = "GET";
                //     ssize_t send_status = send(sockfd, message, strlen(message), 0);
                //     if (send_status < 0){
                //         perror("Message sent failed");
                //         freeaddrinfo(res);
                //         free_mail_array(MDA_array);
                //         close(sockfd);
                //         return;
                //     }

                //     /* Receive the number of emails first */
                //     int email_count;
                //     ssize_t recv_status = recv(sockfd, &email_count, sizeof(int), 0);
                //     if (recv_status < 0) {
                //         perror("Receive failed");
                //         freeaddrinfo(res);
                //         free_mail_array(MDA_array);
                //         close(sockfd);
                //         return;
                //     }
                //     if (email_count > BUFFER_SIZE){
                //         Mail** temp_arr = (Mail**)realloc(MDA_array, 2 * BUFFER_SIZE * sizeof(Mail*));
                //         if (temp_arr == NULL) {
                //             perror("Allocate memory for MDA array unsuccessfully");
                //             freeaddrinfo(res);
                //             free_mail_array(MDA_array);
                //             close(sockfd);
                //             return;
                //         }
                //         MDA_array = temp_arr;
                //     }
                //     ssize_t email_recv_status = recv(sockfd, MDA_array, email_count * sizeof(Mail*), 0);
                //     if (email_recv_status < 0) {
                //         perror("Receive failed");
                //         freeaddrinfo(res);
                //         free_mail_array(MDA_array);
                //         close(sockfd);
                //         return;
                //     }
                //     for (int i = 0; i < email_count; i ++){
                //         recv_arr[i] = MDA_array[i];
                //         cur_recv_index ++;
                //         // If cur_recv_index is greater than or equal to BUFFER_SIZE, we need to reallocate
                //         if (cur_recv_index >= BUFFER_SIZE) {
                //             Mail** temp_arr = (Mail**)realloc(recv_arr, 2 * BUFFER_SIZE * sizeof(Mail*));
                //             if (temp_arr == NULL) {
                //                 perror("Allocate memory for send array unsuccessfully");
                //                 cur_recv_index --;
                //                 freeaddrinfo(res);
                //                 free_mail_array(MDA_array);
                //                 close(sockfd);
                //                 return;
                //             }
                //             recv_arr = temp_arr;
                //         }
                //     }  
                // }
                // free_mail_array(MDA_array);
                // freeaddrinfo(res);
                // close(sockfd);
                printf("Wait, waiting for finishing servers!\n");
                continue;
            }
            else{
                printf("The command is not correct, try again!\n");
                continue;
            }
        }
        /*
            @brief: 
            search for keyword in the mail array
        */
        else if (strcmp(first_token, "search") == 0){
            printf("Wait, implement later!\n");
            continue;
        }
        free(buffer);
    }while(1);
}

void print_email(Mail* mail){
    printf("---------------------START---------------------------\n");
    printf("Header:\n");
    printf("Title: %s\n", mail->header.title);
    printf("Sender: %s\n", mail->header.sender);
    printf("Receiver: %s\n", mail->header.receiver);
    printf("------------------\n");
    printf("Mail Content:\n");
    printf("Content: %s\n", mail->mail_content.content);
    printf("----------------------END---------------------------\n");
}

int main(int argc, char* argv[]){
    char intro[] = "Welcome to Mogwarts University";
    // greeting(intro);
    /*
        init array before jumping into the terminal
    */
    init_arrs();
    spawn_terminal();
    /*
        Lowercase string testing:
            char* original = "SSSSupaMAANNNN";
            char* string = strdup(original);
            printf("hehe\n");
            lower_the_string(string);
            printf("The string: %s\n", string);
            free(string);
    */
    free_arrs();
    return 0;
}