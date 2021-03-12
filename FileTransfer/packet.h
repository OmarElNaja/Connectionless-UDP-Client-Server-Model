#define BUF_SIZE 1048

struct packet {
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[BUF_SIZE];
};

void stringToPacket(char* str, struct packet *packet){
    char *total_str = strtok(str, ":");
    char *frag_str = strtok(NULL, ":");
    char *size_str = strtok(NULL, ":");

    packet->total_frag = atoi(total_str);
    packet->frag_no = atoi(frag_str);
    packet->size = atoi(size_str);
    packet->filename = strtok(NULL, ":");
    
    int offset = (strlen(total_str) + strlen(frag_str) + strlen(size_str) + strlen(packet->filename) + 4);
    
    memcpy(&(packet->filedata), str+offset, packet->size);
}