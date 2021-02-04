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

//#define SERVERPORT "4950" 
#define BUF_SIZE 1024
// the port users will be connecting to
int main(int argc, char *argv[])
{
    int sockfd;
    //struct addrinfo hints, *servinfo, *p;
    //int rv;
    int numbytes;
    if (argc != 3) {
        fprintf(stderr,"usage: talker hostname message\n");
        exit(1);
    }
    int SERVERPORT = atoi(argv[2]); 
    struct sockaddr_in server_addr;
    char buf[BUF_SIZE]={0};

    server_addr.sin_family = AF_INET;    // use 4/6
    server_addr.sin_port = htons(SERVERPORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));  

    //make a socket
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    // adding code
    printf("Please input file as follows: ftp <filename>\n");

    char command[BUF_SIZE]= {0};
    char file_name[BUF_SIZE] = {0};
    char *test;
    char *name;
    
    // Get user input
    while (file_name[0] == 0){
        fgets(buf, BUF_SIZE, stdin);
        test = strtok(buf, " ");
        if (test!=NULL) strcpy(command, test);
        name = strtok(NULL, " \n");
        if (name!=NULL) strcpy(file_name, name);
        else printf("Usage: ftp <filename>\n");
    }

    // Error check
    if (access(file_name, F_OK)!=0){
        fprintf(stderr, "file does not exist\n");
        exit(1);
    }
    else {
        if (sendto(sockfd, command, strlen(command), 0, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0){
            fprintf(stderr, "Failed to send message back to server");
            exit(1);
        }
    }
    
    socklen_t addr_len = sizeof(server_addr);
    numbytes = recvfrom(sockfd, buf, BUF_SIZE, 0, (struct sockaddr*) &server_addr, &addr_len);
    if (numbytes < 0){
        fprintf(stderr, "Failed to receive message from server");
        exit(1);
    }
    buf[numbytes]='\0';

    if(strcmp(buf, "yes") == 0){
        printf("A file transfer can start\n");
    } else {
        printf("Usage: ftp <filename>\n");
    }

    close(sockfd);
    return 0; 
}
