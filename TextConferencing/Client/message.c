#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "message.h"

void printMessageStruct(struct message * m){
    printf("\n===============Printing message============\n");
    printf("type: %d\n", m->type);
    printf("size: %d\n", m->size);
    printf("source: %s\n", m->source);
    printf("data: %s\n", m->data);
}

void printRawMessage(unsigned char p[] ){
    // for(int i = 0; i<PACKETLEN; i++){
    //     // printf("%c", p[i]);
    //     printf("%X ", p[i]);
    // }

    for(int i = 0; i<PACKETLEN; i++){
        printf("%c", p[i]);
        // printf("%X ", p[i]);
    }

    printf("\n");
    // printf("%s\n", deserialPacket);
}

unsigned char* serializeMessage(struct message * m ){
    unsigned char* serialPacket=malloc(PACKETLEN);
    memset(serialPacket, 0, PACKETLEN);
    int serialPacketOffset=0;

    //create packet
    
    //append type to packet
    int tempLength = snprintf( NULL, 0, "%d", m->type); //converts string to int
    char* tempStr = malloc(tempLength+1); //allocates memory for new string
    snprintf( tempStr, tempLength+1 , "%d", m->type); //converts the int to string and saves it in variable
    memcpy(serialPacket+serialPacketOffset, tempStr, tempLength); //append the string to the packet
    free(tempStr); 
    serialPacketOffset+=tempLength; //update packet offset so memcpy doesn't overwrite previously appended info

    memcpy(serialPacket+serialPacketOffset, ":", 1); 
    serialPacketOffset++;

    //append size to packet
    tempLength = snprintf( NULL, 0, "%d", m->size); 
    tempStr = malloc(tempLength+1);
    snprintf( tempStr, tempLength+1 , "%d", m->size);
    memcpy(serialPacket+serialPacketOffset, tempStr, tempLength);
    free(tempStr);
    serialPacketOffset+=tempLength;

    memcpy(serialPacket+serialPacketOffset, ":", 1);
    serialPacketOffset++;

    //append the file name to packet
    memcpy(serialPacket+serialPacketOffset, m->source, strlen((char *)m->source));
    serialPacketOffset=serialPacketOffset+ strlen((char *)m->source)+1;

    memcpy(serialPacket+serialPacketOffset, ":", 1);
    serialPacketOffset++;

    //append the file data to packet
    memcpy(serialPacket+serialPacketOffset, m->data, m->size);
    serialPacketOffset+=m->size;

    return &serialPacket[0];
}

struct message * deserializeMessage(unsigned char p[]){
    //initialize new packet struct incase recreation of packet is necessary
    struct message* myMessage=(struct message*)malloc(sizeof(struct message));
    memset(myMessage->source, 0, MAX_NAME);
    memset(myMessage->data, 0, MAX_DATA);  

    //start deserializing the packet
    char* packetDelim=":";

    //fill in type field
    char* tok = strtok((char*)p, packetDelim);
    int type = strtol(tok, NULL, 10);
    myMessage->type=type;
    
    //fill in the size field
    tok = strtok(NULL, packetDelim);
    int payloadSize = strtol(tok, NULL, 10);
    myMessage->size=payloadSize;        

    //fill in the source field
    char* source = strtok(NULL, "\0");
    int sourceLen = strlen(source);
    memcpy(myMessage->source, source, sourceLen);

    //fill in the filedata field
    char* fileData = strtok(source+sourceLen+2, "\n");
    memcpy(myMessage->data, fileData, payloadSize);
    
    return myMessage;
}
