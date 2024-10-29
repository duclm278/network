#include <arpa/inet.h>  // Usual header files
#include <ctype.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "account.h"
#include "login.h"

#define CMD_SIZE 1024
#define MSG_SIZE 8192
#define RES_SIZE CMD_SIZE + MSG_SIZE
#define FILE_NAME_SIZE 512

int PORT = 5550;   // Port to be opened
int BACKLOG = 20;  // Number of allowed connections
const int BUFF_SIZE = 1024;

int maxfd;
int clients[FD_SETSIZE];
int clientThreads[FD_SETSIZE];
int listenfd, *connfd;
fd_set checkfds, readfds;
int sin_size;

Account *accounts = NULL;
OnlineUser *onlineUsers = NULL;
Login *logins = NULL;

struct ThreadArgs {
    int clientIndex;
    struct sockaddr_in *client;
};

void *task(void *);  // Thread function prototype
void closeClient(int index, char *username);
int handleText(int clientIndex, char *text);
int handleFile(int clientIndex, char *name, char *size);

int main(int argc, char *argv[]) {
    if (argc != 1 && argc != 2) {
        fprintf(stderr, "Usage: server\n");
        fprintf(stderr, "Usage: server PORT\n");
        exit(EXIT_FAILURE);
    } else if (argc == 2) {
        PORT = atoi(argv[1]);
    }

    readDb(&accounts);

    int nEvents;
    struct sockaddr_in server;   // Server's address information
    struct sockaddr_in *client;  // Client's address information
    pthread_t tid;

    // Call socket()
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("\nError");
        return 0;
    }
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr =
        htonl(INADDR_ANY);  // INADDR_ANY puts your IP address automatically.

    if (bind(listenfd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        perror("\nError");
        return 0;
    }

    if (listen(listenfd, BACKLOG) == -1) {
        perror("\nError");
        return 0;
    }

    // Initiate data structures
    for (int i = 0; i < FD_SETSIZE; i++) {
        clients[i] = -1;
    }
    FD_ZERO(&checkfds);
    FD_SET(listenfd, &checkfds);
    maxfd = listenfd;

    // Initiate threads
    sin_size = sizeof(struct sockaddr_in);
    client = malloc(sin_size);
    while (1) {
        readfds = checkfds;
        nEvents = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (nEvents < 0) {
            perror("\nError");
            return 0;
        }

        if (FD_ISSET(listenfd, &readfds)) {  // New client
            connfd = malloc(sizeof(int));
            if ((*connfd = accept(listenfd, (struct sockaddr *)client,
                                  &sin_size)) == -1)
                perror("\nError");

            for (int i = 0; i < FD_SETSIZE; i++) {
                if (clients[i] < 0) {
                    clients[i] = *connfd;

                    // Print client's IP
                    printf("Got connection from %s:%d\n",
                           inet_ntoa(client->sin_addr),
                           ntohs(client->sin_port));
                    printf("+ connfd = %d, opened\n", clients[i]);
                    break;
                }
                if (i == FD_SETSIZE - 1) {
                    printf("Too many clients\n");
                }
                if (--nEvents <= 0) continue;  // No more events
            }

            // Check status of connfd(s)
            for (int i = 0; i < FD_SETSIZE; i++) {
                if (clients[i] < 0) continue;
                if (FD_ISSET(listenfd, &readfds) && !clientThreads[i]) {
                    // Spawn thread to handle new client
                    struct ThreadArgs *threadArgs =
                        malloc(sizeof(struct ThreadArgs));
                    threadArgs->client = client;
                    threadArgs->clientIndex = i;
                    pthread_create(&tid, NULL, &task, (void *)threadArgs);
                    clientThreads[i] = 1;       // Mark client as being handled
                    if (--nEvents <= 0) break;  // No more events
                }
            }
        }
    }

    printf("Closing listenfd\n");
    close(listenfd);
    return 0;
}

void closeClient(int clientIndex, char *username) {
    FD_CLR(clients[clientIndex], &readfds);
    close(clients[clientIndex]);
    printf("+ connfd = %d, closed\n", clients[clientIndex]);
    clients[clientIndex] = -1;
    clientThreads[clientIndex] = 0;

    if (username == NULL) return;
    if (strcmp(username, "") == 0) return;
    signOut(&onlineUsers, username);
    printf("User %s signed out\n", username);
}

