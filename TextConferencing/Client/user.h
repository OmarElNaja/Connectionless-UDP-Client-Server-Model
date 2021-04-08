#ifndef __USER__
#define __USER__

#include <stdbool.h>
#include <arpa/inet.h>

#include "globals.h"

struct user {
    int sockfd;
    bool isLoggedIn;
    unsigned char ipAddress[INET6_ADDRSTRLEN];
    unsigned char sessionID[MAX_NAME];
    unsigned char clientID[MAX_NAME]; // ID of the client sending the message
    unsigned char password[MAX_DATA];
};

extern struct user USERS[MAX_NUM_USERS];

void printUserStruct(struct user* u);

#endif
