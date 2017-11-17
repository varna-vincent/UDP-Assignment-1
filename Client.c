#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include "packet.h"

void assignInputToDataPacket(struct data_packets* data_packet, char segment_number, char length, char payload_length, unsigned short end_packet_id);
void receivePacket(int client_socket, struct sockaddr_in serverAddress, char* req_buffer, int packet_length);

int main() {

    int clientsocket;
    struct sockaddr_in serverAddress;
    struct data_packets data_packet;
    struct ack_packets ack_packet;
    char buffer[1024];
    int segment_number = 1;
    int input;

    printf("\nPress 0 to send 5 correct packets\n");
    printf("\nPress 1 to send 1 correct packet and 4 wrong packets\nYour input = ");

    scanf("%d", &input);
    // Create client socket
    clientsocket = socket(AF_INET, SOCK_DGRAM, 0);

    // Initialize Server socket
    memset( &serverAddress, 0, sizeof(serverAddress) );
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(7891);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    // Set timer options on socket
    struct timeval timeout = {3, 0};
    if (setsockopt(clientsocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Error");
    }
    
    int packet_number = 1;
    while (packet_number <= 5) {

        if(input == 0) {
            // Initialize data packet with  all 5 correct packets
            assignInputToDataPacket(&data_packet, segment_number, 5, 5, END_PACKET_ID);
        } else if(input == 1) {

            switch (packet_number) {
                case 1:
                    // Initialize data packet with 1 correct packet and 4 error packets
                    assignInputToDataPacket(&data_packet, segment_number, 5, 5, END_PACKET_ID);
                    break;
                case 2:
                    // Assign wrong length for 2nd packet
                    assignInputToDataPacket(&data_packet, segment_number, 8, 5, END_PACKET_ID);
                    break;
                case 3:
                    // NULL end packet id for 3rd packet
                    assignInputToDataPacket(&data_packet, segment_number, 5, 5, '\0');
                    break;
                case 4:
                    // Duplicate packet
                    assignInputToDataPacket(&data_packet, segment_number - 1, 5, 5, END_PACKET_ID);
                    break;
                case 5:
                    // Assign wrong sequence number for 5th packet
                    assignInputToDataPacket(&data_packet, segment_number + 10, 5, 5, END_PACKET_ID);
                    break;
            }
        }

        int packet_length = buildDataPacket(data_packet, buffer);

        // Send data packet to server
        sendto(clientsocket, buffer, packet_length, 0, (struct sockaddr*)&serverAddress, sizeof serverAddress);

        printf("Packet %d Sent: \n", packet_number);   
        
        receivePacket(clientsocket, serverAddress, buffer, packet_length);

        printf("\n\n");
        packet_number++;
        segment_number++;
    }

    return 0;
}

void assignInputToDataPacket(struct data_packets* data_packet, char segment_number, char length, char payload_length, unsigned short end_packet_id) {

    data_packet->start_packet_id = START_PACKET_ID;
    data_packet->client_id = 24;
    data_packet->packet_type = DATA_PACKET;
    data_packet->segment_no = segment_number;
    data_packet->length = length;
    data_packet->payload.length = payload_length;
    memcpy(data_packet->payload.data, "Hello", data_packet->payload.length);
    data_packet->end_packet_id = end_packet_id;
}

void receivePacket(int client_socket, struct sockaddr_in serverAddress, char* req_buffer, int packet_length) {
    // Receive ACK or REJECT packet from server
    char ack_buffer[1024];
    struct sockaddr sender;
    struct reject_packets reject_packet;

    socklen_t sendsize = sizeof(sender);
    memset(&sender, 0, sizeof(sender));
    bind(client_socket, (struct sockaddr *) &sender, sendsize);

    int retry_counter = 0;

    while (retry_counter <= 3) {
        int recvlen = recvfrom(client_socket, ack_buffer, sizeof(ack_buffer), 0, &sender, &sendsize);
        
        if(recvlen >= 0) {
            // Message Received
            // Determine whether the received packet is ACK or REJECT
            unsigned short packet_type;
            memcpy(&packet_type, ack_buffer + 3, 2);

            if(packet_type == ACK_PACKET) {
                printf("ACK \n");
            } else if(packet_type == REJECT_PACKET) {

                int reject_code = decodeRejectPacket(ack_buffer, &reject_packet);
                printRejectCode(reject_code);
            }
            break;
        } else {
            // If there is still no response from server after the last try to retransmit
            if(retry_counter == 3) {
                printf("Server does not respond\n");
                break;
            }
            retry_counter++;
            printf("Trying to retransmit..... Attempt no. %d\n", retry_counter);
            sendto(client_socket, req_buffer, packet_length, 0, (struct sockaddr*)&serverAddress, sizeof serverAddress);
        }
    }
}