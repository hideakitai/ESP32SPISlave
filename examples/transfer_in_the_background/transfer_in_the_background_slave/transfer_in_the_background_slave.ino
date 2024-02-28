#include <ESP32SPISlave.h>
#include "helper.h"

ESP32SPISlave slave;

static constexpr size_t BUFFER_SIZE = 8;
static constexpr size_t QUEUE_SIZE = 1;
uint8_t tx_buf[BUFFER_SIZE] {1, 2, 3, 4, 5, 6, 7, 8};
uint8_t rx_buf[BUFFER_SIZE] {0, 0, 0, 0, 0, 0, 0, 0};

void setup()
{
    Serial.begin(115200);

    delay(2000);

    slave.setDataMode(SPI_MODE0);   // default: SPI_MODE0
    slave.setQueueSize(QUEUE_SIZE); // default: 1, requres 2 in this example

    // begin() after setting
    slave.begin();  // default: HSPI (please refer README for pin assignments)

    Serial.println("start spi slave");
}

void loop()
{
    // if no transaction is in flight and all results are handled, queue new transactions
    if (slave.hasTransactionsCompletedAndAllResultsHandled()) {
        // initialize tx/rx buffers
        Serial.println("initialize tx/rx buffers");
        initializeBuffers(tx_buf, rx_buf, BUFFER_SIZE, 0);

        // queue transaction and trigger it right now
        Serial.println("execute transaction in the background");
        slave.queue(tx_buf, rx_buf, BUFFER_SIZE);
        slave.trigger();

        Serial.println("wait for the completion of the queued transactions...");
    }

    // you can do some other stuff here
    // NOTE: you can't touch dma_tx/rx_buf because it's in-flight in the background

    // if all transactions are completed and all results are ready, handle results
    if (slave.hasTransactionsCompletedAndAllResultsReady(QUEUE_SIZE)) {
        // process received data from slave
        Serial.println("all queued transactions completed. start verifying received data from slave");

        // get the oldeest transfer result
        size_t received_bytes = slave.numBytesReceived();

        // verify and dump difference with received data
        // NOTE: we need only 1st results (received_bytes[0])
        if (verifyAndDumpDifference("slave", tx_buf, BUFFER_SIZE, "master", rx_buf, received_bytes)) {
            Serial.println("successfully received expected data from master");
        } else {
            Serial.println("unexpected difference found between master/slave data");
        }
    }
}
