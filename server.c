#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <sys/select.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int socketFD;
    char uuid[37];
    int active;
    bool chatbot_active;  // Corrected variable name
    bool chatbot_v2_active;
} client_info;

client_info clients[MAX_CLIENTS];



typedef struct {
    char sender_uuid[37];
    char receiver_uuid[37];
    char message[BUFFER_SIZE];
} chat_message;

chat_message chat_history[2000]; // Assuming a simple fixed-size history for demonstration
int chat_history_count = 0;

void list_active_clients(int socketFD) {
    char message[BUFFER_SIZE] = "Active clients:\n";
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            strcat(message, clients[i].uuid);
            strcat(message, "\n");
        }
    }
    send(socketFD, message, strlen(message), 0);
}

void update_logfile(){
    FILE *file = fopen("conversation_log.txt", "w");
    if(file == NULL){
        return;
    }
    fclose(file);
}

void print_chat_history() {
    FILE *log_file;
    log_file = fopen("conversation_log.txt", "a"); // Open log file in append mode

    printf("Chat History (Total Messages: %d):\n", chat_history_count);
    for (int i = 0; i < chat_history_count; i++) {
        // printf("\nMessage %d:\n", i + 1);
        // printf("Sender UUID: %s\n", chat_history[i].sender_uuid);
        // printf("Receiver UUID: %s\n", chat_history[i].receiver_uuid);
        // printf("Message: %s\n", chat_history[i].message);
        fprintf(log_file, "Message sent by: %s \t received by: %s \t message:%s\n", chat_history[i].sender_uuid, chat_history[i].receiver_uuid, chat_history[i].message);
    }
    fclose(log_file);
}


char *load_and_traverse_gpt2(const char *buffer) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "python3 gpt_2_gen.py \"%s\"", buffer);
    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        perror("Failed to run command");
        exit(1);
    }

    char *response = malloc(BUFFER_SIZE);
    if (response == NULL) {
        perror("Memory allocation failed");
        exit(1);
    }

    char line[BUFFER_SIZE];
    response[0] = '\0';  // Initialize the response string

    while (fgets(line, sizeof(line), fp) != NULL) {
        // Skip the time taken line
        if (strncmp(line, "Time taken", 10) == 0) {
            continue;
        }

        // Concatenate the rest of the output to the response
        strcat(response, line);
    }

    pclose(fp);
    return response;
}




void print_conv_between_two_client(int socketFD, char *dest_uuid, char* client_uuid) {
    FILE *log_file;
    log_file = fopen("conversation_log.txt", "a"); // Open log file in append mode
    
    if (log_file == NULL) {
        perror("Error opening log file");
        return;
    }

    int len = strlen(dest_uuid);
    if (len > 0 && dest_uuid[len - 1] == '\n') {
        dest_uuid[len - 1] = '\0';
    }
    char conv_history[2000]; // Buffer to accumulate conversation history
    int conv_history_length = 0; // Length of the conversation history

    // Initialize the conversation history buffer
    strcpy(conv_history, "Conversation History:\n");
    printf("Entered in print_conv_fun \n");
    printf("dest: %send\n",dest_uuid);
    printf("\nEntered in function with client: %send\n", client_uuid);
    int conv_present_not = 0;

    for (int i = 0; i < chat_history_count; i++) {
        printf("Inside loop with sender: \n%send\n%send\n\n, receiver: \n%send\n%send\n\n",chat_history[i].sender_uuid, client_uuid, chat_history[i].receiver_uuid,dest_uuid);
        // || (strcmp(chat_history[i].sender_uuid, dest_uuid) == 0 && strcmp(chat_history[i].receiver_uuid, client_uuid) == 0)
        if ((strcmp(chat_history[i].sender_uuid, client_uuid) == 0 && strcmp(chat_history[i].receiver_uuid, dest_uuid) == 0)) {
            printf("sent \n");
            strcat(conv_history, "sent to: ");
            strcat(conv_history, chat_history[i].receiver_uuid);
            strcat(conv_history, "\n");
            strcat(conv_history, chat_history[i].message);
            strcat(conv_history, "\n");
            // Update the length of the conversation history
            conv_history_length += strlen(chat_history[i].message);
            // printf("message : %s\n", chat_history[i].message);
            // printf("length: %zu\n", strlen(chat_history[i].message));

            // Write the conversation history to the log file
            fprintf(log_file, "Message sent by: %s \t received from: %s \t message:%s\n", chat_history[i].sender_uuid, chat_history[i].receiver_uuid, chat_history[i].message);

            send(socketFD, conv_history, strlen(conv_history), 0);  
            conv_history[0] = '\0';
            conv_history_length = 0;   
            conv_present_not = 1;

        }
        if ((strcmp(chat_history[i].sender_uuid, dest_uuid) == 0 && strcmp(chat_history[i].receiver_uuid, client_uuid) == 0)) {
            strcat(conv_history, "received from: ");
            strcat(conv_history, chat_history[i].sender_uuid);
            strcat(conv_history, "\n");
            strcat(conv_history, chat_history[i].message);
            strcat(conv_history, "\n");
            // Update the length of the conversation history
            conv_history_length += strlen(chat_history[i].message);
            // printf("message : %s\n", chat_history[i].message);
            // printf("length: %zu\n", strlen(chat_history[i].message));


            // Write the conversation history to the log file
            fprintf(log_file, "Message sent by: %s \t received from: %s \t message:%s\n", chat_history[i].sender_uuid, chat_history[i].receiver_uuid, chat_history[i].message);
            
            send(socketFD, conv_history, strlen(conv_history), 0);  
            conv_history[0] = '\0';
            conv_history_length = 0;    
            conv_present_not = 1;      
        }
    }

    if (conv_present_not == 0) {
        // No conversation found message
        const char *no_conv_msg = "No conversation history found between the specified clients.\n";
        send(socketFD, no_conv_msg, strlen(no_conv_msg), 0);
    }
    fclose(log_file); // Close the log file after writing
}


