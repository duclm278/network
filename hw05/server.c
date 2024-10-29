#include <arpa/inet.h> /* These are the usual header files */
#include <ctype.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define CMD_SIZE 1024
#define MSG_SIZE 8192
#define RES_SIZE CMD_SIZE + MSG_SIZE
#define FILE_NAME_SIZE 512

int PORT = 5550; /* Port that will be opened */
int BACKLOG = 2; /* Number of allowed connections */

int listen_sock, conn_sock; /* File descriptors */
char res[RES_SIZE];
struct sockaddr_in server; /* Server's address information */
struct sockaddr_in client; /* Client's address information */
int bytes_sent, bytes_received, sin_size;

int handle_text(char *text) {
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
        bytes_sent = send(conn_sock, res, strlen(res), 0);
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
    bytes_sent = send(conn_sock, res, strlen(res), 0);
    if (bytes_sent < 0) {
        printf("\nConnection closed!\n");
        return 0;
    }

    return 1;
}

int handle_file(char *name, char *data) {
    char out_file[FILE_NAME_SIZE];
    memset(out_file, '\0', FILE_NAME_SIZE);
    sprintf(out_file, "s_%s", name);

    char out_data[MSG_SIZE];
    memset(out_data, '\0', MSG_SIZE);
    strcpy(out_data, data);

    // Create file
    FILE *f = fopen(out_file, "w");
    if (f == NULL) {
        memset(res, '\0', RES_SIZE);
        sprintf(res, "Cannot create file!");
        bytes_sent = send(conn_sock, res, strlen(res), 0);
        if (bytes_sent < 0) {
            printf("\nConnection closed!\n");
            return 0;
        }
        return 1;
    }

    // Write data to file
    fprintf(f, "%s", data);
    fclose(f);

    // Notify client
    // memset(res, '\0', RES_SIZE);
    // sprintf(res, "Saved as %s successfully!", saved);
    // bytes_sent = send(conn_sock, res, strlen(res), 0);
    // if (bytes_sent < 0) {
    //     printf("\nConnection closed!\n");
    //     return 0;
    // }

    // Send back content of file
    bytes_sent = send(conn_sock, out_data, strlen(out_data), 0);
    if (bytes_sent < 0) {
        printf("\nConnection closed!\n");
        return 0;
    }

    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 1 && argc != 2) {
        fprintf(stderr, "Usage: server\n");
        fprintf(stderr, "Usage: server PORT\n");
        exit(EXIT_FAILURE);
    } else if (argc == 2) {
        PORT = atoi(argv[1]);
    }

    // Step 1: Construct a TCP socket to listen connection request
    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) ==
        -1) { /* Call socket() */
        perror("\nError: ");
        exit(EXIT_FAILURE);
    }

    // Step 2: Bind address to socket
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port =
        htons(PORT); /* Remember htons() from "Conversions" section? =) */
    server.sin_addr.s_addr =
        htonl(INADDR_ANY); /* INADDR_ANY puts your IP address automatically */
    if (bind(listen_sock, (struct sockaddr *)&server, sizeof(server)) ==
        -1) { /* Call bind() */
        perror("\nError: ");
        exit(EXIT_FAILURE);
    }

    // Step 3: Listen request from client
    if (listen(listen_sock, BACKLOG) == -1) { /* Call listen() */
        perror("\nError: ");
        exit(EXIT_FAILURE);
    }

    // Step 4: Communicate with client
    while (1) {
        // Accept request
        sin_size = sizeof(struct sockaddr_in);
        if ((conn_sock = accept(listen_sock, (struct sockaddr *)&client,
                                &sin_size)) == -1)
            perror("\nError: ");

        printf("\nYou got a connection from %s:%d\n",
               inet_ntoa(client.sin_addr), ntohs(client.sin_port));

        // Start conversation
        while (1) {
            // Receives message from client
            memset(res, 0, RES_SIZE);
            bytes_received = recv(conn_sock, res, RES_SIZE, 0);  // Blocking
            if (bytes_received <= 0) {
                printf("Connection closed\n");
                break;
            } else {
                res[bytes_received] = '\0';
                printf("Receive: %s\n", res);
            }
            if (strlen(res) == 0) continue;

            char *res_cmd = strtok(res, "\n");
            char *res_msg = strtok(NULL, "");
            if (strcmp(res_cmd, "TEXT") == 0) {
                if (!handle_text(res_msg)) {
                    break;
                }
            } else if (strcmp(res_cmd, "FILE") == 0) {
                char *file_name = strtok(res_msg, "\n");
                char *file_data = strtok(NULL, "");
                if (!handle_file(file_name, file_data)) {
                    break;
                }
            }
        }  // End conversation
        close(conn_sock);
    }

    close(listen_sock);
    return 0;
}
