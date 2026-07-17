#include "tftp.h"

int netascii_encode(char *in, int len, char *out)
{
    // Loop through input buffer
    int i, j = 0;
    for(i = 0; i < len; i++)
    {
        // Convert newline to CRLF
        if(in[i] == '\n')
        {
            out[j++] = '\r';
            out[j++] = '\n';
        }
        // Convert carriage return to CR NULL
        else if(in[i] == '\r')
        {
            out[j++] = '\r';
            out[j++] = '\0';
        }
        // Copy normal characters
        else
        {
            out[j++] = in[i];
        }
    }
    // Return encoded length
    return j;
}

int netascii_decode(char *in, int len, char *out)
{
    // Loop through encoded buffer
    int i, j = 0;
    for(i = 0; i < len; i++)
    {
        // Handle CR sequences
        if(in[i] == '\r' && i+1 < len)
        {
            // CRLF → newline
            if(in[i+1] == '\n')
            {
                out[j++] = '\n';
                i++;
            }
            // CR NULL → carriage return
            else if(in[i+1] == '\0')
            {
                out[j++] = '\r';
                i++;
            }
            // Unexpected case, copy as is
            else
            {
                out[j++] = in[i];
            }
        }
        // Copy normal characters
        else
        {
            out[j++] = in[i];
        }
    }
    // Return decoded length
    return j;
}

void receive_file(int sockfd, struct sockaddr_in server_addr,socklen_t len, char *filename, char *mode)
{
    // Open file for writing
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(fd < 0) { perror("open"); return; }

    tftp_packet pkt, ack;
    struct sockaddr_in actual;
    socklen_t actual_len = sizeof(actual);

    // Initialize block number and retry count
    int block = 1, retries = 0, first = 1;

    char temp[1024];

    while(1)
    {
        // Receive packet from server
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

        // Process valid DATA packet
        if(opcode == DATA && ntohs(pkt.body.data_packet.block_number) == block)
        {
            int data_len = n - 4;

            // Handle netascii decoding
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

            // Send ACK to server
            sendto(sockfd, &ack, 4, 0,(struct sockaddr*)&server_addr, sizeof(server_addr));

            printf("Received block %d\n", block);

            block++;
            retries = 0;

            // Last packet condition
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
    printf("File received successfully\n");
}

void send_file_rrq(int sockfd, struct sockaddr_in client_addr,socklen_t client_len, char *filename, char *mode)
{
    // Open file for reading
    int fd = open(filename, O_RDONLY);
    if(fd < 0) { perror("open"); return; }

    tftp_packet pkt, ack;
    struct sockaddr_in actual;
    socklen_t actual_len = sizeof(actual);

    // Initialize block number and retry count
    int block = 1, retries = 0, first = 1;

    char buffer[512], temp[1024];

    while(1)
    {
        // Read file data
        int bytes = read(fd, buffer, 512);
        int send_bytes = bytes;

        // Prepare DATA packet
        pkt.opcode = htons(DATA);
        pkt.body.data_packet.block_number = htons(block);

        // Apply netascii encoding if required
        if(strcmp(mode,"netascii")==0)
        {
            send_bytes = netascii_encode(buffer, bytes, temp);
            memcpy(pkt.body.data_packet.data, temp, send_bytes);
        }
        else
        {
            memcpy(pkt.body.data_packet.data, buffer, bytes);
        }

        // Send DATA packet to client
        sendto(sockfd, &pkt, send_bytes + 4, 0,(struct sockaddr*)&client_addr, client_len);

        // Wait for ACK
        int n = recvfrom(sockfd, &ack, sizeof(ack), 0,(struct sockaddr*)&actual, &actual_len);

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

        // Lock client port after first response
        if(first)
        {
            memcpy(&client_addr, &actual, sizeof(client_addr));
            first = 0;
        }

        // Check for correct ACK
        if(ntohs(ack.opcode) == ACK &&
           ntohs(ack.body.ack_packet.block_number) == block)
        {
            block++;
            retries = 0;
        }
        else
        {
            lseek(fd, -bytes, SEEK_CUR);
        }

        // Last packet condition
        if(bytes < 512)
            break;
    }

    // Close file
    close(fd);
    printf("GET: File sent successfully\n");
}