#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

char *SERVER_IP = "127.0.0.1";
int SERVER_PORT = 5550;
int BUFF_SIZE = 102400;

void get_os_info(char *os_info, int max_len) {
#ifdef __linux__
    struct utsname uname_data;
    if (uname(&uname_data) == 0) {
        snprintf(os_info, max_len,
                 "OS:\nSystem name: %s\nNode name: %s\n"
                 "Release: %s\nVersion: %s\nMachine: %s",
                 uname_data.sysname, uname_data.nodename, uname_data.release,
                 uname_data.version, uname_data.machine);
    } else {
        snprintf(os_info, max_len, "Failed to retrieve OS information");
    }
#else
    snprintf(os_info, max_len, "OS not supported");
#endif
}

void get_hardware_info(char *hardware_info, int max_len) {
#ifdef __linux__
    FILE *fp;
    char buffer[max_len];
    char command[] = "lscpu";

    fp = popen(command, "r");
    if (fp == NULL) {
        snprintf(hardware_info, max_len,
                 "Failed to retrieve hardware information");
        return;
    }

    snprintf(hardware_info, max_len, "HARDWARE:\n");
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        strncat(hardware_info, buffer, max_len - strlen(hardware_info) - 1);
    }

    pclose(fp);
#else
    snprintf(hardware_info, max_len, "OS not supported");
#endif
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

    int client_sock;
    char buff[BUFF_SIZE + 1];
    struct sockaddr_in server_addr; /* Server's address information */
    int msg_len, bytes_sent, bytes_received;

    // Step 1: Construct socket
    client_sock = socket(AF_INET, SOCK_STREAM, 0);

    // Step 2: Specify server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Step 3: Request to connect server
    if (connect(client_sock, (struct sockaddr *)&server_addr,
                sizeof(struct sockaddr)) < 0) {
        printf("\nError! Cannot connect to sever! Client exited immediately!");
        return 0;
    }

    // Step 4: Communicate with server

    // Get operating system information
    char os_info[BUFF_SIZE];
    get_os_info(os_info, BUFF_SIZE);

    // Get hardware information
    char hardware_info[BUFF_SIZE];
    get_hardware_info(hardware_info, BUFF_SIZE);

    // Send both operating system and hardware information
    msg_len = strlen(os_info) + strlen(hardware_info) + 4;
    snprintf(buff, BUFF_SIZE, "%s\n\n%s", os_info, hardware_info);
    bytes_sent = send(client_sock, buff, msg_len, 0);
    if (bytes_sent < 0) perror("\nError: ");

    // Receive echo reply
    bytes_received = recv(client_sock, buff, BUFF_SIZE, 0);
    if (bytes_received < 0)
        perror("\nError: ");
    else if (bytes_received == 0)
        printf("Connection closed\n");

    buff[bytes_received] = '\0';
    printf("Reply from server:\n%s", buff);

    // Step 5: Close socket
    close(client_sock);
    return 0;
}
