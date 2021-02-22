#define BUF_SIZE 1000

struct packet {
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[BUF_SIZE];
};


void stringToPacket(char* str, struct packet *packet){
    int count = 0;
    int start = 0;
    int length = 0;
    char parsed[BUF_SIZE];

    for (int i=0; i<BUF_SIZE; i++){
        if(str[i] == ':'){
            memset(parsed, 0, BUF_SIZE);
            memcpy(parsed, &str[start], length);
            parsed[length] = '\0';
            
            if (count == 0){              
                packet->total_frag = atoi(parsed);
            }
            else if (count == 1){
                packet->frag_no = atoi(parsed);
            }
            else if (count == 2){
                packet->size = atoi(parsed);
            }
            else if (count == 3){
                packet->filename = parsed;
            }
            else if (count == 4){
                memcpy(&(packet->filedata), &str[start], length);
            }
            else {
                fprintf(stderr, "Packet error!\n");
            }
            start = i;
            count++;
            length = 0;
        }
        length++;
    }
    return;
}
