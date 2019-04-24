
#include <stdlib.h>
#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include "rfmodule.h"
#include "cbuf.h"
#include "graph.h"

cbuf_t RxMsgQueue;
cbuf_t TxMsgQueue;

char isTransmitting = 0;    // set when message is sent to rf module, unset when modue interrupts with tx complete
char lastTxFailed = 0;      // last transmission failed indicator
char lastTxChBusy = 0;      // last transmission failed due to busy channel indicator

int rfAddr = 2;             // this device's network id
int nextMemberID = 0;       // used by coordinator to assign member IDs
char isEstablished = 0;     // is this device part of an established network?
int parentAddr = 0;

network_neighbor_t neighbors[MAXNETSIZE];

// Network address translation table,
// given destination address, stores where packet needs to be forwarded to
network_neighbor_t NAT[MAXNETSIZE];
int nextNeighbor = 0;
int nextAddressTranslation = 0;
int joinerRoute = 0;
int recolorGraph = 1;

//// Peripheral interface
int rfSend(int addr, int data)
{
    SPI1->CR2 |= 0xF00; // set 16b data
    SPI1->CR1 |= 0x40;

    // format data as per device spec
    int spiData = ((addr << 9) & 0x7E00) | (data & 0xFF) | (data == -1 ? 0 : 0x100);

    // send data to device
    while(!(SPI1->SR & SPI_SR_TXE));
    GPIOA->ODR &= ~(0x10);
    SPI1->DR = spiData;

    while(!(SPI1->SR & SPI_SR_RXNE));
    spiData = SPI1->DR;

    while((SPI1->SR & SPI_SR_FTLVL) || (SPI1->SR & SPI_SR_FRLVL));
    while((SPI1->SR & SPI_SR_BSY));
    SPI1->CR1 &= ~(0x40);
    GPIOA->ODR |= 0x10;

    return spiData;
}

