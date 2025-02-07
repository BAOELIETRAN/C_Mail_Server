#include "mail_user_agent.h"

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
    spawn_terminal();
    return 0;
}