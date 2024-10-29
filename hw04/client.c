#include <arpa/inet.h> /* These are usual header files. */
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
int BUFF_SIZE = 1024;

int main(int argc, char *argv[]) {
    if (argc != 1 && argc != 3) {
        fprintf(stderr, "Usage: client\n");
        fprintf(stderr, "Usage: client IP PORT\n");
        exit(EXIT_FAILURE);
    } else if (argc == 3) {
        SERVER_IP = argv[1];
        SERVER_PORT = atoi(argv[2]);
    }

    int client_sock; /* File descriptors */
    char buff[BUFF_SIZE];
    struct sockaddr_in server; /* Server's address information */
    struct sockaddr_in client; /* Client's address information */
    int bytes_sent, bytes_received, sin_size;

    // Step 1: Construct UDP socket
    if ((client_sock = socket(AF_INET, SOCK_DGRAM, 0)) <
        0) { /* Call socket() */
        perror("\nError: ");
        exit(EXIT_FAILURE);
    }

    // Step 2: Define address of server
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Step 3.1: Join server
    char username[BUFF_SIZE];
    char password[BUFF_SIZE];
    while (1) {
        memset(buff, '\0', (strlen(buff) + 1));

        fgets(username, BUFF_SIZE, stdin);
        if (username[strlen(username) - 1] == '\n') {
            username[strlen(username) - 1] = '\0';
        }
        if (strcmp(username, "") == 0) {
            close(client_sock);
            exit(EXIT_SUCCESS);
        }

        printf("Insert password\n");
        while (1) {
            fgets(password, BUFF_SIZE, stdin);
            if (password[strlen(password) - 1] == '\n') {
                password[strlen(password) - 1] = '\0';
            }
            if (strcmp(password, "") == 0) {
                close(client_sock);
                exit(EXIT_SUCCESS);
            }
            if (strlen(password) > 0) {
                sprintf(buff, "%s\n%s", username, password);
                break;
            }
        }

        sin_size = sizeof(struct sockaddr_in);
        bytes_sent = sendto(client_sock, buff, strlen(buff), 0,
                            (struct sockaddr *)&server, sin_size);
        if (bytes_sent < 0) {
            perror("\nError: ");
            continue;
        }

        bytes_received = recvfrom(client_sock, buff, BUFF_SIZE - 1, 0,
                                  (struct sockaddr *)&server, &sin_size);
        if (bytes_received < 0) {
            perror("\nError: ");
            continue;
        }

        buff[bytes_received] = '\0';
        if (buff[bytes_received - 1] == '\n') {
            buff[bytes_received - 1] = '\0';
        }
        printf("%s\n", buff);
        if (strcmp(buff, "OK") != 0) {
            continue;
        }

        // Step 3.2 : Communicate with server
        pid_t child_id = fork();
        if (child_id == 0) {
            while (1) {
                memset(buff, '\0', (strlen(buff) + 1));

                fgets(buff, BUFF_SIZE, stdin);
                if (buff[strlen(buff) - 1] == '\n') {
                    buff[strlen(buff) - 1] = '\0';
                }
                if (strcmp(buff, "") == 0) {
                    close(client_sock);
                    kill(getppid(), SIGKILL);
                    exit(EXIT_SUCCESS);
                }

                char msg[BUFF_SIZE];
                memset(msg, '\0', (strlen(msg) + 1));
                sprintf(msg, "%s\n%s", username, buff);

                sin_size = sizeof(struct sockaddr_in);
                bytes_sent = sendto(client_sock, msg, strlen(msg), 0,
                                    (struct sockaddr *)&server, sin_size);
                if (bytes_sent < 0) {
                    perror("\nError: ");
                    close(client_sock);
                    exit(EXIT_FAILURE);
                }
            }
        } else {
            while (1) {
                memset(buff, '\0', (strlen(buff) + 1));

                sin_size = sizeof(struct sockaddr_in);
                bytes_received =
                    recvfrom(client_sock, buff, BUFF_SIZE - 1, 0,
                             (struct sockaddr *)&server, &sin_size);
                if (bytes_received < 0) {
                    perror("\nError: ");
                    close(client_sock);
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
                    kill(child_id, SIGKILL);
                    break;
                }
            }
        }
    }

    close(client_sock);
    return 0;
}
