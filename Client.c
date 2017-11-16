#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include "packet.h"

int main() {

    int clientsocket;
    struct sockaddr_in serverAddress;
    struct data_packets data_packet;
    struct ack_packets ack_packet;
    struct reject_packets reject_packet;
    char buffer[1024];
    int segment_number = 1;
    int input;

    printf("\nPress 0 to send 5 correct packets\n");
    printf("\nPress 1 to send 1 correct packet and 4 wrong packets\nYour input = ");

    scanf("%d", &input);
    printf("%d\n", input);
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
        // Initialize data packet with  all 5 correct packets
        if(input == 0) {

            data_packet.start_packet_id = START_PACKET_ID;
            data_packet.client_id = 24;
            data_packet.packet_type = DATA_PACKET;
            data_packet.segment_no = segment_number;
            data_packet.length = 5;
            data_packet.payload.length = data_packet.length;
            memcpy(data_packet.payload.data, "Hello", data_packet.payload.length);
            data_packet.end_packet_id = END_PACKET_ID;

        } else if(input == 1) {

            // Initialize data packet with 1 correct packet and 4 error packets
            data_packet.start_packet_id = START_PACKET_ID;
            data_packet.client_id = 24;
            data_packet.packet_type = DATA_PACKET;
            if(packet_number == 5) {

                // Assign wrong sequence number for 5th packet
                data_packet.segment_no = segment_number + 10;
            } else if(packet_number == 4) {

                // Duplicate packet
                data_packet.segment_no = 3;
            } else {
                data_packet.segment_no = segment_number;
            }
            // Assign wrong length for 2nd packet
            data_packet.length = (packet_number == 2) ? 8 : 5;
            data_packet.payload.length = 5;
            memcpy(data_packet.payload.data, "Hello", data_packet.payload.length);
            data_packet.end_packet_id = END_PACKET_ID;
            // NULL end packet id for 3rd packet
            if(packet_number == 3) {
                data_packet.end_packet_id = '\0';
            }
        }

        int packet_length = buildDataPacket(data_packet, buffer);

        // Send data packet to server
        sendto(clientsocket, buffer, packet_length, 0, 
        (struct sockaddr*)&serverAddress, sizeof serverAddress);

        printf("Packet %d Sent: \n", packet_number);   
        
        // Receive ACK or REJECT packet from server
        struct sockaddr sender;
        socklen_t sendsize = sizeof(sender);
        memset(&sender, 0, sizeof(sender));
        bind(clientsocket, (struct sockaddr *) &sender, sendsize);

        int recvlen = recvfrom(clientsocket, buffer, sizeof(buffer), 0, &sender, &sendsize);
        
        if (recvlen >= 0) {
            // Message Received
            // Determine whether the received packet is ACK or REJECT
            unsigned short packet_type;
            memcpy(&packet_type, buffer + 3, 2);

            if(packet_type == ACK_PACKET) {

                int response = decodeAckPacket(buffer, &ack_packet);
                printf("ACK \n");
            } else if(packet_type == REJECT_PACKET) {

                int reject_code = decodeRejectPacket(buffer, &reject_packet);
                switch(reject_code) {
                    case 0xfff4: printf("REJECT_OUT_OF_SEQUENCE\n"); break;
                    case 0xfff5: printf("REJECT_LENGTH_MISMATCH\n"); break;
                    case 0xfff6: printf("REJECT_END_OF_PACKET_MISSING\n"); break;
                    case 0xfff7: printf("REJECT_DUPLICATE_PACKET\n"); break;
                }
            }

        }
        else {
            // Message Receive Timeout or other error
            printf("Time out error - %s\n", strerror(errno));
            int retry_counter = 0;

            while(retry_counter < 3) {

                printf("Trying to retransmit..... Attempt no. %d\n", retry_counter + 1);

                // Retransmitting packet
                sendto(clientsocket, buffer, packet_length, 0, (struct sockaddr*)&serverAddress, sizeof serverAddress);
                int received_len = recvfrom(clientsocket, buffer, sizeof(buffer), 0, &sender, &sendsize);
                if(received_len >= 0) {
                    printf("Successfully retransmitted Packet %d\n", packet_number);
                    break;
                } else {

                    // If there is still no response from server after the last try to retransmit
                    if(retry_counter == 2) {
                        printf("Server does not respond\n");
                    }
                    retry_counter++;
                }
            }
        }

        printf("\n\n");
        packet_number++;
        segment_number++;
    }

    return 0;
}