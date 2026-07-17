# TFTP Client-Server

## Overview

This project implements a simplified version of the **Trivial File Transfer Protocol (TFTP)** in **C** using **UDP sockets**. The application consists of a client and a server that communicate using the TFTP protocol to transfer files reliably over UDP.

The project supports both **Read Request (RRQ)** and **Write Request (WRQ)** operations while implementing the fundamental mechanisms of the protocol, including acknowledgments, block numbering, timeout handling, retransmissions, and transfer modes.

This project demonstrates practical implementation of networking concepts, socket programming, protocol design, and file transfer mechanisms.

---

## Features

- TFTP Client-Server architecture
- UDP socket communication
- Read Request (RRQ)
- Write Request (WRQ)
- File download from server
- File upload to server
- Octet transfer mode
- Netascii transfer mode
- Block-based data transfer
- ACK packet handling
- Timeout detection
- Retransmission mechanism
- Error packet handling
- Interactive command-line interface

---

## Technologies Used

- C Programming
- UDP Sockets
- POSIX Socket API
- Linux System Calls
- File Handling
- Network Programming

---

## Project Structure

```
tftp-client-server/
│
├── Client/
│   ├── client.c
│   ├── tftp_file.c
│   ├── tftp.h
│
├── Server/
│   ├── server.c
│   ├── tftp_file.c
│   ├── tftp.h
│
└── README.md
```

---

## Supported Commands

```
connect <server_ip>

get <filename>

put <filename>

mode octet

mode netascii

quit
```

---

## Supported Transfer Modes

### Octet Mode

Transfers files as raw binary data.

Suitable for:

- Images
- PDFs
- Executables
- Binary files

---

### Netascii Mode

Transfers text files by converting line endings according to the TFTP specification.

Suitable for:

- Text files
- Source code
- Configuration files

---

## TFTP Workflow

### Read Request (RRQ)

Client
→ Sends RRQ

Server
→ Sends DATA Block 1

Client
→ Sends ACK 1

Server
→ Sends DATA Block 2

Client
→ Sends ACK 2

...

Transfer completes when the final DATA packet contains less than 512 bytes.

---

### Write Request (WRQ)

Client
→ Sends WRQ

Server
→ Sends ACK Block 0

Client
→ Sends DATA Block 1

Server
→ Sends ACK 1

Client
→ Sends DATA Block 2

Server
→ Sends ACK 2

...

Transfer completes after the final block.

---

## Reliability Features

The project implements reliable file transfer over UDP using:

- Block numbering
- ACK packets
- Receive timeout
- Packet retransmission
- Duplicate packet handling
- Error packet detection

---

## Compilation

### Client

```bash
gcc *.c -o client
```

### Server

```bash
gcc *.c -o server
```

---

## Running

Start the server:

```bash
./server
```

Run the client:

```bash
./client
```

---

## Example Usage

Connect to server

```text
connect 127.0.0.1
```

Download a file

```text
get sample.txt
```

Upload a file

```text
put report.txt
```

Change transfer mode

```text
mode netascii
```

```text
mode octet
```

Exit

```text
quit
```

---

## Error Handling

The application handles various error conditions, including:

- Invalid commands
- Invalid IP addresses
- File not found
- Socket timeout
- Transfer timeout
- Incorrect acknowledgments
- Error packets
- Connection failures

---

## Limitations

- Single client handled per transfer.
- Supports standard TFTP packet size (512-byte data blocks).

---

## Future Improvements

- Concurrent client support.
- Directory browsing.
- Progress indicator.
- Resume interrupted transfers.
- Improved logging.
- IPv6 support.

---

## Learning Outcomes

This project helped reinforce concepts such as:

- UDP socket programming
- Network protocol implementation
- Client-server communication
- Reliable data transfer over UDP
- Timeout and retransmission mechanisms
- File handling in C
- POSIX networking APIs
- Modular software design
