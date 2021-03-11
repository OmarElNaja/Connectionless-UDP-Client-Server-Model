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
#include <time.h>

// Will only compile on my computer
#include "/homes/d/dengpris/ECE361/FileTransfer/packet.h"
// To compile on yours, copy path of packet.h, and replace it over my path


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
    struct hostent *remoteHost = gethostbyname(argv[1]);
    //server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_addr =  *(struct in_addr *) remoteHost->h_addr;
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));  

    //make a socket
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    // adding code
    printf("Please input file as follows: ftp <filename>\n");

    char command[BUF_SIZE]= {0};
    char file_name[BUF_SIZE] = {0};
    char *test;
    
    // Get user input
    while (file_name[0] == 0){
        fgets(buf, BUF_SIZE, stdin);
        test = strtok(buf, " ");
        if (test!=NULL) strcpy(command, test);
        test = strtok(NULL, " \n");
        if (test!=NULL) strcpy(file_name, test);
        else printf("Usage: ftp <filename>\n");
    }
    
    clock_t start, end;
    double cpu_time_used;

    // Error check
    if (access(file_name, F_OK)!=0){
        fprintf(stderr, "file does not exist\n");
        exit(1);
    }
    else {
        start = clock();
        if (sendto(sockfd, command, strlen(command), 0, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0){
            fprintf(stderr, "Failed to send message back to server");
            exit(1);
        }
    }
    
    socklen_t addr_len = sizeof(server_addr);
    numbytes = recvfrom(sockfd, buf, BUF_SIZE, 0, (struct sockaddr*) &server_addr, &addr_len);
    
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("The round-trip time is %f seconds\n", cpu_time_used);
    
    if (numbytes < 0){
        fprintf(stderr, "Failed to receive message from server");
        exit(1);
    }
    buf[numbytes]='\0';

    if(strcmp(buf, "yes") == 0){
        printf("A file transfer can start\n");
    } else {
        printf("Closing client connection\n");
        exit(1);
    }
    
    // Section 3
    
    FILE* file_ptr = fopen(file_name, "rb"); // rb for read binary
    
    if (file_ptr == NULL)
    {
      fprintf(stderr, "Unable to open file\n");
      exit(1);
    }

    // calculating the size of the file  in bytes
    fseek(file_ptr, 0L, SEEK_END); 
    unsigned int file_bytes = ftell(file_ptr); 
    fseek(file_ptr, 0L, SEEK_SET);

    // total packets required to send file
    unsigned int file_total_frag = file_bytes / 1000;
    if(file_bytes%1000 != 0)
        file_total_frag++;
    
    unsigned int frag_no = 1;
    
    char frag_no_str[BUF_SIZE];
    char size_str[BUF_SIZE];
    char total_frag_str[BUF_SIZE];
    char pack_str[BUF_SIZE];
    char file_buffer[1000];
    
    // Setting timeout value
    struct timeval timelimit;

    timelimit.tv_sec = 0;
    timelimit.tv_usec = (int)(cpu_time_used*1000000);
    printf("timelimit val: %ld\n", timelimit.tv_usec);
    printf("cpu: %f\n", cpu_time_used*1000000);
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timelimit, sizeof(timelimit)) < 0) {
        printf("Error setting timeout\n");
        exit(1);
    }

    while(frag_no <= file_total_frag) {
        int size;
        //reset:
        memset(pack_str, 0, BUF_SIZE);
        fread(file_buffer, sizeof(char), 1000, file_ptr); // read contents of file
        
        sprintf(total_frag_str, "%d", file_total_frag);
        sprintf(frag_no_str, "%d", frag_no);

        if(file_bytes%1000 == 0)
            size = 1000;
        else {
            if(frag_no < file_total_frag)
               size = 1000;
            else 
               size = file_bytes%1000;
        }

        sprintf(size_str, "%d", size);

        strcpy(pack_str, total_frag_str);
        strcat(pack_str, ":");
        strcat(pack_str, frag_no_str);
        strcat(pack_str, ":");
        strcat(pack_str, size_str);
        strcat(pack_str, ":");
        strcat(pack_str, file_name);
        strcat(pack_str, ":");

        int offset = strlen(pack_str);

        memcpy(&pack_str[offset], file_buffer, size); 
        
        start = clock();
        
        if(sendto(sockfd, pack_str, BUF_SIZE, 0, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
            fprintf(stderr, "Failed to send packet to server");
            exit(1);
        }
        printf("sending: %s\n", frag_no_str);
        
        // Check acknowledgemet
        numbytes  = recvfrom(sockfd, buf, BUF_SIZE, 0, (struct sockaddr*) &server_addr, &addr_len);
        buf[numbytes] = '\0';
        
        end = clock();
        cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        
        timelimit.tv_usec = (int)(cpu_time_used*1000000);
        printf("timelimit val: %ld\n", timelimit.tv_usec);
        printf("cpu: %f\n", cpu_time_used*1000000);
    
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timelimit, sizeof(timelimit)) < 0) {
             printf("Error setting timeout\n");
            exit(1);
        }

        if(numbytes < 0) {
            if(errno == EAGAIN) {
                printf("Timeout receiving ACK num: frag_no: %d. Resending packet.\n",frag_no);
                printf("Timeout seconds: %f\n", cpu_time_used);
                //frag_no--;
                fseek(file_ptr, -size, SEEK_CUR); // go back in file by packet size so it can be resent
                //goto reset;
                continue;
            } else { 
                printf("Error receiving ACK.\n");
                exit(1);
              }
        }

        if(strcmp(buf, "ACK") == 0){
            frag_no++;
        }
    }

    close(sockfd);
    return 0; 
}
