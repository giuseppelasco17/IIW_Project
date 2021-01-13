#ifndef FILE_TRANSFER_H
#define FILE_TRANSFER_H

#include "structure.h"
#include "SRprotocol.h"

/*it is used by the thread that handles the reception of the ack.
 * In particular, it repeatedly calls the receiveSRack function contained in SRprotocol.*/
void* receive_ack(void*);

/*it is used by the thread that handles the sending of packets.
 * In particular, it deals with the opening of the file to be sent, properly checking its existence, so as to
 * communicate it to the receiving process.
 * It also initializes the packets to be sent by passing them to the writeSR contained in SRprotocol.*/
void* write_transfer(void*);

/*it is used by the process that handles packet reception.
 * In particular, it deals with the creation of the folder in which the file will be saved if the file exists.
 * Call the receiveSR function repeatedly until there are packets to receive.*/
void read_transfer(struct protocolSR* , int);


#endif
