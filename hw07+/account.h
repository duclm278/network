#define MAXLEN 100

#define ACC_ACTIVE 1
#define ACC_BLOCKED 0

#define SIGN_IN_OK 1
#define SIGN_IN_FAILED 2
#define SIGN_IN_BLOCKED 3
#define SIGN_IN_NOT_READY 4
#define SIGN_IN_ALREADY 5

typedef struct Account {
    char username[MAXLEN];
    char password[MAXLEN];
    int status;
    int failedAttempts;
    struct Account *next;
} Account;

typedef struct OnlineUser {
    char username[MAXLEN];
    struct OnlineUser *next;
} OnlineUser;

char *input(char *prompt);

Account *addAccount(Account *accounts, char *username, char *password,
                    int status);

Account *findAccount(Account *accounts, char *username);

void readDb(Account **accounts);

void writeDb(Account *accounts);

int signIn(Account *accounts, OnlineUser **onlineUsers, char *username,
           const char *password);

int strictSignIn(Account *accounts, OnlineUser **onlineUsers, char *username,
                 const char *password);

void signOut(OnlineUser **onlineUsers, char *username);
