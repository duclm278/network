#include <netinet/in.h>

typedef struct Login {
    struct sockaddr_in client;
    struct Login *next;
} Login;

Login *addLogin(Login *logins, struct sockaddr_in client);

Login *findLogin(Login *logins, struct sockaddr_in client);

Login *removeLogin(Login *logins, struct sockaddr_in client);
