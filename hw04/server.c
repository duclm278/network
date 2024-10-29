#include <arpa/inet.h> /* These are usual header files. */
#include <ctype.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "account.h"

int PORT = 5550; /* Port to be opened */
int BUFF_SIZE = 1024;

typedef struct Login {
    struct sockaddr_in client;
    struct Login *next;
} Login;

Login *addLogin(Login *logins, struct sockaddr_in client) {
    Login *newLogin = (Login *)malloc(sizeof(Login));
    newLogin->client = client;
    newLogin->next = logins;
    return newLogin;
}

Login *findLogin(Login *logins, struct sockaddr_in client) {
    Login *current = logins;
    while (current != NULL) {
        if (current->client.sin_addr.s_addr == client.sin_addr.s_addr &&
            current->client.sin_port == client.sin_port) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

Login *removeLogin(Login *logins, struct sockaddr_in client) {
    Login *current = logins;
    Login *prev = NULL;
    while (current != NULL) {
        if (current->client.sin_addr.s_addr == client.sin_addr.s_addr &&
            current->client.sin_port == client.sin_port) {
            if (prev == NULL) {
                logins = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            return logins;
        }
        prev = current;
        current = current->next;
    }
    return logins;
}

int main(int argc, char *argv[]) {
    if (argc != 1 && argc != 2) {
        fprintf(stderr, "Usage: server\n");
        fprintf(stderr, "Usage: server PORT\n");
        exit(EXIT_FAILURE);
    } else if (argc == 2) {
        PORT = atoi(argv[1]);
    }

    int server_sock; /* File descriptors */
    char buff[BUFF_SIZE];
    struct sockaddr_in server; /* Server's address information */
    struct sockaddr_in client; /* Client's address information */
    int bytes_sent, bytes_received, sin_size;

    // Step 1: Construct UDP socket
    if ((server_sock = socket(AF_INET, SOCK_DGRAM, 0)) ==
        -1) { /* Call socket() */
        perror("\nError: ");
        exit(EXIT_FAILURE);
    }

    // Step 2: Bind address to socket
    server.sin_family = AF_INET;
    server.sin_port =
        htons(PORT); /* Remember htons() from "Conversions" section? =) */
    server.sin_addr.s_addr =
        INADDR_ANY; /* INADDR_ANY puts your IP address automatically. */
    bzero(&(server.sin_zero), 8); /* Zero rest of structure */

    if (bind(server_sock, (struct sockaddr *)&server,
             sizeof(struct sockaddr)) == -1) { /* Call bind() */
        perror("\nError: ");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Step 3: Communicate with clients
    Account *accounts = NULL;
    OnlineUser *onlineUsers = NULL;
    Login *logins = NULL;
    readDb(&accounts);
    while (1) {
        memset(buff, '\0', (strlen(buff) + 1));

        sin_size = sizeof(struct sockaddr);
        bytes_received = recvfrom(server_sock, buff, BUFF_SIZE - 1, 0,
                                  (struct sockaddr *)&client, &sin_size);
        if (bytes_received < 0) {
            perror("\nError: ");
            continue;
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

            bytes_sent = sendto(server_sock, buff, strlen(buff), 0,
                                (struct sockaddr *)&client, sin_size);
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
                bytes_sent = sendto(server_sock, msg, strlen(msg), 0,
                                    (struct sockaddr *)&client, sin_size);
                if (bytes_sent < 0) {
                    perror("\nError: ");
                    close(server_sock);
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
                bytes_sent = sendto(server_sock, buff, strlen(buff), 0,
                                    (struct sockaddr *)&client, sin_size);
                if (bytes_sent < 0) {
                    perror("\nError: ");
                    close(server_sock);
                    exit(EXIT_FAILURE);
                }
                continue;
            }

            // Update password
            Account *account = findAccount(accounts, username);
            if (account != NULL) {
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
            bytes_sent = sendto(server_sock, msg, strlen(msg), 0,
                                (struct sockaddr *)&client, sin_size);
            if (bytes_sent < 0) {
                perror("\nError: ");
                close(server_sock);
                exit(EXIT_FAILURE);
            }
        }
    }

    close(server_sock);
    return 0;
}
