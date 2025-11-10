#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define CLNT_MAX 10
#define BUFFSIZE 200

int g_clnt_socks[CLNT_MAX];
int g_clnt_count = 0;

pthread_mutex_t g_mutex;


void send_all_clnt(char* msg, int clnt_sock) {

	pthread_mutex_lock(&g_mutex);

	int str_len = strlen(msg) + 1;
	for(int i = 0; i < g_clnt_count; i++) {
		
		if(g_clnt_socks[i] != clnt_sock) {
		
			printf("send msg: %s", msg);
			write(g_clnt_socks[i], msg, str_len);
		}
	}
	pthread_mutex_unlock(&g_mutex);
}


void* clnt_connection(void* arg) {

	int clnt_sock = (int)arg;
	int str_len;

	char msg[BUFFSIZE];

	while(1) {
		str_len = read(clnt_sock, msg, sizeof(msg));
		if(str_len == 0) {
			printf("clnt[%d] close\n", clnt_sock);
			break;
		}
		send_all_clnt(msg, clnt_sock);
		printf("%s\n", msg);
	}


	pthread_mutex_lock(&g_mutex);

	for(int i = 0; i < g_clnt_count; i++) {
		if(clnt_sock == g_clnt_socks[i]) {
			for(; i < g_clnt_count-1; i++) {
				g_clnt_socks[i] = g_clnt_socks[i+1];
			}
			break;
		}
	}
	
	pthread_mutex_lock(&g_mutex);
	close(clnt_sock);
	pthread_exit(0);

	return NULL;
}


int main(int argc, char** argv) {

	int serv_sock;
	int clnt_sock;

	pthread_t t_thread;

	struct sockaddr_in clnt_addr;
	int clnt_addr_size;

	struct sockaddr_in serv_addr;

	pthread_mutex_init(&g_mutex, NULL);

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	// serv_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	// If you pass 0 as the third argument, its value will be determined by the first and second arguments

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // change host endian to network endian
	serv_addr.sin_port = htons(atoi(argv[1]));

	if(bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
		printf("bind error\n");
	}

	if(listen(serv_sock, 5) == -1) {
		printf("listen error\n");
	}

	char buff[200];
	int recv_len = 0;
	while(1) {

		clnt_addr_size = sizeof(clnt_addr);
		clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);

		pthread_mutex_lock(&g_mutex);
		g_clnt_socks[g_clnt_count++] = clnt_sock;
		pthread_mutex_unlock(&g_mutex);

		pthread_create(&t_thread, NULL, clnt_connection, (void*)clnt_sock);
	}

	return 0;
}
