#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP Address> <Port>\n", argv[0]);
        return 1;
    }

    char* ip = argv[1];
    int port = atoi(argv[2]);

    if (port <= 0) {
        fprintf(stderr, "Please provide a valid port number\n");
        return 1;
    }

    int socketFD = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (strlen(ip) == 0)
        address.sin_addr.s_addr = INADDR_ANY;
    else if (inet_pton(AF_INET, ip, &address.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return 1;
    }

    if (connect(socketFD, (struct sockaddr *)&address, sizeof(address)) == 0)
        printf("Connection was successful\n");
    else {
        perror("Connection failed");
        return 1;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        return 1;
    } else if (pid == 0) {
        // Child process: read data from server
        char buffer[1024];
        while (true) {
            ssize_t amountReceived = recv(socketFD, buffer, sizeof(buffer), 0);
            if (amountReceived > 0) {
                buffer[amountReceived] = '\0';
                printf("%s\n", buffer);
            } else if (amountReceived == 0) {
                printf("Server disconnected\n");
                break;
            } else {
                perror("Recv failed");
                break;
            }
        }
        exit(0);
    } else {
        // Parent process: write data to server
        char* line = NULL;
        size_t lineSize = 0;
        printf("To Exit type 'exit'\nPlease enter your message:\n");

        while (true) {
            ssize_t charCount = getline(&line, &lineSize, stdin);
            if (charCount > 0) {
                if (strcmp(line, "exit\n") == 0) {
                    kill(pid, SIGKILL);  // Kill the child process
                    break;
                }
                send(socketFD, line, charCount, 0);  // Send the message to the server
            }
        }
        free(line);
        close(socketFD);
    }

    return 0;
}
