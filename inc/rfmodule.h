/*
 * rfmodule.h
 *
 *  Created on: Apr 22, 2019
 *      Author: G9NOCX23Lu
 */

#ifndef RFMODULE_H_
#define RFMODULE_H_

#define INTSTAT_RXIF 0x08       // bitmask for rx interrupt in INTSTAT (0x31)
#define INTSTAT_TXNIF 0x01      // bitmask for tx interrupt in INTSTAT (0x31)
#define TXSTAT_CCAFAIL 0x20     // bitmask for clear channel assessment status in TXSTAT (0x24)
#define TXSTAT_TXNSTAT 0x01     // bitmask for tx status in TXSTAT (0x24)

#define PANID 2134              // personal area network ID (static for this project)
#define FRAMEOVERHEAD 12        // number of non-data bytes included in message frame length
#define MAXNETSIZE 16

#define NID_INITIAL 0x04        // network id - on startup
#define NID_JOINING 0x05        // network id - attempting to join network
#define NID_COORDINATOR 0x20    // network id of network coordinator
#define NID_MEMBERS 0x30        // first network id of network members
#define NID_BROADCAST 0xFFFF    // broadcast network id (IEEE spec)

#define RFMSG_IGNORE 0          // message ID - no-op
#define RFMSG_BLINK 1           // message ID - blink LED (test/debug)
#define RFMSG_REQID 2           // message ID - request network ID from coordinator
#define RFMSG_DESTROY_NETWORK 3 // message ID - broadcast unregister all
#define RFMSG_ASSIGNID 4        // message ID - assigned ID from coordinator
#define RFMSG_PINGNETWORK 5     // message ID - check if a network exists in range
#define RFMSG_NETWORKRESPONSE 6 // message ID - acknowledge network ping
#define RFMSG_JOIN 7
#define RFMSG_ROUTEFAILED 8
#define RFMSG_GENERIC 9        // message ID - generic packet, data handeled by the upper layer


#define PACKET_SETCOLOR 1

#define NETWORK_TIMEOUT 500

typedef struct {
    uint8_t pType;
    uint8_t* pData;
    int pDataLength;
    int pDAddr;
} pPacket; // protocol packet

typedef struct rfrxmsg {
    uint8_t* data;
    int dataLength;
    int msgType;
    int frC;
    int seq;
    int dPAN;
    int dAddr;
    int sAddr;
    int LQI;
    int RSSI;
} rfrxmsg_t;

typedef struct rftxmsg {
    uint8_t* data;
    int dataLength;
    int dAddr;
} rftxmsg_t;

typedef struct network_neighbor {
    int addr;
    int routeAddr;
    int RSSI;
} network_neighbor_t;

int rfSend(int, int);
int rfLongSend(int, int);

void rfInit();
void rfJoinNetwork();

int rfPingAddr(int);
void setDeviceShortAddr(int);
void rfSendTestMsg();
void rfSendMsg(int, uint8_t*, int);

void rfHandleTxRxInterrupt();
void rfHandleRxInterrupt();
void rfProcessRxQueue();
void rfQueueTxMsg(int, int, uint8_t*, int);
void rfProcessTxQueue();
void rfSendPacket(pPacket* packet);
int isCoordinator();
int shouldRecolorGraph();
void graphRecolored();
#endif /* RFMODULE_H_ */
