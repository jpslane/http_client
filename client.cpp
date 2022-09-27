#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAXDATASIZE 500 // max number of bytes we can get at once

// get sockaddr, portv4 or portv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
	char const *PORT = "80"; // default port

	char prot[16] = {0};
	char host[128] = {0};
	char port[16] = {0};
	char fn[2048] = {0};
	char const *true_port;

	if (strstr(argv[1], "localhost") == NULL) {
		const char *format = "%[a-z]://%[a-z.:]/%[a-z/._0-9]";
		sscanf(argv[1], format, prot, host, fn); // given format of argument, returns protocol, host, and filename
		true_port = PORT;
	} else {
		const char *format = "%[a-z]://%[a-z.]:%[0-9]/%[a-z/._0-9]";
		sscanf(argv[1], format, prot, host, port, fn); // given format of argument, returns protocol, host, port, and filename
		true_port = port; // set to localhost port
	}

	FILE *fp = fopen("output", "wb");

	if (strstr(argv[1], "http:") == NULL) {
		fputs("INVALIDPROTOCOL", fp);
		fclose(fp);
		exit(1);
	}

	int sockfd, numbytes;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(host, true_port, &hints, &servinfo)) != 0) {
		fputs("NOCONNECTION", fp);
		fclose(fp);
		return 1;
	}

	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			// perror("client: socket");
			continue;
		}
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			// perror("client: connect");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fclose(fp);
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);


	char request[4096] = {0};
	sprintf(request, "GET /%s HTTP/1.0\r\n\r\n", fn); // create request
	int total_len = strlen(request);
	int n, sent_bytes = 0;

	while(sent_bytes <= total_len) {
		n = send(sockfd, request + sent_bytes, total_len - sent_bytes, 0);
		if (n <= 0) {
			break;
		}
		sent_bytes += n;
	}

	freeaddrinfo(servinfo); // all done with this structure

	int total_bytes = 0, headCut = 0;
	int num_bytes;

	char buf[MAXDATASIZE] = {0};
	char test[5] = {0};
	char exit[5] = "\r\n\r\n";
	char error[5] = "404 ";
	char length[5] = "gth:";
	char file_size[16] = {0};
	do {
		num_bytes = recv(sockfd, buf, 1, 0);

		if (num_bytes == 0) {
			break;
		}
		
		test[0] = test[1];
		test[1] = test[2];
		test[2] = test[3];
		test[3] = *buf;
		
		if (!strncmp(test, length, 4)) {
			recv(sockfd, buf, 16, 0);
			const char *f = " %[0-9]";
			sscanf(buf, f, file_size);
			memset(&buf, 0, sizeof buf);
		}
		else if (!strncmp(test,error, 4)) {
			fputs("FILENOTFOUND", fp); 
			fclose(fp);
			close(sockfd);
			return 0;
		}
		else if (!strncmp(test, exit, 4)) {
			break;
		} 
	} while(1);

	int size;
	sscanf(file_size, "%d", &size);

	int received_bytes = 0;
	memset(&buf, 0, sizeof buf);
	
	do {	
		num_bytes = recv(sockfd, buf, MAXDATASIZE - 1, 0);
		
		received_bytes += num_bytes;
		if (num_bytes == 0 || received_bytes > size) {
			break;
		}
		fwrite(buf, sizeof (char), num_bytes, fp);
	} while(1);
	fclose(fp);
	close(sockfd);
	return 0;
}

