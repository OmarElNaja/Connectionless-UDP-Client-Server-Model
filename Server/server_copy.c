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

// Beej's Guide, page 60

#define BUF_SIZE 4096

int main(int argc, char const *argv[]){
    if(argc != 2){
        fprintf(stdout, "Usage: server <server port num>");
        exit(1);
    }
    int SERVERPORT = atoi(argv[1]);

    int sockfd;  // listen on sockfd 
    //struct addrinfo hints, *servinfo, *p;
    //struct sockaddr_storage their_addr; // connector's address information    
    struct sockaddr_in server_addr;
    char buf[BUF_SIZE]={0};
    socklen_t addr_len;

    // we are packing struct by hand (see pg 73)
  
    server_addr.sin_family = AF_INET;    // use 4/6
    //server_addr.sin_socktype = SOCK_DGRAM;   
    //server_addr.sin_flags = AI_PASSIVE; // use my IP
    //server_addr.sin_protocol = IPPROTO_UDP; // UDP protocol  
    server_addr.sin_port = htons(SERVERPORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));  

   /*if ((rv = getaddrinfo(NULL, SERVERPORT, &hints, &servinfo)) != 0) { 
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }*/
    
    //make a socket
    sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    printf("we made a socket\n");
    if (sockfd < 0) {
        return 0;
        printf("socket has issues.");
    }
    
    // bind it to port
    if (bind(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1){
        fprintf(stderr, "wtf its not binding");
        close(sockfd);
        return 0;
    }
    printf("Server here, I'm just waiting");
    
    // receive from client
    if (recvfrom(sockfd, buf, BUF_SIZE, 0, (struct sockaddr*) &server_addr, &addr_len) == -1){
        fprintf(stderr, "Failed to receive from client\n");
        exit(1);
    }

    if (strcmp(buf, "ftp") == 0){
        if (sendto(sockfd, "yes", strlen("yes"), 0, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1){
            fprintf(stderr, "Failed to send message back to client");
            exit(1);
        }
    } else {
        if (sendto(sockfd, "no", strlen("no"), 0, (struct sockaddr*) &server_addr, sizeof(server_addr)) == -1){
            fprintf(stderr, "Failed to send message back to client");
            exit(1);
        }
    }

    close(sockfd);
    return 0;
}
