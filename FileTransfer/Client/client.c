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
#include "FileTransfer/packet.h"
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
        printf("Usage: ftp <filename>\n");
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
    fseek(fp, 0L, SEEK_SET);
    
    // total packets required to send file
    unsigned int file_total_frag = file_bytes / 1000;
    if(file_bytes%1000 != 0)
        file_total_frag++;
    
    unsigned int frag_no = 1;
    
    while(frag_no <= file_total_frag) {
        unsigned char file_buffer[1000];
        fread(file_buffer, sizeof(file_buffer), 1, file_ptr); // read contents of file
        
        struct packet pack;
        
        pack.total_frag = file_total_frag;
        char* total_frag_str;
        itoa(pack.total_frag, total_frag_str, 10); // convert total_frag to string
        
        pack.frag_no = frag_no;
        char* frag_no_str;
        itoa(pack.frag_no, frag_no_str, 10); // convert frag_no to string
        
        if(file_bytes%1000 == 0)
            pack.size = 1000;
        else {
            if(frag_no < file_total_frag)
                pack.size = 1000;
            else 
                pack.size = file_bytes%1000;
        }
        char* size_str;
        itoa(pack.size, size_str, 10); // convert size to string
        
        strcpy(pack.filename, file_name);
        strcpy(pack.filedata, file_buffer);
        
        // Build the packet string
        char* pack_str;
        strcpy(pack_str, total_frag_str);
        strcat(pack_str, ":");
        strcat(pack_str, frag_no_str);
        strcat(pack_str, ":");
        strcat(pack_str, size_str);
        strcat(pack_str, ":");
        strcat(pack_str, pack.filename);
        strcat(pack_str, ":");
        strcat(pack_str, pack.filedata);
   
        if(sendto(sockfd, pack_str, strlen(pack_str), 0, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
            fprintf(stderr, "Failed to send packet to server");
            exit(1);
        }
        
        frag_no++;
    }

    close(sockfd);
    return 0; 
}
