#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define CMD_SIZE 1024
#define MSG_SIZE 8192
#define REQ_SIZE CMD_SIZE + MSG_SIZE
#define RES_SIZE CMD_SIZE + MSG_SIZE
#define FILE_NAME_SIZE 256

char *SERVER_IP = "127.0.0.1";
int SERVER_PORT = 5550;
const int BUFF_SIZE = 1024;

int connfd; /* File descriptors */
char req_cmd[CMD_SIZE], req_msg[MSG_SIZE], req[REQ_SIZE], res[REQ_SIZE];
int bytes_sent, bytes_received, sin_size;

void send_text() {
    while (1) {
        // Send message
        printf("\nInsert string to send: ");
        memset(req_msg, '\0', MSG_SIZE);
        fgets(req_msg, MSG_SIZE, stdin);
        if (req_msg[strlen(req_msg) - 1] == '\n') {
            req_msg[strlen(req_msg) - 1] = '\0';
        }
        int msg_len = strlen(req_msg);
        if (msg_len == 0) break;

        memset(req_cmd, '\0', CMD_SIZE);
        sprintf(req_cmd, "TEXT");
        memset(req, '\0', REQ_SIZE);
        sprintf(req, "%s\n%s", req_cmd, req_msg);

        bytes_sent = send(connfd, req, strlen(req), 0);
        if (bytes_sent <= 0) {
            printf("\nConnection closed!\n");
            break;
        }

        // Receive reply
        bytes_received = recv(connfd, res, RES_SIZE, 0);
        if (bytes_received <= 0) {
            printf("\nError! Cannot receive reply from sever!\n");
            break;
        }

        res[bytes_received] = '\0';
        printf("Reply from server:\n%s\n", res);
    }
}

void send_file() {
    printf("\nInsert filename to send: ");
    char file_name[FILE_NAME_SIZE];
    memset(file_name, '\0', FILE_NAME_SIZE);
    fgets(file_name, FILE_NAME_SIZE, stdin);
    if (file_name[strlen(file_name) - 1] == '\n') {
        file_name[strlen(file_name) - 1] = '\0';
    }
    int file_name_len = strlen(file_name);
    if (file_name_len == 0) return;

    // Open file
    FILE *f = fopen(file_name, "rb");
    if (f == NULL) {
        printf("File %s not found!\n", file_name);
        return;
    }

    // Get file size
    fseek(f, 0, SEEK_END);
    long long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    printf("File size: %lld\n", file_size);

    // Initiate request
    memset(req_cmd, '\0', CMD_SIZE);
    sprintf(req_cmd, "FILE");
    memset(req, '\0', REQ_SIZE);
    sprintf(req, "%s\n%s\n%lld", req_cmd, file_name, file_size);
    bytes_sent = send(connfd, req, REQ_SIZE, 0);
    if (bytes_sent <= 0) {
        printf("\nConnection closed!\n");
        return;
    }

    // Send file
    char buff[BUFF_SIZE];
    long long bytes_sent_total = 0;
    while (bytes_sent_total < file_size) {
        memset(buff, '\0', BUFF_SIZE);
        int bytes_read = fread(buff, 1, BUFF_SIZE, f);
        if (bytes_read <= 0) {
            printf("\nError! Cannot read file!\n");
            return;
        }

        bytes_sent = send(connfd, buff, bytes_read, 0);
        if (bytes_sent <= 0) {
            printf("\nConnection closed!\n");
            return;
        }

        bytes_sent_total += bytes_sent;
    }
    fclose(f);

    // Receive status from server
    memset(res, '\0', RES_SIZE);
    bytes_received = recv(connfd, res, RES_SIZE, 0);
    if (bytes_received <= 0) {
        printf("\nError! Cannot receive status from sever!\n");
        return;
    }

    res[bytes_received] = '\0';
    printf("Reply from server:\n%s\n", res);
}

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

        while (1) {
            int choice = 0;
            printf("\nMENU\n");
            printf("----\n");
            printf("1. Send string\n");
            printf("2. Send file\n");
            printf("3. Quit\n");
            printf("Your choice (1-3): ");
            scanf("%d", &choice);
            printf("Choice: %d\n", choice);
            while (getchar() != '\n')
                ;  // Clear stdin
            switch (choice) {
                case 1:
                    send_text();
                    break;
                case 2:
                    send_file();
                    break;
                default:
                    close(connfd);
                    return 0;
            }
        }
    }

    close(connfd);
    return 0;
}
