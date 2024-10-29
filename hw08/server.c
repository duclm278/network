#include <arpa/inet.h> /* Usual header files */
#include <errno.h>
#include <fcntl.h>  // for file operations
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>  // for timestamp
#include <unistd.h>

int PORT = 5550;  /* Port that will be opened */
int BACKLOG = 20; /* Number of allowed connections */
int BUFF_SIZE = 102400;

void generateTimestamp(char *timestamp) {
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, 20, "%Y-%m-%d_%H-%M-%S", timeinfo);
}

void saveEchoToFile(char *ip, int port, char *message) {
    char timestamp[20];
    generateTimestamp(timestamp);

    char filename[50];
    sprintf(filename, "%s_%d_%s.txt", ip, port, timestamp);

    int fd = open(filename, O_CREAT | O_WRONLY, 0644);
    if (fd == -1) {
        perror("Error opening file");
        return;
    }

    write(fd, message, strlen(message));
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc != 1 && argc != 2) {
        fprintf(stderr, "Usage: server\n");
        fprintf(stderr, "Usage: server PORT\n");
        exit(EXIT_FAILURE);
    } else if (argc == 2) {
        PORT = atoi(argv[1]);
    }

    int listenfd, connfd, sockfd;
    struct sockaddr_in server; /* Server's address information */
    struct sockaddr_in client; /* Client's address information */
    int sin_size;
    int nbytes;
    char buff[BUFF_SIZE];
    fd_set readfds, testfds;

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) ==
        -1) { /* Call socket() */
        perror("\nError: ");
        return 0;
    }
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr =
        htonl(INADDR_ANY); /* INADDR_ANY puts your IP address automatically */

    if (bind(listenfd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        perror("\nError: ");
        return 0;
    }

    if (listen(listenfd, BACKLOG) == -1) {
        perror("\nError: ");
        return 0;
    }

    sin_size = sizeof(struct sockaddr_in);
    FD_ZERO(&readfds);
    FD_SET(listenfd, &readfds);

    while (1) {
        testfds = readfds;
        select(FD_SETSIZE, &testfds, NULL, NULL, NULL);

        for (int fd = 0; fd < FD_SETSIZE; fd++) {
            if (FD_ISSET(fd, &testfds)) {
                if (fd == listenfd) {
                    connfd =
                        accept(listenfd, (struct sockaddr *)&client, &sin_size);
                    FD_SET(connfd, &readfds);
                    printf("Client %d connected from %s:%d\n", connfd,
                           inet_ntoa(client.sin_addr), ntohs(client.sin_port));
                } else {
                    ioctl(fd, FIONREAD, &nbytes);
                    if (nbytes == 0) {
                        close(fd);
                        FD_CLR(fd, &readfds);
                        printf("Client %d disconnected\n", fd);
                    } else {
                        recv(fd, buff, BUFF_SIZE, 0);
                        printf("Received from client %d: %s\n", fd, buff);
                        send(fd, buff, nbytes, 0);

                        saveEchoToFile(inet_ntoa(client.sin_addr),
                                       ntohs(client.sin_port), buff);
                    }
                }
            }
        }
    }

    close(listenfd);
    return 0;
}
