#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void getIp(char *name);
void getName(char *ip);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: resolver name | ip\n");
        exit(EXIT_FAILURE);
    }

    if (inet_addr(argv[1]) == INADDR_NONE) {
        getIp(argv[1]);
    } else {
        getName(argv[1]);
    }

    return 0;
}

void getIp(char *name) {
    struct hostent *he;
    struct in_addr **addr_list;

    he = gethostbyname(name);
    if (he == NULL) {
        fprintf(stderr, "Not found information\n");
        exit(EXIT_FAILURE);
    }

    printf("Official IP: %s\n",
           inet_ntoa(*(struct in_addr *)he->h_addr_list[0]));

    printf("Alias IP: \n");
    addr_list = (struct in_addr **)he->h_addr_list;
    for (int i = 1; addr_list[i] != NULL; i++) {
        printf("%s\n", inet_ntoa(*addr_list[i]));
    }
}

void getName(char *ip) {
    struct hostent *he;
    struct in_addr addr;

    inet_aton(ip, &addr);
    he = gethostbyaddr(&addr, sizeof(addr), AF_INET);
    if (he == NULL) {
        fprintf(stderr, "Not found information\n");
        exit(EXIT_FAILURE);
    }

    printf("Official name: %s\n", he->h_name);

    printf("Alias name: \n");
    for (int i = 0; he->h_aliases[i] != NULL; i++) {
        printf("%s\n", he->h_aliases[i]);
    }
}
