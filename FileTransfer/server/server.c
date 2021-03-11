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
// Will only compile on my computer
#include "/homes/d/dengpris/ECE361/FileTransfer/packet.h"
// To compile on yours, copy path of packet.h, and replace it over my path

// Beej's Guide, page 60

int main(int argc, char const *argv[]){
    if(argc != 2){
        fprintf(stdout, "Usage: server <server port num>");
        exit(1);
    }
    int SERVERPORT = atoi(argv[1]);

    int sockfd;  // listen on sockfd 
    int numbytes;  
    struct sockaddr_in server_addr;
    char buf[BUF_SIZE]={0};
    socklen_t addr_len;

    // we are packing struct by hand (see pg 73)
    server_addr.sin_family = AF_INET;    // use 4/6
    server_addr.sin_port = htons(SERVERPORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));  
    
    //make a socket
    sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        return 0;
    }
    
    // bind it to port
    if (bind(sockfd, (struct sockaddr*) &server_addr, sizeof(server_addr)) <0){
        close(sockfd);
        return 0;
    }
    printf("Server here, I'm just waiting\n");
    // receive from client
    numbytes=recvfrom(sockfd, buf, BUF_SIZE, 0, (struct sockaddr*) &server_addr, &addr_len);
    if (numbytes < 0){
        fprintf(stderr, "Failed to receive from client\n");
        exit(1);
    }
    buf[numbytes] = '\0';
    printf("Response: %s\n", buf);
    // create response
    
    if (strcmp(buf, "ftp") != 0){
        if (sendto(sockfd, "no", strlen("no"), 0, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0){
            fprintf(stderr, "Failed to send message back to client\n");
            exit(1);
        }
    } else {
        if (sendto(sockfd, "yes", strlen("yes"), 0, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0){
            fprintf(stderr, "Failed to send message back to client\n");
            exit(1);
        }
    }
    // -------------- SECTION 3 CODE --------------- //
    printf("Waiting for transfer to start...\n");
    
    FILE *fp;
    struct packet packet;
    int pack_count = 0;

    while(1){
        // Check if packet recieved
        memset(buf, 0, BUF_SIZE);
        memset(&packet, 0, sizeof(packet));
        if (recvfrom(sockfd, buf, BUF_SIZE, 0, (struct sockaddr*) &server_addr, &addr_len) < 0){
            fprintf(stderr, "Failed to receive from client\n");
            exit(1);
        }

        // Parse packet (see packet.h)
        stringToPacket(buf, &packet);
        
        // Create file if packet no. 1
        if (packet.frag_no == 1) {       
            //strcpy(file_name, packet.filename);
            printf("file name is: %s\n", packet.filename);
            fp = fopen(packet.filename, "w");       
        }

        // Write to file
        fwrite(packet.filedata, sizeof(char), packet.size, fp);
        if (packet.frag_no % 50 == 0){
            sleep(1);
        }
        printf("Pack count: %d\n", pack_count);
        printf("Current pack: %d\n", packet.frag_no);
        // Send acknowledgement back to client
        if (packet.frag_no - pack_count == 1){
            if (sendto(sockfd, "ACK", strlen("ACK"), 0, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0){
                fprintf(stderr, "Failed to send message back to client\n");
                exit(1);
            }
            pack_count++;


        } else {
            if (sendto(sockfd, "NACK", strlen("NACK"), 0, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0){
                fprintf(stderr, "Failed to send message back to client\n");
                exit(1);
            }
        }
        // End of file
        if (packet.frag_no == packet.total_frag) {
            printf("I've reached the end of the file.\n");
            break;
        }
    }
    fclose(fp);
    printf("Message has been sent back to client.\n");

    close(sockfd);
    return 0;
}