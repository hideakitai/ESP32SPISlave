#include <ESP32SPISlave.h>

ESP32SPISlave slave;

static const uint32_t BUFFER_SIZE = 32;
uint8_t spi_slave_tx_buf[BUFFER_SIZE];
uint8_t spi_slave_rx_buf[BUFFER_SIZE];

void setup() {
    Serial.begin(115200);
    delay(2000);

    slave.setDataMode(SPI_MODE3);
    slave.setQueueSize(1);  // transaction queue size

    // begin() after setting
    // HSPI = CS: 15, CLK: 14, MOSI: 13, MISO: 12
    slave.begin();  // default SPI is HSPI
}

void loop() {
    // if there is no transaction in queue, add transaction
    if (slave.remained() == 0)
        slave.queue(spi_slave_rx_buf, spi_slave_tx_buf, BUFFER_SIZE);

    // if transaction has completed from master,
    // available() returns size of results of transaction,
    // and buffer is automatically updated

    while (slave.available()) {
        // show received data
        for (size_t i = 0; i < BUFFER_SIZE; ++i)
            printf("%d ", spi_slave_rx_buf[i]);
        printf("\n");

        // copy received bytes to tx buffer
        memcpy(spi_slave_tx_buf, spi_slave_rx_buf, BUFFER_SIZE);

        slave.pop();
    }
}
