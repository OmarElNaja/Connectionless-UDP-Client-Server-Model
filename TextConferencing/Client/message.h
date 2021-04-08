#ifndef __MESSAGE__
#define __MESSAGE__
#include "globals.h"

enum controlType {
    LOGIN,      // 0
    LO_ACK,     // 1
    LO_NAK,     // 2
    EXIT,       // 3
    JOIN,       // 4
    JN_ACK,     // 5
    JN_NAK,     // 6
    LEAVE_SESS, // 7
    NEW_SESS,   // 8
    NS_ACK,     // 9
    MESSAGE,    // 10
    QUERY,      // 11
    QU_ACK      // 12
};

struct message {
    unsigned int type; // type of message
    unsigned int size; // length of the data
    unsigned char source[MAX_NAME]; // ID of the client sending the message
    unsigned char data[MAX_DATA]; // the actual
};

void printMessageStruct(struct message * m );

void printRawMessage(unsigned char p[]);

unsigned char * serializeMessage(struct message * m );

struct message * deserializeMessage(unsigned char p[]);

#endif
