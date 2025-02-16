#include "mail_user_agent.h"

/*
    counting the order of sending mails
*/
int mail_counting = 0;
/*
    @todo:
    function for taking input from the user 
    --> create_and_edit_file() --> return a file with input inside
    @todo
    create a function to cast information inside the newly created file
    --> create an email with that information
    @todo
    done parsing user's input into array of tokens
    --> read the blue print and do the terminal commands
    create sockets, both for recv and send
    send an email --> record that into array
    receive an email --> record that into array
    FINAL: terminal
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
        exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
    }
    recv_arr = (Mail**)malloc(BUFFER_SIZE * sizeof(Mail*));
    if (recv_arr == NULL){
        printf("Value of errno: %d\n", errno);
        perror("Error message from init_arrs() - recv_arr:");
        exit(EXIT_FAILURE);
    }
}

void free_arrs(){
    cur_recv_index = 0;
    cur_send_index = 0;
    free(send_arr);
    free(recv_arr);
}

/*
    Function to create a file, open it in an editor, and return the filename
*/
char* create_and_edit_file(){
    int file_descriptor;
    char* file_name = (char*)malloc(256);
    if (!file_name){
        perror("Creating a file failed - create_and_edit_file()");
        exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
    }
    // Open template file
    FILE* template = fopen(TEMPLATE_FILE, "r");
    if (!template){
        perror("Failed to open template file");
        close(file_descriptor);
        free(file_name);
        exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
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

void spawn_terminal(){
    /*
        receive the input --> turn that input into array of arguments
        --> if len >= 3 --> continue to the next prompt
        --> if the first argument != allowed ones --> continue to next prompt
        --> if == --> process the next argument (dependently)
    */
    do{
        char* buffer = (char*)malloc(BUFFER_SIZE);
        if (buffer == NULL){
            perror("Failed to allocate buffer!");
            exit(EXIT_FAILURE);
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
            exit(EXIT_FAILURE);
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
        for (int i = 0; i < token_count; i ++){
            printf("Token %d: %s\n", i + 1, tokens[i]);
        }
        if (token_count >= 3){
            printf("There are too many arguments, the maximum is 2\n");
            continue;
        }
        free(buffer);
    }while(1);
}

void print_email(Mail* mail){
    printf("Header:\n");
    printf("Title: %s\n", mail->header.title);
    printf("Sender: %s\n", mail->header.sender);
    printf("Receiver: %s\n", mail->header.receiver);
    printf("------------------\n");
    printf("Mail Content:\n");
    printf("Content: %s\n", mail->mail_content.content);
}

int main(int argc, char* argv[]){
    char intro[] = "Welcome to Mogwarts University";
    greeting(intro);
    spawn_terminal();
    // char* file_name = create_and_edit_file();
    // mail_counting ++;
    // if (file_name){
    //     printf("Create file successfully! %s\n", file_name);
    // }
    // Mail* new_mail = parse_user_input_and_create_mail(file_name);
    // print_email(new_mail);
    // free(file_name);
    // free_email(new_mail);
    return 0;
}