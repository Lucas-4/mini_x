#ifndef MESSAGE_H
#define MESSAGE_H

#define MAX_TEXT_LEN 141
typedef struct {
	unsigned short int type;
	unsigned short int orig_uid;
	unsigned short int dest_uid;
	unsigned short int text_len;
	unsigned char text[MAX_TEXT_LEN];
} msg_t;

#endif