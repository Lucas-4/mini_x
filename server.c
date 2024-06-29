#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include "message.h"

#define PORT 8090
#define MAX_CLIENTS 20
#define SERVER_ID 0

int client_sockets[MAX_CLIENTS];
int client_ids[MAX_CLIENTS];
int num_clients = 0;
time_t start_time;
int timer_expired = 0;

void handle_timer(int sig) {
	timer_expired = 1;
}

void setup_timer() {
	struct sigaction sa;
	struct itimerval timer;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &handle_timer;
	sigaction(SIGALRM, &sa, NULL);

	timer.it_value.tv_sec = 60;
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = 60;
	timer.it_interval.tv_usec = 0;

	setitimer(ITIMER_REAL, &timer, NULL);
}

void send_periodic_message() {
	char text[MAX_TEXT_LEN];
	msg_t msg;

	msg.type = htons(2);
	msg.orig_uid = htons(SERVER_ID);
	msg.dest_uid = htons(0);

	time_t current_time = time(NULL);
	int elapsed_time = (int)difftime(current_time, start_time);

	snprintf(text, MAX_TEXT_LEN, "Servidor: %d clientes conectados. Tempo ativo: %d segundos.",
		num_clients, elapsed_time);
	msg.text_len = htons(strlen(text));
	strncpy((char *)msg.text, text, MAX_TEXT_LEN);

	for (int i = 0; i < num_clients; i++) {

		send(client_sockets[i], &msg, sizeof(msg), 0);

	}
}

void handle_client_message(int client_socket, msg_t *msg) {

	msg->type = ntohs(msg->type);
	msg->orig_uid = ntohs(msg->orig_uid);
	msg->dest_uid = ntohs(msg->dest_uid);
	msg->text_len = ntohs(msg->text_len);

	msg_t *to_net_msg = malloc(sizeof(msg_t));
	to_net_msg->type = htons(msg->type);
	to_net_msg->orig_uid = htons(msg->orig_uid);
	to_net_msg->dest_uid = htons(msg->dest_uid);
	to_net_msg->text_len = htons(msg->text_len);
	strcpy(to_net_msg->text, msg->text);
	if (msg->type == 0) { // OI

		for (int i = 0; i < MAX_CLIENTS;i++) {
			if (client_ids[i] == 0)
			{
				client_ids[i] = msg->orig_uid;
				break;
			}

		}
		send(client_socket, to_net_msg, sizeof(*to_net_msg), 0);
	}

	else if (msg->type == 2) { // MSG
		for (int i = 0; i < num_clients; i++) {
			if (msg->dest_uid == 0 || client_ids[i] == msg->dest_uid) {
				send(client_sockets[i], to_net_msg, sizeof(*to_net_msg), 0);
			}
		}
	}
}

int main() {
	for (int i = 0; i < MAX_CLIENTS;i++) {
		client_ids[i] = 0;
	}
	int server_socket, new_socket;
	struct sockaddr_in server_addr, client_addr;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	fd_set read_fds;
	int max_sd;
	int opt = 1;
	msg_t msg;

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == 0) {
		perror("Socket failed");
		exit(EXIT_FAILURE);
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
		sizeof(opt));
	if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("Bind failed");
		close(server_socket);
		exit(EXIT_FAILURE);
	}

	if (listen(server_socket, 3) < 0) {
		perror("Listen failed");
		close(server_socket);
		exit(EXIT_FAILURE);
	}

	setup_timer();
	start_time = time(NULL);

	printf("Servidor iniciado na porta %d\n", PORT);

	while (1) {
		FD_ZERO(&read_fds);
		FD_SET(server_socket, &read_fds);
		max_sd = server_socket;

		for (int i = 0; i < num_clients; i++) {
			int sd = client_sockets[i];
			if (sd > 0) {
				FD_SET(sd, &read_fds);
			}
			if (sd > max_sd) {
				max_sd = sd;
			}
		}

		int activity = select(max_sd + 1, &read_fds, NULL, NULL, NULL);

		if ((activity < 0) && (errno != EINTR)) {
			printf("Select error\n");
		}

		if (FD_ISSET(server_socket, &read_fds)) {
			new_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
			if (new_socket < 0) {
				perror("Accept failed");
				exit(EXIT_FAILURE);
			}

			printf("Nova conexão, socket fd: %d, ip: %s, porta: %d\n",
				new_socket, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

			if (num_clients < MAX_CLIENTS) {
				client_sockets[num_clients] = new_socket;
				num_clients++;
			}
			else {
				printf("Número máximo de clientes atingido. Conexão recusada.\n");
				close(new_socket);
			}
		}

		for (int i = 0; i < num_clients; i++) {
			int sd = client_sockets[i];

			if (FD_ISSET(sd, &read_fds)) {
				int valread = recv(sd, &msg, sizeof(msg), 0);
				if (valread == 0) {
					close(sd);
					client_sockets[i] = 0;
					for (int j = i; j < num_clients - 1; j++) {
						client_sockets[j] = client_sockets[j + 1];

					}
					num_clients--;
				}
				else {
					handle_client_message(sd, &msg);
				}
			}
		}

		if (timer_expired) {
			send_periodic_message();
			timer_expired = 0;
		}
	}

	return 0;
}