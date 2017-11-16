#include <string.h>
#include<stdio.h>

#define START_PACKET_ID 0xFFFF
#define END_PACKET_ID 0xFFFF

#define DATA_PACKET 0xFFF1
#define ACK_PACKET 0xFFF2
#define REJECT_PACKET 0xFFF3

#define REJECT_OUT_OF_SEQUENCE 0xFFF4
#define REJECT_LENGTH_MISMATCH 0xFFF5
#define REJECT_END_OF_PACKET_MISSING 0xFFF6
#define REJECT_DUPLICATE_PACKET 0xFFF7

struct _payload {
    char data[255];
    char length;
};

struct data_packets {
    unsigned short start_packet_id;
    char client_id;
    unsigned short packet_type;
    char segment_no;
    char length;
    struct _payload payload;
    unsigned short end_packet_id;
};

struct ack_packets {
    unsigned short start_packet_id;
    char client_id;
    unsigned short packet_type;
    char received_segment_no;
    unsigned short end_packet_id;
};

struct reject_packets {
    unsigned short start_packet_id;
    char client_id;
    unsigned short packet_type;
    unsigned short reject_sub_code;
    char received_segment_no;
    unsigned short end_packet_id;
};

/**
 * This function builds the buffer to be sent using the data in the data_packet 
 * Input params are data_packet and buffer
 * Output is the length of the data in the buffer
 **/
int buildDataPacket(struct data_packets data_packet, char* buffer) {

    int buffer_length = 0;

    memcpy(&buffer[buffer_length], &data_packet.start_packet_id, 2);
    buffer_length += 2;

    buffer[buffer_length] = data_packet.client_id;
    buffer_length += 1;

    memcpy(&buffer[buffer_length], &data_packet.packet_type, 2);
    buffer_length += 2;

    buffer[buffer_length] = data_packet.segment_no;
    buffer_length += 1;

    buffer[buffer_length] = data_packet.length;
    buffer_length += 1;

    memcpy(&buffer[buffer_length], &data_packet.payload.data, data_packet.payload.length);
    buffer_length += data_packet.payload.length;

    memcpy(&buffer[buffer_length], &data_packet.end_packet_id, 2);
    buffer_length += 2;

    buffer[buffer_length + 1] = '\0';

    return buffer_length;
}

/**
 * This function builds the buffer to be sent using the data in the ack_packet 
 * Input params are ack_packet and buffer
 * Output is the length of the data in the buffer
 **/
int buildAckPacket(struct ack_packets ack_packet, char* buffer) {

    int buffer_length = 0;

    memcpy(&buffer[buffer_length], &ack_packet.start_packet_id, 2);
    buffer_length += 2;

    buffer[buffer_length] = ack_packet.client_id;
    buffer_length += 1;

    memcpy(&buffer[buffer_length], &ack_packet.packet_type, 2);
    buffer_length += 2;

    buffer[buffer_length] = ack_packet.received_segment_no;
    buffer_length += 1;

    memcpy(&buffer[buffer_length], &ack_packet.end_packet_id, 2);
    buffer_length += 2;

    buffer[buffer_length + 1] = '\0';

    return buffer_length;
}

/**
 * This function builds the buffer to be sent using the data in the reject_packet 
 * Input params are reject_packet and buffer
 * Output is the length of the data in the buffer
 **/
int buildRejectPacket(struct reject_packets reject_packet, char* buffer) {

    int buffer_length = 0;

    memcpy(&buffer[buffer_length], &reject_packet.start_packet_id, 2);
    buffer_length += 2;

    buffer[buffer_length] = reject_packet.client_id;
    buffer_length += 1;

    memcpy(&buffer[buffer_length], &reject_packet.packet_type, 2);
    buffer_length += 2;

    memcpy(&buffer[buffer_length], &reject_packet.reject_sub_code, 2);
    buffer_length += 2;

    buffer[buffer_length] = reject_packet.received_segment_no;
    buffer_length += 1;

    memcpy(&buffer[buffer_length], &reject_packet.end_packet_id, 2);
    buffer_length += 2;

    buffer[buffer_length + 1] = '\0';

    return buffer_length;
}

/**
 * This method decodes data in the buffer into the data packet
 * Input params are buffer and data_packet
 * Output is an int - 0 or END_PACKET_MISSING flag
 **/
