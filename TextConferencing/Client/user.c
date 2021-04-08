#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "user.h"

struct user USERS[MAX_NUM_USERS] = {
    {
    .sockfd = -1,
    .isLoggedIn = false,
    .ipAddress = "",
    .sessionID = "",
    .clientID = "lance",
    .password = "l"
    },
    {
    .sockfd = -1,
    .isLoggedIn = false,
    .ipAddress = "",
    .sessionID = "",
    .clientID = "sean",
    .password = "s"
    },
    {
    .sockfd = -1,
    .isLoggedIn = false,
    .ipAddress = "",
    .sessionID = "",
    .clientID = "alice",
    .password = "a"
    },
    {
    .sockfd = -1,
    .isLoggedIn = false,
    .ipAddress = "",
    .sessionID = "",
    .clientID = "bob",
    .password = "b"
    },
    {
    .sockfd = -1,
    .isLoggedIn = false,
    .ipAddress = "",
    .sessionID = "",
    .clientID = "charlie",
    .password = "c"
    },
};

void printUserStruct(struct user* u){
    printf("\n===============Printing user============\n");
    printf("sockfd: %d\n", u->sockfd);
    printf("isLoggedIn: %d\n", u->isLoggedIn);
    printf("ipAddy: %s\n", u->ipAddress);
    printf("session: %s\n", u->sessionID);
    printf("client: %s\n", u->clientID);
    printf("password: %s\n\n", u->password);
}
