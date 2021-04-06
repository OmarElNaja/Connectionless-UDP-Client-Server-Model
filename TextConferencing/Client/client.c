#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "9035"   // port we're listening on
#define MAXBYTES 200

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void produce_message(char *user_in, char *buf, char *store_user_id, int *retval)
{
    memset(buf,0,strlen(buf));

    char temp[200];
    char data_seg[100];
    strcpy(temp, user_in);

    //Tokenize user input by spaces
    char *ptr;
    ptr = strtok(temp, " ");

    //copy first user text segment to data_Seg
    strcpy(data_seg, ptr);

    if (strcmp(user_in, "/logout") == 0 || strcmp(user_in, "/leavesession") == 0 || strcmp(user_in, "/list") == 0 || strcmp(user_in, "/quit") == 0){


        char *type;
        if (strcmp(user_in, "/logout") == 0){
            type = "/logout";
        }
        else if (strcmp(user_in, "/leavesession") == 0){
            type = "/leavesession";
        }
        else if (strcmp(user_in, "/list") == 0){
            type = "/list";
        }
        // else if (strcmp(user_in, "/quit") == 0){
        //     return;
        // }

        strncat(buf, type, strlen(type));
        strncat(buf,":", 2);
        strncat(buf, "0", 2);
        strncat(buf, ":", 2);
        strncat(buf, store_user_id, strlen(store_user_id));
        strncat(buf, ":", 2);
        strncat(buf, "0", 2);
    }

    else if(strcmp(data_seg, "/login") == 0 || strcmp(data_seg, "/joinsession") == 0 || strcmp(data_seg, "/createsession") == 0)
    {
        char *type;
        char *data;
        char size[10];



        if (strcmp(data_seg,"/login") == 0){
            char pass_ip_port[100];
            type = ptr;
            ptr = strtok(NULL," ");
            if (ptr == NULL){
                *retval = 1;
                return;
            }
            char userid[50];
            strcpy(userid, ptr);
            strcpy(store_user_id, userid);
            ptr = strtok (NULL, " ");
            if (ptr == NULL){
                *retval = 1;
                return;
            }
            strcpy(pass_ip_port, ptr);
            strcat(pass_ip_port, ":");
            ptr = strtok (NULL, " ");
            if (ptr == NULL){
                *retval = 1;
                return;
            }
            strcat(pass_ip_port, ptr);
            strcat(pass_ip_port, ":");
            ptr = strtok (NULL, " ");
            strcat(pass_ip_port, ptr);
            strcat(pass_ip_port, ":");
            //printf("data: %s\n", pass_ip_port);
            data = pass_ip_port;
            sprintf(size, "%ld", strlen(pass_ip_port) - 1);

        }

        else if (strcmp(data_seg,"/joinsession") == 0){
            type = ptr;
            ptr = strtok(NULL," ");
            if (ptr == NULL){
                *retval = 1;
                return;
            }
            data = ptr;
            sprintf(size, "%ld", strlen(data) - 1);

        }

        else if (strcmp(data_seg,"/createsession") == 0){
            type = ptr;
            //printf("--%s\n", ptr);
            ptr = strtok(NULL," ");
            if (ptr == NULL){
                //printf("--%s\n", ptr);
                *retval = 1;
                return;
            }
            data = ptr;
            sprintf(size, "%ld", strlen(data) - 1);

        }


        strncat(buf, type, strlen(type));
        strncat(buf,":", 2);
        strncat(buf, size, strlen(size));
        strncat(buf, ":", 2);
        strncat(buf, store_user_id, strlen(store_user_id));
        strncat(buf, ":", 2);
        strncat(buf, data, strlen(data));

    }

    else{
        char *data;
        char size[10];
        data = user_in;
        sprintf(size, "%ld", strlen(data) - 1);

        strncat(buf, "/message", 9);
        strncat(buf,":", 2);
        strncat(buf, size, strlen(size));
        strncat(buf, ":", 2);
        strncat(buf, store_user_id, strlen(store_user_id));
        strncat(buf, ":", 2);
        strncat(buf, data, strlen(data));
    }

}

int main(int argc, char *argv[])
{

    setvbuf(stdout, NULL, _IOLBF,BUFSIZ);

    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int listener;     // listening socket descriptor
    //int newfd;        // newly accept()ed socket descriptor
    //struct sockaddr_storage remoteaddr; // client address
    //socklen_t addrlen;

    char store_user_id[50];
    char user_in[256]; //buffer for client input
    char buf[256];    // buffer for client data
    //int nbytes;
    //int logged_in = 0;
    int retval = 0;

    //char remoteIP[INET6_ADDRSTRLEN];

    if (argc != 2) {
        fprintf(stderr, "usage: client hostname\n");
        exit(1);
    }

    int yes=1;        // for setsockopt() SO_REUSEADDR, below
    int i, rv;

    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    // get a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) {
            continue;
        }

        // get rid of the "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (connect(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    // if we get here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    //initialization complete ----

    printf("Text Conferencing Server connected client, port %d \n", listener);

    freeaddrinfo(ai); // all done with this structure

    int fd_stdin = fileno(stdin);

    // add the listener to the master set
    FD_SET(listener, &master);
    FD_SET(fd_stdin, &master);

    // keep track of the biggest file descriptor
    if(listener > fd_stdin){
        fdmax = listener;
    }
    else{
        fdmax = fd_stdin;
    }


    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    // main loop
    while(1) {
        read_fds = master; // copy it
        //select monitors sets of file descriptors (readfds,writefds,exceptds) and returns the file descriptors
        //that are ready for some sort of interaction (reading or writing).

        //printf("Enter command: ");

        fflush(stdout);

        if (select(fdmax+1, &read_fds, NULL, NULL, &tv) == -1) {
            perror("select");
            exit(4);
        }
        // run through the existing connections looking for data to read
        for(i = 0; i <= fdmax; i++) {

            if (FD_ISSET(i, &read_fds)) { // we got one

                if (i == fd_stdin){
                    memset(user_in,0,strlen(user_in));
                    //int read_bytes = read(i, user_in, MAXBYTES);
                    read(i, user_in, MAXBYTES);
                    if (strcmp(user_in, "/quit\n")==0){
                        if(send(listener, "/quit", sizeof("/quit"), 0) == -1){
                            perror("send");
                        }
                        return 0;
                    }
                    user_in[strcspn(user_in, "\n")] = 0;
                    produce_message(user_in, buf, store_user_id, &retval);
                    if (retval){
                        printf("Error: Not a valid command.\n");
                    }
                    else{
                        if(send(listener, (char*)buf, sizeof(buf), 0) == -1){
                            perror("send");
                        }
                    }
                }
                else{
                    //there is a new connection waiting on the server port
                    memset(buf,0,strlen(buf));
                    //int ret = recv(listener,(char*)buf, sizeof(buf),0);
                    recv(listener,(char*)buf, sizeof(buf),0);
                    printf("Server: %s\n", buf);
                    memset(buf,0,strlen(buf));
                    break;
                }
            }
        } // END looping through file descriptors
    }  // END while(1)

    return 0;
}

