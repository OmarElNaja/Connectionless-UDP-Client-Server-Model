#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "message.h"

#define BUF_SIZE 1096

char* file_name = "login.txt";
struct user_list *user_list; // global
struct sessions_list *sessions_list;

// beej programming pg 42
void *get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) return &(((struct sockaddr_in*)sa)->sin_addr);
}

int main(int argc, char *argv[]){
    if (!argv[1]){
        printf("usage: ./server <num>\n");
        return -1;
    }

    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()    
    int fdmax;        // maximum file descriptor number    
    int listener;     // listening socket descriptor    
    int newfd;        // newly accept()ed socket descriptor    
    struct sockaddr_storage remoteaddr; // client address    
    socklen_t addrlen;    
    char buf[BUF_SIZE];    // buffer for client data    
    int nbytes;    
    char remoteIP[INET6_ADDRSTRLEN];    
    int yes=1;        // for setsockopt() SO_REUSEADDR, below    
    int i, rv;    
    struct addrinfo hints, *ai, *p;    
    
    FD_ZERO(&master);    // reset data  
    FD_ZERO(&read_fds);    
       
    memset(&hints, 0, sizeof hints);    
    hints.ai_family = AF_UNSPEC;    
    hints.ai_socktype = SOCK_STREAM;    
    hints.ai_flags = AI_PASSIVE;    
    
    if ((rv = getaddrinfo(NULL, argv[1], &hints, &ai)) != 0) {        
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));  
        exit(1);
    }
    for(p = ai; p != NULL; p = p->ai_next) {        
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);        
        if (listener < 0) {             
            continue;        
        }        // lose the pesky "address already in use" error message        
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));        
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);            
            continue;        
        }        
        break;    
    }
    // bindsocket failed   
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");        
        exit(2);    
    }    
    freeaddrinfo(ai); // all done with this    
    // listen    
    if (listen(listener, 10) == -1) {        
        perror("listen");        
        exit(3);    
    }    
    // add listener to master  
    FD_SET(listener, &master);       
    fdmax = listener; 
  
    for(;;) {        
        read_fds = master;    
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {            
            perror("select");            
            exit(4);        
        }       
        // run through the existing connections looking for data to read        
        for(i = 0; i <= fdmax; i++) {            
            if (FD_ISSET(i, &read_fds)) {            
                if (i == listener) {                    
                    // handle new connections                    
                    addrlen = sizeof remoteaddr;                    
                    newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
                    
                    if (newfd == -1) {                        
                        perror("accept");                    
                    } else {                        
                        FD_SET(newfd, &master); // add to master set                        
                        if (newfd > fdmax) {    // keep track of the max                            
                            fdmax = newfd;                        
                        }                        
                        printf("selectserver: new connection from %s on socket %d\n", 
                            inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr*)&remoteaddr), remoteIP, INET6_ADDRSTRLEN), newfd);
                    }
                } else {
                    // handle data from a client
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client                        
                        if (nbytes == 0) {                            
                            // connection closed                            
                            printf("selectserver: socket %d hung up\n", i);                        
                        } else {                            
                            perror("recv");                        
                        }                        
                        close(i);                       
                        FD_CLR(i, &master); // remove from master set
                    } else {
                        // decrypt msg
                        struct message *message;
                        stringToPacket(buf, message);
                        printPacket(message);                        

                        if(message->type == LOGIN){
                            char* username = strtok((char*)message->data, " ");
                            char* password = strtok(NULL, " ");
                            // look for user in database
                            struct user *new_user = searchDatabase(username, file_name);
                            new_user->sock_fd = i; 

                            if (new_user == NULL){
                                char *msg = "Error: user not found.\n";
                                sprintf(buf, "%d:%ld:%s:%s", LO_NAK, strlen(msg), "client", msg);
                            } else if (searchID(user_list, username)){
                                char *msg = "Error: already logged in.\n";
                                sprintf(buf, "%d:%ld:%s:%s", LO_NAK, strlen(msg), "client", msg);
                            } else {
                                // check password
                                if (strcmp(new_user->pwd, password) != 0){
                                    char *msg = "Error: incorrect password.\n";
                                    sprintf(buf, "%d:%ld:%s:%s", LO_NAK, strlen(msg), "client", msg);
                                } else {
                                // log in
                                    addID(user_list, new_user);
                                    char *msg = "Success: User has logged in.\n";
                                    sprintf(buf, "%d:%ld:%s:%s", LO_ACK, strlen(msg), "client", msg);
                                }
                            }
                            if (send(i, buf, BUF_SIZE, 0) < 0){
                                fprintf(stderr, "Failed to send message back to client\n");
                                exit(1);
                            }
                        }

                        if (message->type == EXIT){
                            char* username = strtok((char*)message->source, " ");
                            removeID(user_list, username);
                        }

                        if (message->type == JOIN){
                            char sessionID[MAX_DATA];
                            char username[MAX_DATA];
                            strcpy(sessionID, (char*)message->data);
                            strcpy(username, (char*)message->source);

                            if (!findSession(sessions_list, sessionID)){
                                // session not found!
                                char *msg = "Error: Session not found.\n";
                                sprintf(buf, "%d:%ld:%s:%s", JN_NAK, strlen(msg), "client", msg);
                            } 
                            else if (!searchID(user_list, username)){
                                // check if logged in
                                char *msg = "Error: Not logged in!\n";
                                sprintf(buf, "%d:%ld:%s:%s", JN_NAK, strlen(msg), "client", msg);
                            }
                            else {
                                struct user *new_user = searchDatabase(username, file_name);
                                struct sessions *new_session = findSession(sessions_list, sessionID);
                                if (new_user->sessionID){
                                    // check if already in a session (part 1)
                                    char *msg = "Error: Already in a session!";
                                    sprintf(buf, "%d:%ld:%s:%s", JN_NAK, strlen(msg), "client", msg);
                                } else {
                                    if (updateSession(user_list, sessions_list, new_session, new_user)){
                                        char *msg = "Success: Joined session.";
                                        sprintf(buf, "%d:%ld:%s:%s", JN_ACK, strlen(msg), "client", msg);
                                    } else {
                                        char *msg = "Error: Failed to join session.";
                                        sprintf(buf, "%d:%ld:%s:%s", JN_NAK, strlen(msg), "server", msg);
                                    }
                                }
                            }
                            if (send(i, buf, BUF_SIZE, 0) < 0){
                                fprintf(stderr, "Failed to send message back to client\n");
                                exit(1);
                            }
                        }

                        if(message->type== LEAVE_SESS){
                            char username[MAX_DATA];
                            strcpy(username, (char*)message->source);
                            struct user *logout_user = searchDatabase(username, file_name);
                            updateSession(user_list, sessions_list, NULL, logout_user);
                        }

                        if (message->type == NEW_SESS){
                            char sessionID[MAX_DATA];
                            char username[MAX_DATA];
                            strcpy(sessionID, (char*)message->data);
                            strcpy(username, (char*)message->source);
                            
                            struct sessions *new_session = addSession(sessions_list, sessionID);
                            new_session->activeUsers = 1;
                            
                            struct user *user = searchDatabase(username, file_name);
                            strcpy(user->sessionID, sessionID);
                        }

                        if (message->type == MESSAGE){
                            char msg[MAX_DATA];
                            char username[MAX_NAME];
                            strcpy(msg, (char*)message->data);
                            strcpy(username, (char*)message->source);

                            if (!searchID(user_list, username)){
                                // check if logged in
                                char *msg = "Error: Not logged in!\n";
                                sprintf(buf, "%d:%ld:%s:%s", JN_NAK, strlen(msg), "client", msg);
                            }
                            else {
                                sprintf(buf, "%d:%ld:%s:%s", JN_NAK, strlen(msg), "client", msg);
                            }
                            char curr_session[MAX_NAME];
                            strcpy(curr_session, (searchID(user_list, username))->sessionID);
                            // send message to everyone in session
                            while(user_list){
                                if (strcmp(user_list->user->sessionID, curr_session) == 0){
                                    if (send(user_list->user->sock_fd, buf, BUF_SIZE, 0) < 0){
                                        fprintf(stderr, "Error: send\n");
                                        exit(1);
                                    }
                                }
                                user_list = user_list->next;
                            }
                        }

                        if (message->type == QUERY){
                            // print online users
                            char online[MAX_DATA];
                            memset(online, 0, MAX_DATA);

                            strcat(online, "Online users:\n");
                            while(user_list){
                                strcat(online, user_list->user->usr);
                                strcat(online, "\n");
                            }
                            strcat(online, "\n");
                            // print sessions
                            strcat(online, "Available sessions:\n");
                            while(sessions_list){
                                strcat(online, sessions_list->session->sessionID);
                                strcat(online, "\n");
                            }
                            // send to client
                            sprintf(buf, "%d:%ld:%s:%s", QU_ACK, strlen(online), "client", online);
                            if (send(i, buf, BUF_SIZE, 0) < 0){
                                fprintf(stderr, "Error: send\n");
                                exit(1);
                            } 
                        }                  
                    }                
                } // END handle data from client            
            } // END got new incoming connection        
        } // END looping through file descriptors    
    } // END for(;;)
    return 0;
}

