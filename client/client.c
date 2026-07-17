#include "tftp.h"

int main()
{
    // Socket descriptor
    int sockfd;

    // Server address structure
    struct sockaddr_in server_addr;

    // Command buffer
    char command[100];

    // Default transfer mode
    char mode[20] = "octet";

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // Set timeout for recv
    struct timeval tv = {TIMEOUT_SEC, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Default server config (localhost)
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Command loop
    while(1)
    {
        // Prompt
        printf("tftp> ");

        // Read command
        fgets(command, sizeof(command), stdin);

        // Remove newline
        command[strcspn(command, "\n")] = 0;

        // Process command
        process_command(sockfd, &server_addr, command, mode);
    }

    return 0;
}

void process_command(int sockfd, struct sockaddr_in *server_addr,char *command, char *mode)
{
    // Command and argument
    char cmd[20], arg[100];

    // Parse command
    int count = sscanf(command, "%s %s", cmd, arg);

    if(strcmp(cmd, "connect") == 0)
    {
        // Validate argument count
        if(count != 2)
        {
            printf("Usage: connect <ip>\n");
            return;
        }

        // IP validation variables
        int a,b,c,d;
        char extra;

        // Validate IP format and range
        if(sscanf(arg, "%d.%d.%d.%d%c", &a,&b,&c,&d,&extra) != 4 || a<0 || a>255 || b<0 || b>255 || c<0 || c>255 || d<0 || d>255)
        {
            printf("Invalid IP address\n");
            return;
        }

        // Connect to server
        connect_to_server(server_addr, arg);
    }

    else if(strcmp(cmd, "get") == 0)
    {
        // Validate argument count
        if(count != 2)
        {
            printf("Usage: get <filename>\n");
            return;
        }

        // Call GET handler
        get_file_cmd(sockfd, *server_addr, arg, mode);
    }

    else if(strcmp(cmd, "put") == 0)
    {
        // Validate argument count
        if(count != 2)
        {
            printf("Usage: put <filename>\n");
            return;
        }

        // Check file exists
        int fd = open(arg, O_RDONLY);
        if(fd < 0)
        {
            printf("File not found\n");
            return;
        }
        close(fd);

        // Call PUT handler
        put_file_cmd(sockfd, *server_addr, arg, mode);
    }

   
    else if(strcmp(cmd, "mode") == 0)
    {
        // Validate argument count
        if(count != 2)
        {
            printf("Usage: mode <octet/netascii>\n");
            return;
        }

        // Validate mode
        if(strcmp(arg, "octet") != 0 && strcmp(arg, "netascii") != 0)
        {
            printf("Invalid mode. Use octet/netascii\n");
            return;
        }

        // Set mode
        strcpy(mode, arg);

        // Confirm change
        printf("Mode set to %s\n", mode);
    }


    else if(strcmp(cmd, "quit") || strcmp(cmd,"bye")== 0)
    {
        // Exit program
        disconnect(sockfd);
    }

    else
    {
        // Unknown command
        printf("Invalid command\n");
    }
}

void connect_to_server(struct sockaddr_in *server_addr, char *ip)
{
    // Set family
    server_addr->sin_family = AF_INET;

    // Set port
    server_addr->sin_port = htons(PORT);

    // Set IP
    server_addr->sin_addr.s_addr = inet_addr(ip);

    // Confirmation
    printf("Connected to %s:%d\n", ip, PORT);
}

void get_file_cmd(int sockfd, struct sockaddr_in server_addr,
                  char *filename, char *mode)
{
    // Buffer for RRQ
    char buffer[BUFFER_SIZE];

    // Length tracker
    int len = 0;

    // RRQ opcode
    uint16_t op = htons(RRQ);

    // Copy opcode
    memcpy(buffer+len, &op, 2); len+=2;

    // Copy filename
    strcpy(buffer+len, filename);
    len += strlen(filename)+1;

    // Copy mode
    strcpy(buffer+len, mode);
    len += strlen(mode)+1;

    // Send RRQ
    sendto(sockfd, buffer, len, 0,(struct sockaddr*)&server_addr, sizeof(server_addr));

    // Receive file
    receive_file(sockfd, server_addr, sizeof(server_addr), filename, mode);
}

void put_file_cmd(int sockfd, struct sockaddr_in server_addr,char *filename, char *mode)
{
    // Check file exists
    int fd = open(filename, O_RDONLY);
    if(fd < 0)
    {
        perror("open");
        return;
    }
    close(fd);

    // Buffer for WRQ
    char buffer[BUFFER_SIZE];

    // Length tracker
    int len = 0;

    // WRQ opcode
    uint16_t op = htons(WRQ);

    // Copy opcode
    memcpy(buffer+len, &op, 2); len+=2;

    // Copy filename
    strcpy(buffer+len, filename);
    len += strlen(filename)+1;

    // Copy mode
    strcpy(buffer+len, mode);
    len += strlen(mode)+1;

    // Send WRQ
    sendto(sockfd, buffer, len, 0,(struct sockaddr*)&server_addr, sizeof(server_addr));

    // Send file after WRQ
    send_file_wrq(sockfd,server_addr,sizeof(server_addr),filename,mode);
}

void disconnect(int sockfd)
{
    // Close socket
    close(sockfd);

    // Exit program
    exit(0);
}
