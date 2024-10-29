#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

char *SERVER_IP = "127.0.0.1";
int SERVER_PORT = 5550;
const int BUFF_SIZE = 1024;

int connfd; /* File descriptors */
int bytes_sent, bytes_received, sin_size;

int main(int argc, char *argv[]) {
    if (argc != 1 && argc != 3) {
        fprintf(stderr, "Usage: client\n");
        fprintf(stderr, "Usage: client IP PORT\n");
        exit(EXIT_FAILURE);
    } else if (argc == 3) {
        SERVER_IP = argv[1];
        SERVER_PORT = atoi(argv[2]);
    }

    char buff[BUFF_SIZE];
    struct sockaddr_in server; /* Server's address information */
    struct sockaddr_in client; /* Client's address information */

    // Step 1: Construct socket
    connfd = socket(AF_INET, SOCK_STREAM, 0);

    // Step 2: Specify server address
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Step 3: Request to connect server
    if (connect(connfd, (struct sockaddr *)&server, sizeof(struct sockaddr)) <
        0) {
        printf("\nError! Cannot connect to sever! Exit client immediately!\n");
        return 0;
    }

    // Step 4.1: Join server
    char username[BUFF_SIZE];
    char password[BUFF_SIZE];
    while (1) {
        printf("\nEnter without input to exit\n");
        printf("Enter username: ");
        memset(buff, '\0', (strlen(buff) + 1));

        fgets(username, BUFF_SIZE, stdin);
        if (username[strlen(username) - 1] == '\n') {
            username[strlen(username) - 1] = '\0';
        }
        if (strcmp(username, "") == 0) {
            close(connfd);
            exit(EXIT_SUCCESS);
        }

        printf("Enter password: ");
        while (1) {
            fgets(password, BUFF_SIZE, stdin);
            if (password[strlen(password) - 1] == '\n') {
                password[strlen(password) - 1] = '\0';
            }
            if (strcmp(password, "") == 0) {
                close(connfd);
                exit(EXIT_SUCCESS);
            }
            if (strlen(password) > 0) {
                sprintf(buff, "%s\n%s", username, password);
                break;
            }
        }

        sin_size = sizeof(struct sockaddr_in);
        bytes_sent = send(connfd, buff, strlen(buff), 0);
        if (bytes_sent < 0) {
            perror("\nError: ");
            continue;
        }

        bytes_received = recv(connfd, buff, BUFF_SIZE - 1, 0);
        if (bytes_received < 0) {
            perror("\nError: ");
            continue;
        } else if (bytes_received == 0) {
            printf("\nServer disconnected!\n");
            close(connfd);
            exit(EXIT_FAILURE);
        }

        buff[bytes_received] = '\0';
        if (buff[bytes_received - 1] == '\n') {
            buff[bytes_received - 1] = '\0';
        }
        printf("%s\n", buff);
        if (strcmp(buff, "OK") != 0) {
            continue;
        }

        // Step 4.2 : Communicate with server
        while (1) {
            printf("\nEnter bye to logout\n");
            printf("Enter without input to exit\n");
            printf("Change password: ");
            memset(buff, '\0', (strlen(buff) + 1));

            fgets(buff, BUFF_SIZE, stdin);
            if (buff[strlen(buff) - 1] == '\n') {
                buff[strlen(buff) - 1] = '\0';
            }
            if (strcmp(buff, "") == 0) {
                close(connfd);
                exit(EXIT_SUCCESS);
            }

            char msg[BUFF_SIZE];
            memset(msg, '\0', (strlen(msg) + 1));
            sprintf(msg, "%s\n%s", username, buff);

            sin_size = sizeof(struct sockaddr_in);
            bytes_sent = send(connfd, msg, strlen(msg), 0);
            if (bytes_sent < 0) {
                perror("\nError: ");
                close(connfd);
                exit(EXIT_FAILURE);
            }

            memset(buff, '\0', (strlen(buff) + 1));

            sin_size = sizeof(struct sockaddr_in);
            bytes_received = recv(connfd, buff, BUFF_SIZE - 1, 0);
            if (bytes_received < 0) {
                perror("\nError: ");
                close(connfd);
                exit(EXIT_FAILURE);
            } else if (bytes_received == 0) {
                printf("\nServer disconnected!\n");
                close(connfd);
                exit(EXIT_FAILURE);
            }

            buff[bytes_received] = '\0';
            if (buff[bytes_received - 1] == '\n') {
                buff[bytes_received - 1] = '\0';
            }
            printf("%s\n", buff);

            char byeMsg[BUFF_SIZE];
            memset(byeMsg, '\0', (strlen(byeMsg) + 1));
            sprintf(byeMsg, "Goodbye %s", username);
            if (strcmp(buff, byeMsg) == 0) {
                break;
            }
        }
    }

    close(connfd);
    return 0;
}
