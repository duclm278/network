#include <bits/stdc++.h>
using namespace std;
struct Account {
    string username;
    string password;
    int status;  // 1: active, 0: blocked
    int failedAttempts;
    Account *next;
};
Account *accounts = NULL;  // Head of linked list
unordered_set<string> onlineUsers;

string input(string message) {
    string s;
    cout << message;
    cin >> s;
    return s;
}

Account *addAccount(string username, string password, int status) {
    Account *newAccount = new Account;
    newAccount->username = username;
    newAccount->password = password;
    newAccount->status = status;
    newAccount->failedAttempts = 0;
    newAccount->next = accounts;
    accounts = newAccount;
    return newAccount;
}

Account *findAccount(string username) {
    Account *cur = accounts;
    while (cur != NULL) {
        if (cur->username == username) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

void readDb() {
    ifstream fin("account.txt");
    string username, password;
    int status;
    while (fin >> username >> password >> status) {
        addAccount(username, password, status);
    }
    fin.close();
}

void writeDb() {
    ofstream fout("account.txt");
    Account *cur = accounts;
    while (cur != NULL) {
        fout << cur->username << " " << cur->password << " " << cur->status
             << "\n";
        cur = cur->next;
    }
    fout.close();
}

void signUp() {
    string username = input("Username: ");
    if (findAccount(username) != NULL) {
        cout << "Account existed\n";
        return;
    }
    string password = input("Password: ");

    // Create new account of status = 1 (active) & failedAttempts = 0
    addAccount(username, password, 1);

    // Add new account to database
    ofstream fout("account.txt", ios::app);
    fout << username << " " << password << " 1\n";
    cout << "Successful registration\n";
    fout.close();
}

void signIn() {
    string username = input("Username: ");
    Account *account = findAccount(username);
    if (account == NULL) {
        cout << "Cannot find account\n";
        return;
    }
    if (account->status == 0) {
        cout << "Account is blocked\n";
        return;
    }
    string password = input("Password: ");
    if (account->password != password) {
        // Update failedAttempts
        if (++account->failedAttempts == 3) {
            // Lock account
            account->status = 0;
            cout << "Password is incorrect. Account is blocked\n";

            // Update database
            writeDb();
        } else {
            cout << "Password is incorrect\n";
        }
        return;
    }

    // Reset failedAttempts
    account->failedAttempts = 0;

    // Add user to onlineUsers
    onlineUsers.insert(username);
    cout << "Hello " << username << "\n";
}

void search() {
    string username = input("Username: ");
    Account *account = findAccount(username);
    if (account == NULL) {
        cout << "Cannot find account\n";
        return;
    } else {
        cout << "Account is ";
        if (account->status == 1) {
            cout << "active\n";
        } else {
            cout << "blocked\n";
        }
    }
}

void signOut() {
    string username = input("Username: ");
    Account *account = findAccount(username);
    if (account == NULL) {
        cout << "Cannot find account\n";
        return;
    }
    if (onlineUsers.find(username) == onlineUsers.end()) {
        cout << "Account is not sign in\n";
        return;
    }
    onlineUsers.erase(username);
    cout << "Goodbye " << username << "\n";
}

int main() {
    // ios_base::sync_with_stdio(false);
    // cin.tie(NULL);
    // freopen("input.txt", "r", stdin);

    readDb();
    int choice;
    while (true) {
        cout << "USER MANAGEMENT PROGRAM\n";
        cout << "-----------------------\n";
        cout << "1. Sign up\n";
        cout << "2. Sign in\n";
        cout << "3. Search\n";
        cout << "4. Sign out\n";
        cout << "Your choice (1-4, other to quit): ";
        cin >> choice;
        switch (choice) {
            case 1:
                signUp();
                break;
            case 2:
                signIn();
                break;
            case 3:
                search();
                break;
            case 4:
                signOut();
                break;
            default:
                return 0;
        }
        cout << "\n";
    }
    return 0;
}
