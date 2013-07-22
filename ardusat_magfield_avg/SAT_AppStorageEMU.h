#ifndef SAT_APP_STORAGEEMU_H
#define SAT_APP_STORAGEEMU_H

#include <inttypes.h>
#include <stdlib.h>

#include <nanosat_message.h>

class SAT_AppStorageEMU
{
  private:
    uint8_t nodeAddress_;
    int dataCount;

    /*
     * Takes a fixed size of data to be packed into a Nanosat message struct
     * then queued in I2C.
     *
     * @param data    A byte array of data.
     * @param offset  starting offset.
     * @param length  how many bytes to be copied.
     */
    void copyAndSend(char data[], unsigned int offset, unsigned int length);
  public:
    /*
     * Constructor
     */
    SAT_AppStorageEMU();

    /*
     * Simple way to enqueue data to be published to disk.
     *
     * @note        When formatting data, don't forget to add newline or
     *              carriage returns.
     * @param data  A formatted string that needs to be written to disk for
     *   retreival by ground station.
     */
    void send(char data[]);

};

#endif /* SAT_APP_STORAGEEMU_H */
