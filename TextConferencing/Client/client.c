/*
** Some code was used from Beej's Guide to Network Programming
** Source: Beej's Guide to Network Programming
** client.c -- a stream socket client demo
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>

#include "../include/globals.h"
#include "../include/message.h"

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int sockfd, numbytes;
    char buf[MAX_INPUT];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    char* cmd;
    char* clientID;
    char* password;
    char* serverIP;
    char* serverPort;//, sessionID;
    char* myIP;
    bool isLoggedIn=false;
    bool isInSession=false;


    // user input variables and delimiter
    char delim[]=" \t\n\r\f\v";

    char myHostBuf[256];
    struct hostent *host_entry;

    gethostname(myHostBuf, sizeof(myHostBuf));
    host_entry = gethostbyname(myHostBuf);

    myIP = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));
    // printf("Host IP: %s\n", myIP);

    while(1){
        while(!isLoggedIn){
            char userInputNotLoggedIn[MAX_INPUT]={0};

            //get user input
            fgets(userInputNotLoggedIn, MAX_INPUT, stdin);

            char *userInputPtr = strtok(userInputNotLoggedIn, delim);

            cmd = userInputPtr;

            while(strcmp(cmd, "/login")!=0 && strcmp(cmd, "/quit")!=0){
                printf("Not a valid command. Did you mean \"/login\" or \"/quit\"?\n");
                fgets(userInputNotLoggedIn, MAX_INPUT, stdin);
                
                userInputPtr = strtok(userInputNotLoggedIn, delim);
                cmd = userInputPtr;
            }

            if(strcmp(cmd, "/login")==0){
                userInputPtr = strtok(NULL, delim);
                clientID = malloc(strlen(userInputPtr)+1);
                memcpy(clientID, userInputPtr, strlen(userInputPtr)+1);
                // clientID = userInputPtr;

                userInputPtr = strtok(NULL, delim);
                password = userInputPtr;

                userInputPtr = strtok(NULL, delim);
                serverIP = userInputPtr;

                userInputPtr = strtok(NULL, delim);
                serverPort = userInputPtr;
            }else if(strcmp(cmd, "/quit")==0){
                printf("client: Goodbye!\n");
                exit(0);
            }

            memset(&hints, 0, sizeof hints);
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;

            if ((rv = getaddrinfo(serverIP, serverPort, &hints, &servinfo)) != 0) {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                printf("Please try again and check your input.\n");
                continue;
            }

            // loop through all the results and connect to the first we can
            for(p = servinfo; p != NULL; p = p->ai_next) {
                if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                    perror("client: socket");
                    continue;
                }
                if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                    close(sockfd);
                    perror("client: connect");
                    continue;
                }
                break;
            }

            if (p == NULL) {
                fprintf(stderr, "client: failed to connect\n");
                printf("Please try again and check your input.\n");
                continue;
            }

            inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
            printf("client: connecting to %s\n", s);
            freeaddrinfo(servinfo); // all done with this structure

            // initialize login message
            struct message * loginMessage = (struct message*)malloc(sizeof(struct message));

            char* loginData=malloc(MAX_DATA);
            strcpy(loginData, clientID);
            strcat(loginData, ",");
            strcat(loginData, password);
            strcat(loginData, ",");
            strcat(loginData, myIP);

            loginMessage->type = LOGIN;
            loginMessage->size = strlen(loginData);    
            memcpy(loginMessage->source, clientID, strlen(clientID)+1);
            memcpy(loginMessage->data, loginData, loginMessage->size);

            unsigned char* loginPacket = serializeMessage(loginMessage);   

            if (send(sockfd, loginPacket, PACKETLEN, 0) == -1) perror("send");
            
            free(loginData);
            free(loginMessage);

            int numbytes;
            if ((numbytes = recv(sockfd, buf, sizeof buf, 0)) == -1) {
                perror("recv");
                exit(1);
            }
            buf[numbytes] = '\0';

            struct message* loginAckMessage = deserializeMessage((unsigned char*)buf);

            if(loginAckMessage->type==LO_NAK){
                printf("Login failed: %s. Please try again.\n", loginAckMessage->data);
            }else if(loginAckMessage->type==LO_ACK){
                printf("Welcome, %s.\n", clientID);
                // printf("Your password is: %s\n", password);
                // printf("And you want to connect to: %s\n", serverIP);
                // printf("At port number: %s\n", serverPort);
                isLoggedIn=true;
            }else{
                free(loginMessage);
                printf("if we got here, the server didnt send a LO_ACK or LO_NACK soooo something went totally wrong.\n");
            }
            // free(loginMessage);
        }

        fd_set master;
        fd_set read_fds;

        FD_ZERO(&master);
        FD_ZERO(&read_fds);

        FD_SET(STDIN, &master);
        FD_SET(sockfd, &master);

        while(isLoggedIn){

            read_fds = master; // copy it
            if (select(sockfd+1, &read_fds, NULL, NULL, NULL) == -1) {
                perror("select");
                exit(4);
            }

            if(FD_ISSET(sockfd, &read_fds)){ //child will handle listening to messages from the server
                if ((numbytes = recv(sockfd, buf, sizeof buf, 0)) == -1) {
                    perror("recv");
                    exit(1);
                }
                buf[numbytes] = '\0';
                struct message* messageToReceive = deserializeMessage((unsigned char*)buf);

                if(messageToReceive->type == JN_ACK){
                    isInSession=true;
                    printf("Successfully joined \"%s\".\n", messageToReceive->data);
                }else if(messageToReceive->type == JN_NAK){
                    char * attemptedSession = strtok((char*)messageToReceive->data, ",");
                    char * sessionFailureReason = strtok(NULL, "");
                    printf("You failed to join \"%s\" because %s.\n", attemptedSession, sessionFailureReason);
                }else if(messageToReceive->type == NS_ACK){
                    isInSession=true;
                    printf("Successfully created \"%s\".\n", messageToReceive->data);
                }else if(messageToReceive->type == QU_ACK){
                    char* tok = strtok((char*)messageToReceive->data, ",");
                    // if(tok==NULL)break;
                    printf("%s ..... ", tok);
                    tok = strtok(NULL, "|");
                    

                    do{ 
                        printf("%s\n", tok);
                        tok = strtok(NULL, ",");
                        if(tok==NULL)break;
                        printf("%s ..... ", tok);
                        tok = strtok(NULL, "|");
                        if(tok==NULL)break;
                    }while(tok!=NULL);
                }
                else{//message
                    fflush(stdout);
                    printf("%s: %s\n", messageToReceive->source, messageToReceive->data);
                }

                memset(messageToReceive->data, 0, MAX_DATA);  
                free(messageToReceive);

            }else if (FD_ISSET(STDIN, &read_fds)){
                char userInput[MAX_INPUT]={0};
                struct message * myMessage = (struct message*)malloc(sizeof(struct message));

                //get user input
                fgets(userInput, MAX_INPUT, stdin);
                char copyUserInput[MAX_INPUT]={0};
                memcpy(copyUserInput, userInput, MAX_INPUT);

                char *userInputPtr = strtok(userInput, delim);

                if(strcmp(userInputPtr, "/quit")==0 || strcmp(userInputPtr, "/logout")==0){
                    isLoggedIn=false;
                    isInSession=false;
                    myMessage->type=EXIT;
                    myMessage->size=strlen(userInputPtr);
                    memcpy(myMessage->source, clientID, strlen(clientID)+1);
                    memcpy(myMessage->data, userInputPtr, myMessage->size);
                    printf("client: Goodbye!\n");
                }else if(strcmp(userInputPtr, "/joinsession")==0){
                    if(!isInSession){
                        myMessage->type=JOIN;
                        userInputPtr = strtok(NULL, delim);
                        myMessage->size=strlen(userInputPtr);
                        memcpy(myMessage->source, clientID, strlen(clientID)+1);
                        memcpy(myMessage->data, userInputPtr, myMessage->size);
                    }else{
                        printf("Please leave the current session to join another session.\n");
                        continue;
                    }
                }else if(strcmp(userInputPtr, "/createsession")==0){
                    if(!isInSession){
                        myMessage->type=NEW_SESS;
                        userInputPtr = strtok(NULL, delim);
                        myMessage->size=strlen(userInputPtr);
                        memcpy(myMessage->source, clientID, strlen(clientID)+1);
                        memcpy(myMessage->data, userInputPtr, myMessage->size);
                    }else{
                        printf("Please leave the current session to create another session.\n");
                        continue;
                    }
                }else if(strcmp(userInputPtr, "/leavesession")==0){
                    if(!isInSession){
                        printf("You are not in a session.\n");
                        continue;
                    }else{
                        isInSession=false;
                        myMessage->type=LEAVE_SESS;
                        myMessage->size=strlen(userInputPtr);
                        memcpy(myMessage->source, clientID, strlen(clientID)+1);
                        memcpy(myMessage->data,userInputPtr, myMessage->size);
                        printf("Leaving session.\n");
                    }
                }else if(strcmp(userInputPtr, "/list")==0){
                    myMessage->type=QUERY;
                    myMessage->size=strlen(userInputPtr);
                    memcpy(myMessage->source, clientID, strlen(clientID)+1);
                    memcpy(myMessage->data, userInputPtr, myMessage->size);
                }else{ //message
                    if(!isInSession){
                        printf("You are not in a session. Please join a session.\n");
                        continue;
                    }else{
                        myMessage->type=MESSAGE;
                        myMessage->size=strlen(copyUserInput)-1; //minus 1 cuz we dont want the carriage return key
                        memcpy(myMessage->source, clientID, strlen(clientID)+1);
                        memcpy(myMessage->data, copyUserInput, myMessage->size);
                    }
                }
                
                unsigned char* messageToSend = serializeMessage(myMessage);
                free(myMessage);
                
                if (send(sockfd, messageToSend, PACKETLEN, 0) == -1) perror("send"); //change the length
                
                if(strcmp(userInputPtr, "/quit")==0){
                    close(sockfd);
                    exit(0);
                }
                free(messageToSend);
            }
        }
        close(sockfd);
    }
    return 0;
}