void delete_chat_message(char *sender_uuid, char *receiver_uuid) {
    printf("sender:%send\nreceiver:%send\n",sender_uuid,receiver_uuid);
    int len = strlen(receiver_uuid);
    if (len > 0 && receiver_uuid[len - 1] == '\n') {
        receiver_uuid[len - 1] = '\0';
    }
    for (int i = 0; i < chat_history_count; i++) {
        if (strcmp(chat_history[i].sender_uuid, sender_uuid) == 0 &&
            strcmp(chat_history[i].receiver_uuid, receiver_uuid) == 0) {
            // Shift subsequent elements to fill the gap
            for (int j = i; j < chat_history_count - 1; j++) {
                strcpy(chat_history[j].sender_uuid, chat_history[j + 1].sender_uuid);
                strcpy(chat_history[j].receiver_uuid, chat_history[j + 1].receiver_uuid);
                strcpy(chat_history[j].message, chat_history[j + 1].message);
            }
            // Decrease the chat_history_count to reflect the deleted message
            chat_history_count--;
            i--;
            // break; // Exit loop after deleting the message
        }
    }
    update_logfile();
    print_chat_history();

}




char* trim(char *str) {
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0)  // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator character
    end[1] = '\0';

    return str;
}



char *load_and_traverse_faqs(const char *client_question) {
    FILE *file = fopen("FAQs.txt", "r");
    if (!file) {
        perror("Failed to open FAQ file");
        return NULL;
    }

    // Convert client_question to lowercase for comparison
    char temp_question[strlen(client_question) + 1];
    strcpy(temp_question, client_question);
    // for (int i = 0; temp_question[i]; i++) {
    //     temp_question[i] = tolower(temp_question[i]);
    // }
    

    char line[10000];
    while (fgets(line, sizeof(line), file)) {
        char temp[strlen(line)+1];
        strcpy(temp,line);

        char *question = strtok(temp, "|||");
        char *answer = strtok(NULL, "");

        question = trim(question);
        printf("question: %s\n",question);
        printf("client_que: %s\n", client_question);
        size_t one = strlen(client_question)-1;
        size_t two = strlen(question);
        size_t len = one < two ? two : one;
        printf("len of client_que: %zu\n", len);

        if (strncmp(client_question, question, len) == 0) {
            // printf("%s\n", answer);
            return strdup(answer+2);
        }

    }

    fclose(file);
    return NULL; // No match found
}


void send_message_to_client(char* dest_uuid, char* message, int sourceFD, char* client_uuid) {
    int found = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && strcmp(clients[i].uuid, dest_uuid) == 0) {
            char full_message[BUFFER_SIZE];
            snprintf(full_message, sizeof(full_message), "Message from %s: %s", client_uuid, message);
            send(clients[i].socketFD, full_message, strlen(full_message), 0);
            
            // Store the message in chat history
            if (chat_history_count < 2000) {
                strcpy(chat_history[chat_history_count].sender_uuid, client_uuid);
                strcpy(chat_history[chat_history_count].receiver_uuid, dest_uuid);
                strcpy(chat_history[chat_history_count].message, message);
                chat_history_count++;
            }
            update_logfile();
            print_chat_history();
            
            found = 1;
            break;
        }
    }

    if (!found) {
        char errorMsg[BUFFER_SIZE];
        snprintf(errorMsg, sizeof(errorMsg), "Client %s is offline.\n", dest_uuid);
        send(sourceFD, errorMsg, strlen(errorMsg), 0);
    }
}


