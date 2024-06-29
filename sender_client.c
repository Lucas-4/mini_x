#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "message.h"


void send_messages(int sock, int client_id) {
	msg_t msg;
	msg.type = htons(2);
	msg.orig_uid = htons(client_id);

	while (1) {
		printf("Enter destination ID (0 for broadcast): ");
		scanf("%hu", &msg.dest_uid);
		msg.dest_uid = htons(msg.dest_uid);

		printf("Enter message: ");
		getchar(); // consume newline character
		fgets((char *)msg.text, MAX_TEXT_LEN, stdin);
		msg.text_len = htons(strlen((char *)msg.text));

		send(sock, &msg, sizeof(msg), 0);
	}
}

int main(int argc, char *argv[]) {
	if (argc != 4) {
		fprintf(stderr, "Usage: %s <client_id> <server_ip> <server_port>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	int client_id = atoi(argv[1]);
	char *server_ip = argv[2];
	int server_port = atoi(argv[3]);

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("Socket failed");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_port);

	if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
		perror("Invalid address / Address not supported");
		exit(EXIT_FAILURE);
	}

	if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("Connection failed");
		exit(EXIT_FAILURE);
	}

	msg_t msg;
	msg.type = htons(0);
	msg.orig_uid = htons(client_id);
	send(sock, &msg, sizeof(msg), 0);

	send_messages(sock, client_id);

	close(sock);
	return 0;
}