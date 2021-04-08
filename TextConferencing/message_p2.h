#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_NAME 32
#define MAX_DATA 1048
#define MAX_PWD 32
#define MAX_USERS 32

enum msgType {
    LOGIN,
    LO_ACK,
    LO_NAK,
    EXIT,
    JOIN,
    JN_ACK,
    JN_NAK,
    LEAVE_SESS,
    NEW_SESS,
    NS_ACK,
    MESSAGE,
    QUERY,
    QU_ACK
};

struct message {
    int type;
    int size;
    unsigned source[MAX_NAME];
    unsigned data[MAX_DATA];
};

// User credential information
struct user {
    char usr[MAX_NAME];
    char pwd[MAX_PWD];
    //int online;
    int sock_fd;
    char sessionID[MAX_USERS][MAX_NAME]; //-1 if not in session
};

// We use a linked list to determine who is online
struct user_list{
    struct user *user;
    struct user_list *next;
};

struct sessions{
    int activeUsers;
    char sessionID[MAX_NAME];
};

struct sessions_list{
    struct sessions *session;
    struct sessions_list *next;
};

void stringToPacket(char* str, struct message *message){
    char *type = strtok(str, ":");
    char *size = strtok(NULL, ":");
    char *source = strtok(NULL, ":");
    char *data = strtok(NULL, "");

    message->type = atoi(type);
    message->size = atoi(size);
    
    strcpy((char* )message->source, source);
    strcpy((char* )message->data, data);
}

void printPacket(struct message *message){
    printf("Type: %d\n", message->type);
    printf("Size: %d\n", message->size);
    printf("Source: %ls\n", message->source);
    printf("Data: %ls\n", message->data);
}

struct user* searchID(struct user_list *user, char* username){
    // iterate through linked list until username found
    while (user->next){
        if(strcmp(user->user->usr, username) == 0) return user->user;
    }
    // username not found
    return NULL;
}

// Removes username from active session
void removeID(struct user_list *user, char* username){
    if (!user) return;

    struct user_list *curr = user;
    struct user_list *prev = NULL;

    while(curr){
        if(strcmp(curr->user->usr, username) == 0){
            if (!prev) {
                // if head
                prev = curr;
                curr = curr->next;
                free(prev);
            } else {
                prev->next = curr->next;
                free(curr);
                return;
            }
        } else {
            prev = curr;
            curr = curr->next;
        }
    }
    printf("error, user could not be deleted, NOT FOUND\n");  
}

// Adds user to list of logged in users
void addID(struct user_list *user, struct user *new_user){
    if (!(user)) { //empty
        user->user = new_user;
    } else {
        struct user_list *temp = user;
        user->user = new_user;
        user->next = temp;
    }
}

// Finds the credentials of the user given clientID, returns NULL if DNE
struct user* searchDatabase(char* username, char* file_name){    
    char clientID[MAX_NAME];
    char clientPwd[MAX_NAME];

    FILE *fp = fopen(file_name, "rb");
    if (fp == NULL){
      fprintf(stderr, "Unable to open file\n");
      exit(1);
    }

    int r;
    while(1){
        r = fscanf(fp, "%s %s\n", clientID, clientPwd);
        if(strcmp(username, clientID)==0){
            // Match found, return the credentials
            struct user *user = malloc(sizeof(struct user));
            strcpy(user->usr, clientID);
            strcpy(user->pwd, clientPwd);
            memset(user->sessionID, 0, MAX_NAME);

            fclose(fp);
            return user;
        }
        if (r==EOF) break;
    }
    fclose(fp);
    return NULL;
}

struct sessions* findSession(struct sessions_list *s_list, char* session){
    while(s_list){
        if (strcmp(session, s_list->session->sessionID) == 0) {
            return s_list->session; // session exists
        }
        s_list = s_list->next;
    }
    return NULL; //session does not exist
}

void removeSession(struct sessions_list *s_list, struct sessions *session){
    struct sessions_list *curr = s_list;
    struct sessions_list *prev = NULL;

    while(curr){
        if(curr->session == session){
            if (!prev) {
                // if head
                prev = curr;
                curr = curr->next;
                free(prev);
            } else {
                prev->next = curr->next;
                free(curr);
                return;
            }
        } else {
            prev = curr;
            curr = curr->next;
        }
    }
    printf("error, user could not be deleted, NOT FOUND\n");  
}

// add a new session to head of current sessions list
struct sessions* addSession(struct sessions_list *s_list, char* session_name){
    struct sessions *new_session = malloc(sizeof(struct sessions));
    strcpy(new_session->sessionID, session_name);
    new_session->activeUsers = 1;

    if (!(s_list)) { //empty
        s_list->session = new_session;
        s_list->next = NULL;
    } else {
        struct sessions_list *temp = s_list;
        s_list->session = new_session;
        s_list->next = temp;
    }
    return new_session;
}

int updateSession(struct user_list *user, struct sessions_list *s_list, struct sessions *new_session, struct user *username){
    char[MAX_USERS][MAX_NAME] remove_sessions; // sessions to change because user has left
    
    while(user){
        if (user->user == username) {
            // update data in user struct
            if (new_session == NULL){
                    remove_sessions = user->user->sessionID;
                    user->user->sessionID = {'\0'}
            }
            for(int i=0; i<MAX_USERS; i++){
                if (user->user->sessionID[i] == NULL){
                    // add new session
                    strcpy(user->user->sessionID[i], new_session->sessionID);
                    break;
                }
            }
            while (s_list){
                if(new_session == NULL){
                    int i=0;
                    while(remove_sessions[i]){
                        if(strcmp(s_list->session->sessionID, remove_sessions[i]) == 0) {
                            s_list->session->activeUsers--;
                            if (!s_list->session->activeUsers){
                                // remove session if no users are left
                                removeSession(s_list, s_list->session);
                            }
                        }
                        i++;
                    }
                }
                if (new_session){
                    // if new session exists
                    if(strcmp(s_list->session->sessionID, new_session->sessionID) == 0){
                        s_list->session->activeUsers++;
                    }
                }
                s_list = s_list->next;
            }
            return 1; // success
        }
        user = user->next;
    }
    return -1; // failed
}
