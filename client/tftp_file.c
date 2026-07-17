#include "tftp.h"

// Convert local file format to netascii format
int netascii_encode(char *in, int len, char *out)
{
    int i, j = 0;

    // Loop through input buffer
    for(i = 0; i < len; i++)
    {
        // Convert newline to CRLF
        if(in[i] == '\n')
        {
            out[j++] = '\r';
            out[j++] = '\n';
        }
        // Convert carriage return to CRNULL
        else if(in[i] == '\r')
        {
            out[j++] = '\r';
            out[j++] = '\0';
        }
        // Copy normal character
        else
        {
            out[j++] = in[i];
        }
    }

    // Return encoded length
    return j;
}

// Convert netascii format back to local format
int netascii_decode(char *in, int len, char *out)
{
    int i, j = 0;

    // Loop through received data
    for(i = 0; i < len; i++)
    {
        // Handle CR sequences
        if(in[i] == '\r' && i+1 < len)
        {
            // CRLF -> newline
            if(in[i+1] == '\n')
            {
                out[j++] = '\n';
                i++;
            }
            // CRNULL -> carriage return
            else if(in[i+1] == '\0')
            {
                out[j++] = '\r';
                i++;
            }
            // Unexpected sequence
            else
            {
                out[j++] = in[i];
            }
        }
        // Copy normal character
        else
        {
            out[j++] = in[i];
        }
    }

    // Return decoded length
    return j;
}

// Receive file from server (used for GET)
void receive_file(int sockfd, struct sockaddr_in server_addr,socklen_t len, char *filename, char *mode)
{
    // Open file for writing
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(fd < 0) { perror("open"); return; }

    // Packet structures
    tftp_packet pkt, ack;

    // Actual sender address
    struct sockaddr_in actual;
    socklen_t actual_len = sizeof(actual);

    // Block tracking
    int block = 1, retries = 0, first = 1;

    // Temporary buffer for netascii
    char temp[1024];

    // Receive loop
    while(1)
    {
        // Receive packet
        int n = recvfrom(sockfd, &pkt, sizeof(pkt), 0,(struct sockaddr*)&actual, &actual_len);

        // Handle timeout
        if(n < 0)
        {
            if(++retries > MAX_RETRIES)
            {
                printf("Transfer failed\n");
                close(fd);
                return;
            }
            continue;
        }

        // Lock server port after first packet
        if(first)
        {
            memcpy(&server_addr, &actual, sizeof(server_addr));
            first = 0;
        }

        // Extract opcode
        uint16_t opcode = ntohs(pkt.opcode);

        // Handle error packet
        if(opcode == ERROR)
        {
            printf("Error: %s\n", pkt.body.error_packet.error_msg);
            close(fd);
            return;
        }

        // Handle valid DATA packet
        if(opcode == DATA &&
           ntohs(pkt.body.data_packet.block_number) == block)
        {
            int data_len = n - 4;

            // Decode netascii if required
            if(strcmp(mode,"netascii")==0)
            {
                int d = netascii_decode(pkt.body.data_packet.data, data_len, temp);
                write(fd, temp, d);
            }
            // Write raw data
            else
            {
                write(fd, pkt.body.data_packet.data, data_len);
            }

            // Prepare ACK
            ack.opcode = htons(ACK);
            ack.body.ack_packet.block_number = htons(block);

            // Send ACK
            sendto(sockfd, &ack, 4, 0,(struct sockaddr*)&server_addr, sizeof(server_addr));

            // Print block info
            printf("Received block %d\n", block);

            // Move to next block
            block++;
            retries = 0;

            // Check end of file
            if(data_len < 512)
                break;
        }
        else
        {
            // Resend last ACK for duplicate/out-of-order packet
            ack.opcode = htons(ACK);
            ack.body.ack_packet.block_number = htons(block-1);

            sendto(sockfd, &ack, 4, 0,(struct sockaddr*)&server_addr, sizeof(server_addr));
        }
    }

    // Close file
    close(fd);

    // Completion message
    printf("File received successfully\n");
}

// Send file to server (used for PUT)
void send_file_wrq(int sockfd, struct sockaddr_in server_addr,socklen_t len, char *filename, char *mode)
{
    // Open file for reading
    int fd = open(filename, O_RDONLY);
    if(fd < 0) 
    { 
        perror("open"); 
        return; 
    }

    // Packet structures
    tftp_packet pkt, ack;

    // Actual server address
    struct sockaddr_in actual;
    socklen_t actual_len = sizeof(actual);

    // Block tracking
    int block = 1, retries = 0;

    // Buffers
    char buffer[512], temp[1024];

    // Wait for ACK(0) from server
    int n = recvfrom(sockfd, &ack, sizeof(ack), 0,(struct sockaddr*)&actual, &actual_len);

    // Validate ACK(0)
    if(n < 0 || ntohs(ack.opcode) != ACK || ntohs(ack.body.ack_packet.block_number) != 0)
    {
        printf("ACK(0) failed\n");
        close(fd);
        return;
    }

    // Lock server port
    memcpy(&server_addr, &actual, sizeof(server_addr));

    // Send loop
    while(1)
    {
        // Read file chunk
        int bytes = read(fd, buffer, 512);
        int send_bytes = bytes;

        // Prepare DATA packet
        pkt.opcode = htons(DATA);
        pkt.body.data_packet.block_number = htons(block);

        // Encode netascii if needed
        if(strcmp(mode,"netascii")==0)
        {
            send_bytes = netascii_encode(buffer, bytes, temp);
            memcpy(pkt.body.data_packet.data, temp, send_bytes);
        }
        // Copy raw data
        else
        {
            memcpy(pkt.body.data_packet.data, buffer, bytes);
        }

        // Send DATA packet
        sendto(sockfd, &pkt, send_bytes + 4, 0,(struct sockaddr*)&server_addr, actual_len);

        // Wait for ACK
        n = recvfrom(sockfd, &ack, sizeof(ack), 0,(struct sockaddr*)&actual, &actual_len);

        // Handle timeout
        if(n < 0)
        {
            if(++retries > MAX_RETRIES)
            {
                printf("Transfer failed\n");
                break;
            }
            lseek(fd, -bytes, SEEK_CUR);
            continue;
        }

        // Correct ACK received
        if(ntohs(ack.opcode) == ACK && ntohs(ack.body.ack_packet.block_number) == block)
        {
            block++;
            retries = 0;
        }
        // Wrong ACK, resend block
        else
        {
            lseek(fd, -bytes, SEEK_CUR);
        }

        // Check end of file
        if(bytes < 512)
            break;
    }

    // Close file
    close(fd);

    // Completion message
    printf("PUT: File sent successfully\n");
}