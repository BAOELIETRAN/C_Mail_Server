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
    create sockets, both for recv and send
    send an email --> record that into array
    receive an email --> record that into array
    FINAL: terminal
*/

/*
    Need to allocate memories for the strings inside each struct
    Since they are stored by reference
*/
Mail* create_email(char* title, char* sender, char* receiver, char* content){
    Mail* email = (Mail*)malloc(sizeof(Mail));
    if (email == NULL){
        printf("Value of errno: %d\n", errno);
        perror("Error message in create_email():");
        exit(EXIT_FAILURE);
    }
    email->header.title = strdup(title);
    email->header.sender = strdup(sender);
    email->header.receiver = strdup(receiver);
    email->mail_content.content = strdup(content);
    if (!email->header.title || !email->header.sender || !email->header.receiver || !email->mail_content.content){
        free(email->header.title);
        free(email->header.sender);
        free(email->header.receiver);
        free(email->mail_content.content);
        free(email);
        printf("Value of errno: %d\n", errno);
        perror("Error message in create_email() - content:");
        exit(EXIT_FAILURE);
    }
    return email;
}

/*
    free the email after done using
*/
void free_email(Mail* email){
    if (email){
        free(email->header.title);
        free(email->header.sender);
        free(email->header.receiver);
        free(email->mail_content.content);
        free(email);
    }
}

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
        command created a file on disk
        --> need to rename the file on disk
        --> cannot use snprintf since the file is not on memory
        --> use rename() to change the name on the disk
    */
    char new_file_name[256];
    snprintf(new_file_name, sizeof(new_file_name), "email-%d", mail_counting);
    
    /*
        @brief: Rename the file on disk to match the new name
    */
    if (rename(file_name, new_file_name) != 0) {
        perror("Failed to rename the file");
        free(file_name);
        exit(EXIT_FAILURE);
    }
    /*
        @brief: redirect email to folder
        @attention: need to use the new name of the mail
    */
    char redirect_command[512];
    snprintf(redirect_command, sizeof(redirect_command), "mv %s Send_Emails", new_file_name);
    int system_return_redirect = system(redirect_command);
    if (system_return_redirect == -1){
        perror("Can not execute the command - create_and_edit_file()");
        free(file_name);
        exit(EXIT_FAILURE);
    }
    /*
        must free after done using the file
    */
    return file_name;
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
    do{
        char* buffer = (char*)malloc(BUFFER_SIZE);
        printf("Enter an input: ");
        if (scanf("%s", buffer) != 1){
            perror("Failed to read the input string!");
            exit(EXIT_FAILURE);
        }
        printf("Input: %s\n", buffer);
        free(buffer);
    }while(1);
}

int main(int argc, char* argv[]){
    char intro[] = "Welcome to Mogwarts University";
    greeting(intro);
    // spawn_terminal();
    char* file_name = create_and_edit_file();
    mail_counting ++;
    if (file_name){
        printf("Create file successfully!\n");
        free(file_name);
    }
    return 0;
}