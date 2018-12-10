//============================================================================
// Name        : cross.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <iostream>
#include <cstring>
#include <pthread.h>
#include <atomic>
#include "map_server.h"

using namespace std;

static pthread_t thr_decoder_id;
static atomic<unsigned int> counter;
static map_server msrv;

constexpr uint32_t regS(uint32_t reg) {
	return 5 * 1024 + reg;
}
constexpr uint32_t regG(uint32_t reg) {
	return reg;
}

static void * thr_decoder(void *args) {
	int fsm = 0;
	bool found = false;

	while (true) {
		//проверка содержимого фифо
		int decocerBytes = msrv.read(regS(2)) & 0xffff;
		for (int i = 0; i < decocerBytes; i++) {
			fsm++;
			unsigned int data = msrv.read(regS(0));
			if (data == 0x5A5A1E3C) {
				fsm = 0;
				found = true;
			}
			switch (fsm){
				case 1:
					if (data != 0x8FFFFFFF) found = false;
					break;
				case 2:
					if (data != 0xFFFFFFFF) found = false;
					break;
				case 3:
					if ((data & 0xFFFFFF00) != 0xFFFFFF00) found = false;
					break;
				case 4:
					if (found) counter++;
					break;
			}
			//printf("Decoder: %08X word %d %d\n", data, fsm, (int)found);

		}

		usleep(10000);
	}
	return NULL;
}

int main() {
	cout << "Cross: remote receiver ..." << endl;
	int sock;
	struct sockaddr_in addr;
	uint8_t buf[2048];
	uint16_t port = 20000;
	struct sockaddr_in from;
	int fromlen = sizeof(from);

	printf("own port = %d\n", port);
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		perror("socket");
		exit(1);
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		perror("bind");
		exit(2);
	}

	if (!msrv.map()) {
		perror("map");
		exit(3);
	}
	//open decoder S
	msrv.write(regS(5), 2);

	printf("Threshold    : %08X \n", msrv.read(regG(0x1D)) >> 20);
	printf("Decoder S ID : %08X \n", msrv.read(regS(0x7)));
	pthread_create(&thr_decoder_id, NULL, thr_decoder, NULL);

	while (1) {

		int bytes_read = recvfrom(sock, buf, 2048, 0, (struct sockaddr *) &from,
				(socklen_t *) &fromlen);
		if (bytes_read == -1) {
			printf("error\n");
		} else {
			//printf("in fifo %d words\n", msrv.read(regS(2)) & 0xffff);
			//usleep(50000);
			//unsigned int localCounter = counter.load();
			if (strncmp((const char *) buf, "getCounter", 10) == 0) {
				sendto(sock, (char *) &counter, 4, 0, (struct sockaddr *) &from,
						sizeof(addr));
				//printf("counter=%d\n",localCounter);
				counter = 0;
			}
		}
	}

	return 0;
}
