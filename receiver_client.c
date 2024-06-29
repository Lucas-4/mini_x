#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "message.h"


void receive_messages(int sock) {
	msg_t msg;
	while (1) {
		int valread = recv(sock, &msg, sizeof(msg), 0);
		if (valread > 0) {
			printf("Mensagem de %d: %s\n", ntohs(msg.orig_uid), msg.text);
		}
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

	receive_messages(sock);

	close(sock);
	return 0;
}