int decodeDataPacket(char* buffer, struct data_packets* data_packet) {

    int buffer_length = 0;

    memcpy(&(data_packet->start_packet_id), buffer + buffer_length, 2);
    buffer_length += 2;

    data_packet->client_id = buffer[buffer_length];
    buffer_length += 1;
    
    memcpy(&(data_packet->packet_type), buffer + buffer_length, 2);
    buffer_length += 2;

    data_packet->segment_no = buffer[buffer_length];
    buffer_length += 1;

    data_packet->length = buffer[buffer_length];
    buffer_length += 1;

    int payload_length = 0;
    while(payload_length < 255) {
        unsigned short data;
        memcpy(&data, buffer + buffer_length, 2);
        if(data ==  END_PACKET_ID) {
            break;
        } else {
            data_packet->payload.data[payload_length] = buffer[buffer_length];
            buffer_length++;
            payload_length++;
        }
    }
    data_packet->payload.length = payload_length;

    unsigned short end_packet;
    memcpy(&end_packet, buffer + buffer_length, 2);
    if(end_packet == END_PACKET_ID) {
        memcpy(&(data_packet->end_packet_id), &end_packet, 2);
        return 0;
    } else {
        return REJECT_END_OF_PACKET_MISSING;
    }
}

/**
 * This method decodes data in the buffer into the ack packet
 * Input params are buffer and ack_packet
 * Output is an int 
 **/
int decodeAckPacket(char* buffer, struct ack_packets* ack_packet) {

    int buffer_length = 0;

    memcpy(&(ack_packet->start_packet_id), buffer + buffer_length, 2);
    buffer_length += 2;

    ack_packet->client_id = buffer[buffer_length];
    buffer_length += 1;
    
    memcpy(&(ack_packet->packet_type), buffer + buffer_length, 2);
    buffer_length += 2;

    ack_packet->received_segment_no = buffer[buffer_length];
    buffer_length += 1;

    memcpy(&(ack_packet->end_packet_id), buffer + buffer_length, 2);

    return 0;
}

/**
 * This method decodes data in the buffer into the reject packet
 * Input params are buffer and reject_packet
 * Output is an int - corresponding reject sub code
 **/
int decodeRejectPacket(char* buffer, struct reject_packets* reject_packet) {

    int buffer_length = 0;

    memcpy(&(reject_packet->start_packet_id), buffer + buffer_length, 2);
    buffer_length += 2;

    reject_packet->client_id = buffer[buffer_length];
    buffer_length += 1;
    
    memcpy(&(reject_packet->packet_type), buffer + buffer_length, 2);
    buffer_length += 2;

    memcpy(&(reject_packet->reject_sub_code), buffer + buffer_length, 2);
    buffer_length += 2;

    reject_packet->received_segment_no = buffer[buffer_length];
    buffer_length += 1;

    memcpy(&(reject_packet->end_packet_id), buffer + buffer_length, 2);

    return reject_packet->reject_sub_code;
}

/**
 * This function initializes reject_packet with values from data_packet
 * Input params are data_packet, reject_packet and reject_sub_code
 **/
void initializeRejectPacket(struct data_packets data_packet, struct reject_packets* reject_packet, int reject_sub_code) {
    
    reject_packet->start_packet_id = START_PACKET_ID;
    reject_packet->client_id = 24;
    reject_packet->packet_type = REJECT_PACKET;
    reject_packet->reject_sub_code = REJECT_END_OF_PACKET_MISSING;
    reject_packet->received_segment_no = data_packet.segment_no;
    reject_packet->end_packet_id = END_PACKET_ID;
    switch(reject_sub_code) {
        case 0xfff4: reject_packet->reject_sub_code = REJECT_OUT_OF_SEQUENCE; break;
        case 0xfff5: reject_packet->reject_sub_code = REJECT_LENGTH_MISMATCH; break;
        case 0xfff6: reject_packet->reject_sub_code = REJECT_END_OF_PACKET_MISSING; break;
        case 0xfff7: reject_packet->reject_sub_code = REJECT_DUPLICATE_PACKET; break;
    }

    printf("REJECT CODE - %x\n", reject_packet->reject_sub_code);
}
