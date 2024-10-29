#include <arpa/inet.h> /* These are usual header files. */
#include <ctype.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int PORT = 5550; /* Port to be opened */
int BUFF_SIZE = 1024;

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
    int num_clients = 0;
    struct sockaddr_in clients[2]; /* Clients' address information */
    while (num_clients < 2) {
        memset(buff, '\0', (strlen(buff) + 1));

        sin_size = sizeof(struct sockaddr);
        bytes_received = recvfrom(server_sock, buff, BUFF_SIZE - 1, 0,
                                  (struct sockaddr *)&client, &sin_size);
        if (bytes_received < 0) {
            perror("\nError: ");
            continue;
        }

        // Add client to chat
        if (num_clients == 1 &&
            clients[0].sin_addr.s_addr == client.sin_addr.s_addr &&
            clients[0].sin_port == client.sin_port) {
            continue;
        }
        clients[num_clients] = client;
        printf("Client %d joined: [%s:%d]\n", num_clients + 1,
               inet_ntoa(client.sin_addr), ntohs(client.sin_port));
        num_clients++;

        // Acknowledge client
        sprintf(buff, "Status: Waiting for others...");
        bytes_sent = sendto(server_sock, buff, strlen(buff), 0,
                            (struct sockaddr *)&client, sin_size);
        if (bytes_sent < 0) {
            perror("\nError: ");
            close(server_sock);
            exit(EXIT_FAILURE);
        }

        // Enough clients to start chatting
        if (num_clients == 2) {
            sprintf(buff, "Status: Enough clients. Let's chat!");
            for (int i = 0; i < 2; i++) {
                bytes_sent = sendto(server_sock, buff, strlen(buff), 0,
                                    (struct sockaddr *)&clients[i], sin_size);
                if (bytes_sent < 0) {
                    perror("\nError: ");
                    close(server_sock);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    while (1) {
        memset(buff, '\0', (strlen(buff) + 1));

        sin_size = sizeof(struct sockaddr_in);
        bytes_received = recvfrom(server_sock, buff, BUFF_SIZE - 1, 0,
                                  (struct sockaddr *)&client, &sin_size);
        if (bytes_received < 0)
            perror("\nError: ");
        else {
            buff[bytes_received] = '\0';
            printf("[%s:%d]: %s\n", inet_ntoa(client.sin_addr),
                   ntohs(client.sin_port), buff);
        }

        char msg[BUFF_SIZE];
        memset(msg, '\0', (strlen(msg) + 1));
        char digits[BUFF_SIZE];
        memset(digits, '\0', (strlen(digits) + 1));
        char alphas[BUFF_SIZE];
        memset(alphas, '\0', (strlen(alphas) + 1));

        // Separate digits & alphabets
        int isInvalid = 0;
        int digit_len = 0;
        int alpha_len = 0;
        for (int i = 0; i < bytes_received; i++) {
            if (isdigit(buff[i])) {
                digits[digit_len] = buff[i];
                digit_len++;
            } else if (isalpha(buff[i])) {
                alphas[alpha_len] = buff[i];
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
        for (int i = 0; i < 2; i++) {
            if (clients[i].sin_addr.s_addr == client.sin_addr.s_addr &&
                clients[i].sin_port == client.sin_port) {
                continue;
            }
            bytes_sent = sendto(server_sock, msg, strlen(msg), 0,
                                (struct sockaddr *)&clients[i], sin_size);
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
