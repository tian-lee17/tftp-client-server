#ifndef TFTP_H
#define TFTP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define PORT 6969
#define BUFFER_SIZE 516
#define DATA_SIZE 512
#define TIMEOUT_SEC 5
#define MAX_RETRIES 5

#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4
#define ERROR 5

// Process user command
void process_command(int sockfd, struct sockaddr_in *server_addr,
                     char *command, char *mode);

// Set server IP
void connect_to_server(struct sockaddr_in *server_addr, char *ip);

// Handle GET operation
void get_file_cmd(int sockfd, struct sockaddr_in server_addr,
                  char *filename, char *mode);

// Handle PUT operation
void put_file_cmd(int sockfd, struct sockaddr_in server_addr,
                  char *filename, char *mode);

// Close connection
void disconnect(int sockfd);

// File transfer functions (defined elsewhere)
void send_file_wrq(int, struct sockaddr_in, socklen_t, char*, char*);
void receive_file(int, struct sockaddr_in, socklen_t, char*, char*);


typedef struct
{
    uint16_t opcode;

    union
    {
        struct
        {
            char filename[100];
            char mode[20];
        } request;

        struct
        {
            uint16_t block_number;
            char data[DATA_SIZE];
        } data_packet;

        struct
        {
            uint16_t block_number;
        } ack_packet;

        struct
        {
            uint16_t error_code;
            char error_msg[100];
        } error_packet;

    } body;

} tftp_packet;

#endif