void broadcast_message(char* message, int sourceFD, char* client_uuid) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].socketFD != sourceFD) {
            send(clients[i].socketFD, message, strlen(message), 0);
            // send_message_to_client()
            // Store the message in chat history
            if (chat_history_count < 2000) {
                strcpy(chat_history[chat_history_count].sender_uuid, client_uuid);
                strcpy(chat_history[chat_history_count].receiver_uuid, clients[i].uuid);
                strcpy(chat_history[chat_history_count].message, message);
                chat_history_count++;
            }
            print_chat_history();
        }
    }
    update_logfile();
    print_chat_history();
}


int main(int argc, char *argv[]) {
    int port;

    // Check if the port number is passed as an argument
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    port = atoi(argv[1]); // Convert the argument to integer

    if (port <= 0) {
        fprintf(stderr, "Please provide a valid port number\n");
        exit(1);
    }

    int serverSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    char* ip = "127.0.0.1";
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port); // Use the port number from command line argument
    inet_pton(AF_INET, ip, &serverAddress.sin_addr);

    if (bind(serverSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == 0) {
        printf("Socket bound successfully on port %d\n", port);
    } else {
        perror("Bind failed");
        close(serverSocketFD);
        return 1;
    }

    if (listen(serverSocketFD, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(serverSocketFD);
        return 1;
    }

    printf("Waiting for incoming connections on port %d...\n", port);
    update_logfile();

    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(serverSocketFD, &read_fds);

        int max_socket = serverSocketFD;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active) {
                FD_SET(clients[i].socketFD, &read_fds);
                if (clients[i].socketFD > max_socket) {
                    max_socket = clients[i].socketFD;
                }
            }
        }

        int activity = select(max_socket + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("Error in select");
            continue;
        }

        if (FD_ISSET(serverSocketFD, &read_fds)) {
            struct sockaddr_in clientAddress;
            socklen_t clientAddressSize = sizeof(clientAddress);
            int clientSocketFD = accept(serverSocketFD, (struct sockaddr *)&clientAddress, &clientAddressSize);
            if (clientSocketFD < 0) {
                perror("Accept failed");
                continue;
            }

            int index = -1;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (!clients[i].active) {
                    index = i;
                    break;
                }
            }

            if (index != -1) {
                clients[index].socketFD = clientSocketFD;
                uuid_generate_random((unsigned char *)&clients[index].uuid);
                uuid_unparse_lower((unsigned char *)&clients[index].uuid, clients[index].uuid);
                clients[index].active = 1;
                clients[index].chatbot_active = false;  // Initialize chatbot status

                printf("Connection accepted and UUID assigned: %s\n", clients[index].uuid);
            } else {
                const char* message = "Server is full!\n";
                send(clientSocketFD, message, strlen(message), 0);
                close(clientSocketFD);
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active && FD_ISSET(clients[i].socketFD, &read_fds)) {
                char buffer[BUFFER_SIZE];
                ssize_t amountReceived = recv(clients[i].socketFD, buffer, BUFFER_SIZE, 0);
                if (amountReceived > 0) {
                    buffer[amountReceived] = '\0';

                    // Handle client messages and commands
                    if (strncmp(buffer, "/active", 7) == 0) {
                        list_active_clients(clients[i].socketFD);
                    } else if (strncmp(buffer, "/send ", 6) == 0) {
                        char* dest_uuid = strtok(buffer + 6, " ");
                        char* message = strtok(NULL, "");
                        if (dest_uuid && message) {
                            if (strcmp(dest_uuid, "all") == 0) {
                                broadcast_message(message, clients[i].socketFD, clients[i].uuid);
                            } else {
                                send_message_to_client(dest_uuid, message, clients[i].socketFD, clients[i].uuid);
                            }
                        }
                    } else if ((strncmp(buffer, "/logout", 7) == 0) && (!clients[i].chatbot_active)) {
                        const char* byeMessage = "Bye!! Have a nice day\n";
                        send(clients[i].socketFD, byeMessage, strlen(byeMessage), 0);
                        printf("Client [%s] logged out.\n", clients[i].uuid);
                        clients[i].active = 0;
                        close(clients[i].socketFD);
                    } else if (strncmp(buffer, "/chatbot login", 14) == 0) {
                        clients[i].chatbot_active = true; // Activate chatbot
                        const char* welcomeMsg = "stupidbot> Hi, I am stupidbot. I am able to answer a limited set of your questions.\nuser> ";
                        send(clients[i].socketFD, welcomeMsg, strlen(welcomeMsg), 0);
                    } else if ((strncmp(buffer, "/chatbot logout", 15) == 0) && (clients[i].chatbot_active)) {
                        clients[i].chatbot_active = false; // Deactivate chatbot
                        const char* byeMsg = "stupidbot> Bye! Have a nice day and do not complain about me\n";
                        send(clients[i].socketFD, byeMsg, strlen(byeMsg), 0);
                    } else if (strncmp(buffer, "/history ", 9) == 0) {
                        char* dest_uuid = strtok(buffer + 9, " ");
                        printf("just befor func with %s\n", dest_uuid);
                        print_conv_between_two_client(clients[i].socketFD, dest_uuid, clients[i].uuid);
                        printf("exited from func\n");
                        // Implement retrieval of chat history
                    } else if (strncmp(buffer, "/history_delete ", 16) == 0) {
                        char* dest_uuid = strtok(buffer + 16, " ");
                        delete_chat_message(clients[i].uuid, dest_uuid);
                        // Implement deletion of specific chat history
                    } else if (strncmp(buffer, "/delete_all", 11) == 0) {
                        chat_history_count = 0;
                        update_logfile();
                        print_chat_history();
                    } else if ((strncmp(buffer, "/chatbot_v2 login", 17) == 0) && (!clients[i].chatbot_active)){
                        clients[i].chatbot_v2_active = true; // Activate chatbot
                        const char* welcomeMsg = "gpt2bot> Hi,I am updated bot, I am able to answer any question be it correct or incorrect\nuser> ";
                        send(clients[i].socketFD, welcomeMsg, strlen(welcomeMsg), 0);
                    } else if ((strncmp(buffer, "/chatbot_v2 logout", 18) == 0) && (clients[i].chatbot_v2_active)) {
                        clients[i].chatbot_v2_active = false; // Deactivate chatbot
                        const char* byeMsg = "gpt2bot> Bye! Have a nice day and hope you do not have any complaints about me.\n";
                        send(clients[i].socketFD, byeMsg, strlen(byeMsg), 0);
                    } 
                    else {
                        if (clients[i].chatbot_active) {
                            const char* chatbotMsg = "Chatbot is active. Need to implement response logic.\n";
                            const char *filename = "FAQs.txt";
                            printf("buffer is : %s\n",buffer);
                            char *response = load_and_traverse_faqs(buffer);
                            
                            if (response) {
                                size_t final_response_len = strlen("stupidbot> ") + strlen(response) + 1;  // +1 for null terminator
                                char *final_response = malloc(final_response_len);

                                if (final_response) {
                                    strcpy(final_response, "stupidbot> ");
                                    strcat(final_response, response);

                                    printf("response: %s\n", response);
                                    send(clients[i].socketFD, final_response, strlen(final_response), 0);

                                    free(final_response); // Free the final_response after sending
                                } else {
                                    perror("Memory allocation failed");
                                }

                                free(response); // Free the original response memory
                            } else {
                                char* message = "stupidbot> System Malfunction, I couldn't understand your query.";
                                send(clients[i].socketFD, message, strlen(message), 0);
                            }
                            char* client_arrow = "client> ";
                            send(clients[i].socketFD, client_arrow, strlen(client_arrow), 0);

                        }
                        else if(clients[i].chatbot_v2_active){
                            const char* chatbotMsg = "Chatbot v2 is active. Need to implement response logic.\n";
                            // const char *filename = "FAQs.txt";
                            printf("buffer is : %s\n",buffer);
                            char *response = load_and_traverse_gpt2(buffer);

                            
                            if (response) {
                                size_t final_response_len = strlen("gpt2bot ") + strlen(response) + 1;  // +1 for null terminator
                                char *final_response = malloc(final_response_len);

                                if (final_response) {
                                    strcpy(final_response, "gpt2bot> ");
                                    strcat(final_response, response);

                                    printf("response: %s\n", response);
                                    send(clients[i].socketFD, final_response, strlen(final_response), 0);

                                    free(final_response); // Free the final_response after sending
                                } else {
                                    perror("Memory allocation failed");
                                }

                                free(response); // Free the original response memory
                            } else {
                                char* message = "gpt2bot> Sorry, Please provide more reference for better response.\n";
                                send(clients[i].socketFD, message, strlen(message), 0);
                            }
                            char* client_arrow = "client> ";
                            send(clients[i].socketFD, client_arrow, strlen(client_arrow), 0);
                        } else {
                            printf("Client [%s] says: %s\n", clients[i].uuid, buffer);
                        }
                    }
                } else if (amountReceived == 0) {
                    printf("Client [%s] disconnected\n", clients[i].uuid);
                    clients[i].active = 0;
                    close(clients[i].socketFD);
                } else {
                    perror("Recv failed");
                    clients[i].active = 0;
                    close(clients[i].socketFD);
                }
            }
        }
    }

    shutdown(serverSocketFD, SHUT_RDWR);
    return 0;
}

