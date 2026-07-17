#include "tftp.h"

int main()
{
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char buffer[BUFFER_SIZE];

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // Set receive timeout
    struct timeval tv = {TIMEOUT_SEC, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Initialize server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket to server address
    bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    printf("Server running...\n");

    while(1)
    {
        // Set client address length
        client_len = sizeof(client_addr);

        // Receive request from client
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,(struct sockaddr*)&client_addr, &client_len);

        // Ignore timeout or error
        if(n < 0)
            continue;

        // Handle the client request
        handle_client(sockfd, buffer, client_addr, client_len);
    }

    // Close socket
    close(sockfd);
    return 0;
}

// =================================
// HANDLE CLIENT
// =================================

void handle_client(int sockfd, char *buffer,struct sockaddr_in client_addr, socklen_t client_len)
{
    uint16_t opcode;

    // Extract opcode safely from buffer
    memcpy(&opcode, buffer, 2);
    opcode = ntohs(opcode);

    char filename[100], mode[20];
    int len = 2;

    // Extract filename from request
    strcpy(filename, buffer + len);
    len += strlen(filename) + 1;

    // Extract mode from request
    strcpy(mode, buffer + len);

    printf("Request: %s | Mode: %s\n", filename, mode);

    // Create new socket for this transfer (TFTP requirement)
    int new_sock = socket(AF_INET, SOCK_DGRAM, 0);

    // Set timeout for new socket
    struct timeval tv = {TIMEOUT_SEC, 0};
    setsockopt(new_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Handle RRQ (Read Request)
    if(opcode == RRQ)
    {
        // Send file to client
        send_file_rrq(new_sock, client_addr, client_len,filename, mode);
    }

    // Handle WRQ (Write Request)
    else if(opcode == WRQ)
    {
        // Prepare ACK packet for block 0
        tftp_packet ack;
        ack.opcode = htons(ACK);
        ack.body.ack_packet.block_number = htons(0);

        // Send ACK(0) to client
        sendto(new_sock, &ack, 4, 0,(struct sockaddr*)&client_addr, client_len);

        // Receive file from client
        receive_file(new_sock, client_addr, client_len, filename, mode);
    }

    // Handle invalid opcode
    else
    {
        printf("Invalid opcode received: %d\n", opcode);
    }

    // Close transfer socket
    close(new_sock);
}