int rfLongSend(int addr, int data)
{
    SPI1->CR2 = (SPI1->CR2 & ~(0xF00)) | 0xB00; // set 12b data
    SPI1->CR1 |= 0x40;

    int spiData = 0;


    // send addr
    while(!(SPI1->SR & SPI_SR_TXE));
    GPIOA->ODR &= ~(0x10);
    SPI1->DR = 0x800 | ((addr << 1) & 0x7FE) | (data == -1 ? 0 : 0x1);

    // send data
    while(!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = data;

    while(!(SPI1->SR & SPI_SR_RXNE));
    spiData = SPI1->DR;

    while(!(SPI1->SR & SPI_SR_RXNE));
    spiData = SPI1->DR;

    while((SPI1->SR & SPI_SR_FTLVL));
    while((SPI1->SR & SPI_SR_BSY));
    SPI1->CR1 &= ~(0x40);
    GPIOA->ODR |= 0x10;

    return spiData;
}


//// Device init
void rfInit()
{
    // follow init procedure as per device manual
    rfSend(0x2A, 0x07);
    rfSend(0x18, 0x98);
    rfSend(0x2E, 0x95);
    rfLongSend(0x200, 0x03);
    rfLongSend(0x201, 0x01);
    rfLongSend(0x202, 0x80);
    rfLongSend(0x206, 0x90);
    rfLongSend(0x207, 0x80);
    rfLongSend(0x208, 0x10);
    rfLongSend(0x220, 0x21);
    rfSend(0x3A, 0x80);
    rfSend(0x3F, 0x60);
    rfSend(0x3E, 0x40);
    rfSend(0x32, 0xF6);         // interrupt on normal tx/rx
    rfLongSend(0x200, 0x63);    // ch 17
    rfLongSend(0x203, 0x00);    // set transmit power (max)
    rfSend(0x36, 0x04);
    rfSend(0x36, 0x00);
    micro_wait(200);

    // non-standard init
    rfSend(0x01, PANID & 0xFF);     // set PAN id
    rfSend(0x02, PANID >> 8);
    setDeviceShortAddr(NID_INITIAL);
}

void rfJoinNetwork()
{
    // change state to joining
    // try and join existing network
    while(1)
    {
        // wait for any other joiners to finish joining
        while(rfPingAddr(NID_JOINING))
        {
            micro_wait(100000);
        }

        // try to become the joiner
        setDeviceShortAddr(NID_JOINING);

        // check if anyone else also tried to become the joiner
        if(rfPingAddr(NID_JOINING))
        {
            // if yes -> return to anonymous pool and wait
            setDeviceShortAddr(NID_INITIAL);
            micro_wait(10000 * ((rand() % 10) + 1));
        }
        else
        {
            break;  // no -> OK OK OK
        }
    }

    // get id from coordinator
    while(rfAddr == NID_JOINING)
    {
        // request response from any networked devices
        nextNeighbor = 0;

        // ping everyone to figure out who is closest to you
        // ping using BROADCAST address
        uint8_t msg = RFMSG_PINGNETWORK;
        rfSendMsg(NID_BROADCAST, &msg, 1);
        while(isTransmitting);

        // wait for responses
        // we wait for all nodes in the network to respond
        // to the broadcast
        int timeout = 0;
        while(timeout < NETWORK_TIMEOUT)
        {
            micro_wait(100);
            rfProcessRxQueue();
            timeout++;
        }

        // processRxQueue updates nextNeighbours
        // if there are none, no devices exist in the
        // network. If there are no other devices? -> promote yourself
        if(nextNeighbor == 0)
        {
            setDeviceShortAddr(NID_COORDINATOR);
            isEstablished = 1;

            // debug
            GPIOC->ODR |= (1<<9);
        }

        // otherwise, choose a parent node and request a network ID
        else
        {
            // select neighbor with highest RSSI as parent
            int maxPowerNeighbor = 0;
            for(int cx = 1; cx < nextNeighbor; cx++)
            {
                if(neighbors[cx].RSSI > neighbors[maxPowerNeighbor].RSSI)
                {
                    maxPowerNeighbor = cx;
                }
            }

            parentAddr = neighbors[maxPowerNeighbor].addr;

            // send association request
            uint8_t msg2[2] = {RFMSG_JOIN, parentAddr};
            rfSendMsg(parentAddr, msg2, 2);
            while(isTransmitting);

            // wait for responses
            int timeout = 0;
            while(timeout < NETWORK_TIMEOUT && rfAddr == NID_JOINING)
            {
                micro_wait(100);
                rfProcessRxQueue();
                timeout++;
            }
        }
    }

    return;
}


//// Device + low-level network functionality
int rfPingAddr(int dsAddr)
{
    uint8_t msg = RFMSG_IGNORE;
    rfSendMsg(dsAddr, &msg, 1);
    while(isTransmitting);
    return (lastTxFailed) ? 0 : 1;
}

void setDeviceShortAddr(int addr)
{
    rfAddr = addr;
    rfSend(0x03, rfAddr & 0xFF);    // set short addr
    rfSend(0x04, rfAddr >> 8);
}

void rfSendTestMsg()
{
    uint8_t data[16];

    for(int cx = 0; cx < 16; cx++)
    {
        data[cx] = cx+1;
    }

//  data[0] = 2;

    //rfSendMsg(NID_COORDINATOR, data, 16);

    rfQueueTxMsg(NID_BROADCAST, RFMSG_BLINK, data, 16);

//  uint8_t msg = RFMSG_REQID;
//  rfSendMsg(0x03, &msg, 1);
    //while(isTransmitting);

    //rfSendMsg(0x03, data, 16);

    //assignMemberID();
}

void rfSendMsg(int dsAddr, uint8_t* data, int datasize)
{   /* Populate TX FIFO and trigger TX (see device ds sec 3.12 - pg. 110) */

    isTransmitting = 1;

    rfLongSend(0x00, 9);            // Header Length (ignored?)
    rfLongSend(0x01, 9 + datasize); // Frame Length


    // MAC HEADER (see IEEE spec sec 5.2.1 - pg. 77)
    rfLongSend(0x02, 0x61); // Data frame, ack request, PAN compress
    rfLongSend(0x03, 0x98); // short addr mode, frame version 1
    rfLongSend(0x04, 0x00); // sequence 0

    rfLongSend(0x05, PANID & 0xFF);     // destination PAN
    rfLongSend(0x06, PANID >> 8);
    rfLongSend(0x07, dsAddr & 0xFF);    // destination short addr
    rfLongSend(0x08, dsAddr >> 8);
    rfLongSend(0x09, rfAddr & 0xFF);
    rfLongSend(0x0A, rfAddr >> 8);


    // DATA PAYLOAD
    for(int cx = 0; cx < datasize; cx++)
    {
        rfLongSend(0x0B + cx, data[cx]);
    }

    uint8_t msg[64];
    for(int cx = 0; cx < 64; cx++)
    {
        msg[cx] = rfLongSend(cx, -1);
    }


    // Trigger TX Normal w/ ACK
    rfSend(0x1B, 0x05);

}


//// TX/RX queueing
void rfHandleTxRxInterrupt()
{
    int intStat = rfSend(0x31, -1);

    if(intStat & INTSTAT_RXIF)
    {
        rfHandleRxInterrupt();
    }

    if(intStat & INTSTAT_TXNIF)
    {
        int txStat = rfSend(0x24, -1);
        lastTxChBusy = (txStat & TXSTAT_CCAFAIL) ? 1 : 0;
        lastTxFailed = (txStat & TXSTAT_TXNSTAT) ? 1 : 0;
        isTransmitting = 0;
    }
}

void rfHandleRxInterrupt()
{
    int msgType = rfLongSend(0x30A, -1);

    // if message is of type ignore then don't even bother reading it
    if(msgType != RFMSG_IGNORE)
    {
        // DEBUG only
        uint8_t msg[64];
        for(int cx = 0; cx < 64; cx++)
        {
            msg[cx] = rfLongSend(0x300 + cx, -1);
        }

        // read frame header from device (see device ds sec 3.11 - pg. 107)
        int frameLength = rfLongSend(0x300, -1);

        rfrxmsg_t *rfMsg = malloc(sizeof(rfrxmsg_t));

        rfMsg->frC = rfLongSend(0x301, -1) << 8 | rfLongSend(0x302, -1);
        rfMsg->seq = rfLongSend(0x303, -1);
        rfMsg->dPAN = rfLongSend(0x305, -1) << 8 | rfLongSend(0x304, -1);
        rfMsg->dAddr = rfLongSend(0x307, -1) << 8 | rfLongSend(0x306, -1);
        rfMsg->sAddr = rfLongSend(0x309, -1) << 8 | rfLongSend(0x308, -1);
        rfMsg->msgType = msgType;
        rfMsg->LQI = rfLongSend(0x300 + frameLength + 1, -1);
        rfMsg->RSSI = rfLongSend(0x300 + frameLength + 2, -1);

        // read payload
        if(frameLength - FRAMEOVERHEAD > 0)
        {
            uint8_t *payload = malloc(frameLength - FRAMEOVERHEAD);

            rfMsg->dataLength = frameLength - FRAMEOVERHEAD;
            rfMsg->data = payload;

            for(int cx = 0; cx < frameLength - FRAMEOVERHEAD; cx++)
            {
                payload[cx] = rfLongSend(0x300 + cx + 11, -1);
            }
        }
        else
        {
            rfMsg->dataLength = 0;
        }

        // Put message in rx queue to process in main task
        cbuf_insert(&RxMsgQueue, (uint32_t)rfMsg);
    }
}

void rfProcessRxQueue()
{
    while(!cbuf_empty(&RxMsgQueue))
    {
        rfrxmsg_t *rfMsg = (rfrxmsg_t*)cbuf_dequeue(&RxMsgQueue);

        if(rfMsg->msgType == RFMSG_BLINK)
        {
            GPIOC->ODR |= (1<<8);
            micro_wait(500000);
            GPIOC->ODR &= ~(1<<8);
        }

        else if(rfMsg->msgType == RFMSG_PINGNETWORK && isEstablished)
        {
            uint8_t data = rfMsg->RSSI;
            rfQueueTxMsg(rfMsg->sAddr, RFMSG_NETWORKRESPONSE, &data, 1);
        }

        else if(rfMsg->msgType == RFMSG_JOIN)
        {
            // if this device is the coordinator, generate ID
            if(rfAddr == NID_COORDINATOR)
            {
                uint8_t id = NID_MEMBERS + nextMemberID++;

                // Build the NAT table as we build the network graph
                if(nextAddressTranslation < MAXNETSIZE)
                {
                    NAT[nextAddressTranslation].addr = id;
                    NAT[nextAddressTranslation].routeAddr = joinerRoute;
                    nextAddressTranslation++;
                }

                rfQueueTxMsg(rfMsg->sAddr, RFMSG_ASSIGNID, &id, 1);

                // Add the vertex with id
                addVertex(id);

                // Add a link from the parent to the new leaf
                // Weight of the link is RSSI
                addEdge(rfMsg->data[1], id, rfMsg->RSSI);
                recolorGraph = 1;
            }

            // if someone else is already holding a join route, send back error
            else if(joinerRoute != 0)
            {
                rfQueueTxMsg(rfMsg->sAddr, RFMSG_ROUTEFAILED, 0, 0);
            }

            // otherwise, forward up chain
            else
            {
                // preserve join route for backtrace
                joinerRoute = rfMsg->sAddr;

                uint8_t data = rfMsg->data[0];
                rfQueueTxMsg(parentAddr, RFMSG_JOIN, &data, 1);
            }
        }

        else if(rfMsg->msgType == RFMSG_ASSIGNID)
        {
            // forward message to joiner
            if(isEstablished && joinerRoute != 0)
            {
                uint8_t id = rfMsg->data[0];
                rfQueueTxMsg(joinerRoute, RFMSG_ASSIGNID, &id, 1);

                if(nextAddressTranslation < MAXNETSIZE)
                {
                    NAT[nextAddressTranslation].addr = id;
                    NAT[nextAddressTranslation].routeAddr = joinerRoute;
                    nextAddressTranslation++;
                }

                joinerRoute = 0;
            }

            // we're the joiner, take the ID
            else if(!isEstablished)
            {
                setDeviceShortAddr(rfMsg->data[0]);

                neighbors[0].addr = parentAddr;
                nextNeighbor = 1;

                isEstablished = 1;


                // debug only
                GPIOC->ODR |= (1<<8);
                micro_wait(100000);
                GPIOC->ODR &= ~(1<<8);
            }
        }

        else if(rfMsg->msgType == RFMSG_NETWORKRESPONSE && !isEstablished && nextNeighbor < MAXNETSIZE)
        {
            neighbors[nextNeighbor].addr = rfMsg->sAddr;
            neighbors[nextNeighbor].RSSI = rfMsg->RSSI;
            nextNeighbor++;
        }

        else if(rfMsg->msgType == RFMSG_GENERIC)
        {
            // cast data as protocol packet
            pPacket* packet = (pPacket*) rfMsg->data;
            // if we are the intended destination
            if(packet->pDAddr == rfAddr) {
                handleGenericPacket(packet);
            }

            // forward message to dest
            else
            {
                // hopId the address to forward the packet to
                int hopId = rfAddr;
                for(int i = 0; i < MAXNETSIZE; i++) {
                    if(NAT[i].addr == packet->pDAddr) {
                        hopId = NAT[i].routeAddr;
                    }
                }

                rfQueueTxMsg(hopId, RFMSG_GENERIC, &(rfMsg->data), rfMsg->dataLength);
            }
        }

        if(rfMsg->dataLength > 0)
        {
            free(rfMsg->data);
        }

        free(rfMsg);
    }
}

void rfQueueTxMsg(int dsAddr, int msgType, uint8_t* body, int bodyLength)
{
    rftxmsg_t *txMsg = malloc(sizeof(rftxmsg_t));
    uint8_t *data = malloc(1 + bodyLength);

    data[0] = (uint8_t) msgType;
    for(int cx = 0; cx < bodyLength; cx++)
    {
        data[cx+1] = body[cx];
    }

    txMsg->dataLength = 1 + bodyLength;
    txMsg->data = data;
    txMsg->dAddr = dsAddr;

    cbuf_insert(&TxMsgQueue, (uint32_t)txMsg);
}

void rfProcessTxQueue()
{
    if(isTransmitting == 0 && !cbuf_empty(&TxMsgQueue))
    {
        rftxmsg_t *txMsg = (rftxmsg_t*)cbuf_dequeue(&TxMsgQueue);
        rfSendMsg(txMsg->dAddr, txMsg->data, txMsg->dataLength);

        free(txMsg->data);
        free(txMsg);
    }
}

void rfSendPacket(pPacket* packet) {
    int hopId = rfAddr;
    for(int i = 0; i < MAXNETSIZE; i++) {
        if(NAT[i].addr == packet->pDAddr) {
            hopId = NAT[i].routeAddr;
        }
    }

    rfQueueTxMsg(hopId, RFMSG_GENERIC, &(packet->pData), packet->pDataLength);
}

int isCoordinator() {
    if(rfAddr == NID_COORDINATOR)
        return 1;
    return 0;
}

int shouldRecolorGraph() {
    return recolorGraph;
}

void graphRecolored() {
    recolorGraph = 0;
}
