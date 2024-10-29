#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int client_sock;
char req_cmd[CMD_SIZE], req_msg[MSG_SIZE], req[REQ_SIZE], res[REQ_SIZE];
struct sockaddr_in server_addr; /* Server's address information */
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

        bytes_sent = send(client_sock, req, REQ_SIZE, 0);
        if (bytes_sent <= 0) {
            printf("\nConnection closed!\n");
            break;
        }

        // Receive reply
        bytes_received = recv(client_sock, res, RES_SIZE, 0);
        if (bytes_received <= 0) {
            printf("\nError! Cannot receive reply from sever!\n");
            break;
        }

        res[bytes_received] = '\0';
        printf("Reply from server:\n%s\n", res);
    }
}

void send_file(int client_sock) {
    printf("\nInsert filename to send: ");
    char file_name[FILE_NAME_SIZE];
    memset(file_name, '\0', FILE_NAME_SIZE);
    fgets(file_name, FILE_NAME_SIZE, stdin);
    if (file_name[strlen(file_name) - 1] == '\n') {
        file_name[strlen(file_name) - 1] = '\0';
    }
    int file_name_len = strlen(file_name);
    if (file_name_len == 0) return;

    FILE *f = fopen(file_name, "r");
    if (f == NULL) {
        printf("File %s not found!\n", file_name);
        return;
    }

    // Minus 1 for later delimiter
    char file_data[MSG_SIZE - FILE_NAME_SIZE - 1];
    memset(file_data, '\0', MSG_SIZE - FILE_NAME_SIZE - 1);
    int file_data_len = fread(file_data, 1, MSG_SIZE - FILE_NAME_SIZE - 1, f);
    fclose(f);

    memset(req_cmd, '\0', CMD_SIZE);
    sprintf(req_cmd, "FILE");
    memset(req, '\0', REQ_SIZE);
    sprintf(req, "%s\n%s\n%s", req_cmd, file_name, file_data);

    bytes_sent = send(client_sock, req, REQ_SIZE, 0);
    if (bytes_sent <= 0) {
        printf("\nConnection closed!\n");
        return;
    }

    // Receive reply
    memset(res, '\0', RES_SIZE);
    bytes_received = recv(client_sock, res, RES_SIZE, 0);
    if (bytes_received <= 0) {
        printf("\nError! Cannot receive reply from sever!\n");
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

    // Step 1: Construct socket
    client_sock = socket(AF_INET, SOCK_STREAM, 0);

    // Step 2: Specify server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Step 3: Request to connect server
    if (connect(client_sock, (struct sockaddr *)&server_addr,
                sizeof(struct sockaddr)) < 0) {
        printf("\nError! Can not connect to sever! Client exit immediately!\n");
        return 0;
    }

    // Step 4: Communicate with server
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
            ;
        switch (choice) {
            case 1:
                send_text(client_sock);
                break;
            case 2:
                send_file(client_sock);
                break;
            default:
                close(client_sock);
                return 0;
        }
    }

    close(client_sock);
    return 0;
}
