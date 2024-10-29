#include <arpa/inet.h>  // Usual header files
#include <ctype.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "account.h"
#include "login.h"

int PORT = 5550;   // Port to be opened
int BACKLOG = 20;  // Number of allowed connections
const int BUFF_SIZE = 1024;

int listenfd, *connfd;
int sin_size;

Account *accounts = NULL;
OnlineUser *onlineUsers = NULL;
Login *logins = NULL;

struct ThreadArgs {
    int *connfd;
    struct sockaddr_in *client;
};

// Receive & Echo message to client
void *echo(void *);

int main(int argc, char *argv[]) {
    if (argc != 1 && argc != 2) {
        fprintf(stderr, "Usage: server\n");
        fprintf(stderr, "Usage: server PORT\n");
        exit(EXIT_FAILURE);
    } else if (argc == 2) {
        PORT = atoi(argv[1]);
    }

    readDb(&accounts);

    struct sockaddr_in server;   // Server's address information
    struct sockaddr_in *client;  // Client's address information
    pthread_t tid;

    // Call socket()
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("\nError: ");
        return 0;
    }
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr =
        htonl(INADDR_ANY);  // INADDR_ANY puts your IP address automatically.

    if (bind(listenfd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        perror("\nError: ");
        return 0;
    }

    if (listen(listenfd, BACKLOG) == -1) {
        perror("\nError: ");
        return 0;
    }

    sin_size = sizeof(struct sockaddr_in);
    client = malloc(sin_size);
    while (1) {
        connfd = malloc(sizeof(int));
        if ((*connfd =
                 accept(listenfd, (struct sockaddr *)client, &sin_size)) == -1)
            perror("\nError: ");

        // Print client's IP
        printf("You got a connection from %s:%d\n", inet_ntoa(client->sin_addr),
               ntohs(client->sin_port));

        // For each client, spawn thread to handle new client
        struct ThreadArgs *threadArgs = malloc(sizeof(struct ThreadArgs));
        threadArgs->connfd = connfd;
        threadArgs->client = client;
        pthread_create(&tid, NULL, &echo, (void *)threadArgs);
    }

    printf("Closing listenfd\n");
    close(listenfd);
    return 0;
}

void *echo(void *arg) {
    int connfd;
    struct sockaddr_in client;
    int bytes_sent, bytes_received;
    char buff[BUFF_SIZE];

    struct ThreadArgs *threadArgs = (struct ThreadArgs *)arg;
    connfd = *(threadArgs->connfd);
    client = *(threadArgs->client);
    free(arg);
    pthread_detach(pthread_self());

    while (1) {
        memset(buff, '\0', (strlen(buff) + 1));

        sin_size = sizeof(struct sockaddr);
        bytes_received = recv(connfd, buff, BUFF_SIZE - 1, 0);
        if (bytes_received < 0) {
            perror("\nError: ");
            continue;
        } else if (bytes_received == 0) {
            printf("Client %s:%d disconnected\n", inet_ntoa(client.sin_addr),
                   ntohs(client.sin_port));
            break;
        }

        buff[bytes_received] = '\0';
        if (buff[bytes_received - 1] == '\n') {
            buff[bytes_received - 1] = '\0';
        }

        if (findLogin(logins, client) == NULL) {
            char *username = strtok(buff, "\n");
            char *password = strtok(NULL, "\n");

            int status = signIn(accounts, &onlineUsers, username, password);
            if (status == SIGN_IN_OK) {
                logins = addLogin(logins, client);
                sprintf(buff, "OK");
            } else if (status == SIGN_IN_FAILED) {
                sprintf(buff, "Not OK");
            } else if (status == SIGN_IN_BLOCKED) {
                sprintf(buff, "Account blocked");
            } else if (status == SIGN_IN_NOT_READY) {
                sprintf(buff, "Account not ready");
            }

            bytes_sent = send(connfd, buff, strlen(buff), 0);
            if (bytes_sent < 0) {
                perror("\nError: ");
                continue;
            }
        } else {
            char msg[BUFF_SIZE];
            memset(msg, '\0', (strlen(msg) + 1));

            char *username = strtok(buff, "\n");
            char *password = strtok(NULL, "\n");

            if (strcmp(password, "bye") == 0) {
                signOut(&onlineUsers, username);
                logins = removeLogin(logins, client);
                sprintf(msg, "Goodbye %s", username);
                bytes_sent = send(connfd, msg, strlen(msg), 0);
                if (bytes_sent < 0) {
                    perror("\nError: ");
                    close(connfd);
                    exit(EXIT_FAILURE);
                }
                continue;
            }

            char digits[BUFF_SIZE];
            memset(digits, '\0', (strlen(digits) + 1));
            char alphas[BUFF_SIZE];
            memset(alphas, '\0', (strlen(alphas) + 1));

            // Separate digits & alphabets
            int isInvalid = 0;
            int digit_len = 0;
            int alpha_len = 0;
            for (int i = 0, n = strlen(password); i < n; i++) {
                if (isdigit(password[i])) {
                    digits[digit_len] = password[i];
                    digit_len++;
                } else if (isalpha(password[i])) {
                    alphas[alpha_len] = password[i];
                    alpha_len++;
                } else {
                    isInvalid = 1;
                    break;
                }
            }

            if (isInvalid) {
                sprintf(buff, "Server: Only digits and alphabets are allowed!");
                bytes_sent = send(connfd, buff, strlen(buff), 0);
                if (bytes_sent < 0) {
                    perror("\nError: ");
                    close(connfd);
                    exit(EXIT_FAILURE);
                }
                continue;
            }

            // Update password
            Account *account = findAccount(accounts, username);
            if (account != NULL) {
                if (account->status == ACC_BLOCKED) {
                    sprintf(msg, "Account blocked");
                    bytes_sent = send(connfd, msg, strlen(msg), 0);
                    if (bytes_sent < 0) {
                        perror("\nError: ");
                        close(connfd);
                        exit(EXIT_FAILURE);
                    }
                    continue;
                }

                strcpy(account->password, password);
                writeDb(accounts);
            }

            // Create message
            digits[digit_len] = '\0';
            alphas[alpha_len] = '\0';
            if (digit_len != 0 && alpha_len != 0) {
                sprintf(msg, "%s\n%s", digits, alphas);
            } else if (digit_len != 0) {
                strcpy(msg, digits);
            } else if (alpha_len != 0) {
                strcpy(msg, alphas);
            } else {
                strcpy(msg, "");
            }

            // Send message to client
            bytes_sent = send(connfd, msg, strlen(msg), 0);
            if (bytes_sent < 0) {
                perror("\nError: ");
                close(connfd);
                exit(EXIT_FAILURE);
            }
        }
    }
}
