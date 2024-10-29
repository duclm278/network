#include "account.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *input(char *prompt) {
    char *s = (char *)malloc(MAXLEN);
    printf("%s", prompt);
    scanf("%s", s);
    return s;
}

Account *addAccount(Account *accounts, char *username, char *password,
                    int status) {
    Account *newAccount = (Account *)malloc(sizeof(Account));
    strcpy(newAccount->username, username);
    strcpy(newAccount->password, password);
    newAccount->status = status;
    newAccount->failedAttempts = 0;
    newAccount->next = accounts;
    accounts = newAccount;
    return newAccount;
}

Account *findAccount(Account *accounts, char *username) {
    Account *cur = accounts;
    while (cur != NULL) {
        if (strcmp(cur->username, username) == 0) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

void readDb(Account **accounts) {
    FILE *fin = fopen("account.txt", "r");
    char username[MAXLEN], password[MAXLEN];
    int status;
    while (fscanf(fin, "%s %s %d", username, password, &status) != EOF) {
        *accounts = addAccount(*accounts, username, password, status);
    }
    fclose(fin);
}

void writeDb(Account *accounts) {
    FILE *fout = fopen("account.txt", "w");
    Account *cur = accounts;
    while (cur != NULL) {
        fprintf(fout, "%s %s %d\n", cur->username, cur->password, cur->status);
        cur = cur->next;
    }
    fclose(fout);
}

int signIn(Account *accounts, OnlineUser **onlineUsers, char *username,
           const char *password) {
    Account *account = findAccount(accounts, username);
    if (account == NULL) {
        return SIGN_IN_NOT_READY;
    }
    if (account->status == 0) {
        return SIGN_IN_NOT_READY;
    }
    if (strcmp(account->password, password) != 0) {
        // Update failedAttempts
        if (++account->failedAttempts == 3) {
            // Lock account
            account->status = ACC_BLOCKED;
            writeDb(accounts);

            return SIGN_IN_BLOCKED;
        } else {
            return SIGN_IN_FAILED;
        }
    }

    // Reset failedAttempts
    account->failedAttempts = 0;

    // Add user to onlineUsers
    OnlineUser *newOnlineUser = (OnlineUser *)malloc(sizeof(OnlineUser));
    strcpy(newOnlineUser->username, username);
    newOnlineUser->next = *onlineUsers;
    *onlineUsers = newOnlineUser;
    return SIGN_IN_OK;
}

int strictSignIn(Account *accounts, OnlineUser **onlineUsers, char *username,
                 const char *password) {
    // Return SIGN_IN_ALREADY if user is already online
    OnlineUser *cur = *onlineUsers;
    while (cur != NULL) {
        if (strcmp(cur->username, username) == 0) {
            return SIGN_IN_ALREADY;
        }
        cur = cur->next;
    }

    // Else, try to sign in
    return signIn(accounts, onlineUsers, username, password);
}

void signOut(OnlineUser **onlineUsers, char *username) {
    OnlineUser *cur = *onlineUsers, *prev = NULL;
    while (cur != NULL) {
        if (strcmp(cur->username, username) == 0) {
            if (prev == NULL) {
                *onlineUsers = cur->next;
            } else {
                prev->next = cur->next;
            }
            free(cur);
            break;
        }
        prev = cur;
        cur = cur->next;
    }
}
