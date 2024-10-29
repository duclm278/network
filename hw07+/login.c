#include "login.h"

#include <arpa/inet.h>
#include <stdlib.h>

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
