#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>

#define BUFFER_SIZE 64
#define MAXIMUM_QUEUE 8
#define PORT 2507
#define BROADCAST_ADDRESS "255.255.255.255"

void handle_error(char * error_message) {
	perror(error_message);
	exit(EXIT_FAILURE);
}

// Errors in this program are sparcely handled.

int main (int argc, char* argv[]) {

	if (argc < 2) {
		printf("You need to send the id of this node\n");
		exit(EXIT_FAILURE);
	}

	int ret; 
	unsigned int number_of_neighbours = argc, neighbours[number_of_neighbours];

	for (int i = 1; i<number_of_neighbours; i++) {
		neighbours[i-1] = atoi(argv[i]);
	}

	// OPEN LISTENING SOCKET

	int listening_fd;
	char buffer[BUFFER_SIZE];
	struct sockaddr_in listening_address;
	
	listening_fd = socket(AF_INET, SOCK_DGRAM, 0);
	
	int reuseAddressPermission = 1;
	ret = setsockopt(listening_fd, SOL_SOCKET, SO_REUSEADDR, (void *) &reuseAddressPermission, sizeof(reuseAddressPermission));
    if (ret) handle_error("setsockopt");
	
	memset((char*) &listening_address, 0, sizeof(listening_address));
	listening_address.sin_family = AF_INET;
	listening_address.sin_addr.s_addr = htonl(INADDR_ANY);
	listening_address.sin_port = htons(PORT);

	bind(listening_fd, (struct sockaddr *) &listening_address, sizeof(listening_address));

	// OPEN WRITING SOCKET
	
	int writing_fd, n;
	struct sockaddr_in broadcast_addr;
		
	memset(&broadcast_addr, 0, sizeof(broadcast_addr));
	char writing_buffer[BUFFER_SIZE];
	writing_fd = socket(AF_INET, SOCK_DGRAM, 0);

	int broadcastPermission = 1;
 	ret = setsockopt(writing_fd, SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission, sizeof(broadcastPermission));
    if (ret) handle_error("setsockopt");

	memset((char*) &broadcast_addr, 0, sizeof(broadcast_addr));
	
	broadcast_addr.sin_family = AF_INET;
	broadcast_addr.sin_addr.s_addr = inet_addr(BROADCAST_ADDRESS);
	broadcast_addr.sin_port = htons(PORT);
		
	int data, internal_sequence_number= -1;

	// "LEADER" NODE HANDLING
	if (neighbours[0] == 0) {
		printf("Leader node initializing...\n");
		internal_sequence_number = 0;
		sleep(1);
		srand(time(NULL));
		data = rand();
		sprintf(buffer, "0-0-%d", data);
		ret =sendto(writing_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &broadcast_addr, sizeof(broadcast_addr));
		if (ret < 0) handle_error("sendto");
	}
	
	// LISTENING LOOP
	int sender_id, sequence_number;

	printf("Node initialized, listening...\n");
	while(1) {

		memset(&buffer, 0, BUFFER_SIZE);
	
		int n = read(listening_fd, buffer, BUFFER_SIZE-1);
	
		sscanf(buffer, "%d-%d-%d", &sender_id, &sequence_number, &data);

		// Simulate a network topology: only compute inputs from neighbouring nodes
		for (int i = 1; i < number_of_neighbours; i++) {
			if (sender_id == neighbours[i]) {
				
				// If the package's sequence number is higher than the internal one, discard it.
				if (sequence_number>=internal_sequence_number && internal_sequence_number != -1) break;
				
				internal_sequence_number = sequence_number;
				sprintf(buffer, "%d-%d-%d", neighbours[0], ++internal_sequence_number, data);

				printf("Recieved data package: %d\n", data);
				fflush(stdout);

				ret =sendto(writing_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &broadcast_addr, sizeof(broadcast_addr));
				if (ret < 0) handle_error("sendto");

				break; // We can break early from this loop
			}
		}

		
	}
	
	int sock_out = socket(AF_INET, SOCK_STREAM, 0);
}