void *task(void *arg) {
    int connfd;
    struct sockaddr_in client;
    int clientIndex;
    int bytes_sent, bytes_received;
    char res[RES_SIZE];
    char buff[BUFF_SIZE];

    struct ThreadArgs *threadArgs = (struct ThreadArgs *)arg;
    client = *(threadArgs->client);
    clientIndex = threadArgs->clientIndex;
    connfd = clients[clientIndex];
    free(arg);
    pthread_detach(pthread_self());

    char *username;
    char *password;
    char *currentUsername = "";
    while (1) {
        if (findLogin(logins, client) == NULL) {
            memset(buff, '\0', (strlen(buff) + 1));

            sin_size = sizeof(struct sockaddr);
            bytes_received = recv(connfd, buff, BUFF_SIZE - 1, 0);
            if (bytes_received < 0) {
                perror("\nError");
                continue;
            } else if (bytes_received == 0) {
                printf("Client %s:%d disconnected\n",
                       inet_ntoa(client.sin_addr), ntohs(client.sin_port));
                break;
            }

            buff[bytes_received] = '\0';
            if (buff[bytes_received - 1] == '\n') {
                buff[bytes_received - 1] = '\0';
            }

            username = strtok(buff, "\n");
            password = strtok(NULL, "\n");

            // Use normal sign in, same account can be signed in multiple times
            // int status =
            //     signIn(accounts, &onlineUsers, username, password);

            // Use strict sign in, same account can be signed in only once
            int status =
                strictSignIn(accounts, &onlineUsers, username, password);

            // Validate sign in
            if (status == SIGN_IN_OK) {
                logins = addLogin(logins, client);
                currentUsername = malloc(strlen(username) + 1);
                strcpy(currentUsername, username);
                printf("User %s signed in\n", currentUsername);
                sprintf(buff, "OK");
            } else if (status == SIGN_IN_FAILED) {
                sprintf(buff, "Not OK");
            } else if (status == SIGN_IN_BLOCKED) {
                sprintf(buff, "Account blocked");
            } else if (status == SIGN_IN_NOT_READY) {
                sprintf(buff, "Account not ready");
            } else if (status == SIGN_IN_ALREADY) {
                sprintf(buff, "Account already signed in");
            }

            bytes_sent = send(connfd, buff, strlen(buff), 0);
            if (bytes_sent < 0) {
                perror("\nError");
                continue;
            }
        } else {
            while (1) {
                // Receives message from client
                memset(res, 0, RES_SIZE);
                bytes_received = recv(connfd, res, RES_SIZE, 0);  // Blocking
                if (bytes_received <= 0) {
                    printf("Connection closed\n");
                    break;
                }
                res[bytes_received] = '\0';
                if (strlen(res) == 0) continue;
                printf("Received:\n%s\n", res);

                char *res_cmd = strtok(res, "\n");
                char *res_msg = strtok(NULL, "");
                if (strcmp(res_cmd, "TEXT") == 0) {
                    if (!handleText(clientIndex, res_msg)) break;
                } else if (strcmp(res_cmd, "FILE") == 0) {
                    char *file_name = strtok(res_msg, "\n");
                    char *file_size = strtok(NULL, "");
                    if (!handleFile(clientIndex, file_name, file_size)) break;
                } else {
                    printf("Unknown command\n");
                }
            }  // End conversation
            break;
        }
    }

    closeClient(clientIndex, currentUsername);
    pthread_exit(NULL);
}

int handleText(int clientIndex, char *text) {
    int connfd = clients[clientIndex];
    int bytes_sent, bytes_received;
    char res[RES_SIZE];

    char digits[strlen(text) + 1];
    memset(digits, '\0', strlen(text) + 1);
    char alphas[strlen(text) + 1];
    memset(alphas, '\0', strlen(text) + 1);

    // Separate digits & alphabets
    int isInvalid = 0;
    int digit_len = 0;
    int alpha_len = 0;
    long n = strlen(text);
    for (int i = 0; i < n; i++) {
        if (isdigit(text[i])) {
            digits[digit_len] = text[i];
            digit_len++;
        } else if (isalpha(text[i])) {
            alphas[alpha_len] = text[i];
            alpha_len++;
        } else {
            isInvalid = 1;
            break;
        }
    }

    if (isInvalid) {
        memset(res, '\0', RES_SIZE);
        sprintf(res, "Only digits and alphabets are allowed!");
        bytes_sent = send(connfd, res, strlen(res), 0);
        if (bytes_sent < 0) {
            printf("\nConnection closed!\n");
            return 0;
        }
        return 1;
    }

    // Create message
    digits[digit_len] = '\0';
    alphas[alpha_len] = '\0';
    memset(res, '\0', RES_SIZE);
    if (digit_len != 0 && alpha_len != 0) {
        sprintf(res, "%s\n%s", digits, alphas);
    } else if (digit_len != 0) {
        strcpy(res, digits);
    } else if (alpha_len != 0) {
        strcpy(res, alphas);
    } else {
        strcpy(res, "");
    }

    // Send message to client
    bytes_sent = send(connfd, res, strlen(res), 0);
    if (bytes_sent < 0) {
        printf("\nConnection closed!\n");
        return 0;
    }

    return 1;
}

int handleFile(int clientIndex, char *name, char *size) {
    int connfd = clients[clientIndex];
    int bytes_sent, bytes_received;
    char res[RES_SIZE];

    char out_file[FILE_NAME_SIZE];
    memset(out_file, '\0', FILE_NAME_SIZE);
    sprintf(out_file, "s_%s", name);

    long long file_size = strtoll(size, NULL, 10);

    // Create file
    FILE *f = fopen(out_file, "wb");
    if (f == NULL) {
        memset(res, '\0', RES_SIZE);
        sprintf(res, "Cannot create file!");
        bytes_sent = send(connfd, res, strlen(res), 0);
        if (bytes_sent < 0) {
            printf("\nConnection closed!\n");
            return 0;
        }
        return 1;
    }

    // Receive file
    char buff[BUFF_SIZE];
    long long bytes_received_total = 0;
    while (bytes_received_total < file_size) {
        memset(buff, '\0', BUFF_SIZE);
        bytes_received = recv(connfd, buff, BUFF_SIZE - 1, 0);
        if (bytes_received < 0) {
            printf("\nConnection closed!\n");
            return 0;
        }
        bytes_received_total += bytes_received;
        fwrite(buff, 1, bytes_received, f);
    }
    fclose(f);

    // Send status to client
    memset(res, '\0', RES_SIZE);
    sprintf(res, "Saved as %s successfully!", out_file);
    bytes_sent = send(connfd, res, strlen(res), 0);
    if (bytes_sent < 0) {
        printf("\nConnection closed!\n");
        return 0;
    }

    return 1;
}