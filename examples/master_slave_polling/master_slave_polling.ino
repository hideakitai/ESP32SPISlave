#include <ESP32SPISlave.h>

static constexpr uint8_t VSPI_SS {SS};  // default: GPIO 5
SPIClass master(VSPI);
ESP32SPISlave slave;

static const uint32_t BUFFER_SIZE {8};
uint8_t spi_master_tx_buf[BUFFER_SIZE] {1, 2, 3, 4, 5, 6, 7, 8};
uint8_t spi_master_rx_buf[BUFFER_SIZE] {0};
uint8_t spi_slave_tx_buf[BUFFER_SIZE] {8, 7, 6, 5, 4, 3, 2, 1};
uint8_t spi_slave_rx_buf[BUFFER_SIZE] {0};

void setup() {
    Serial.begin(115200);
    delay(2000);

    // SPI Master
    // VSPI = CS: 5, CLK: 18, MOSI: 23, MISO: 19
    pinMode(VSPI_SS, OUTPUT);
    digitalWrite(VSPI_SS, HIGH);
    master.begin();

    // SPI Slave
    // HSPI = CS: 15, CLK: 14, MOSI: 13, MISO: 12 -> default
    // VSPI = CS:  5, CLK: 18, MOSI: 23, MISO: 19
    slave.setDataMode(SPI_MODE0);
    slave.begin(HSPI);

    // connect same name pins each other
    // CS - CS, CLK - CLK, MOSI - MOSI, MISO - MISO

    printf("Master Slave Polling Test Start\n");
}

void loop() {
    // just queue transaction
    // if transaction has completed from master, buffer is automatically updated
    slave.queue(spi_slave_rx_buf, spi_slave_tx_buf, BUFFER_SIZE);

    // start master transaction
    master.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    digitalWrite(VSPI_SS, LOW);
    master.transferBytes(spi_master_tx_buf, spi_master_rx_buf, BUFFER_SIZE);
    // Or you can transfer like this
    // for (size_t i = 0; i < BUFFER_SIZE; ++i)
    //     spi_master_rx_buf[i] = master.transfer(spi_master_tx_buf[i]);
    digitalWrite(VSPI_SS, HIGH);
    master.endTransaction();

    // if slave has received transaction data, available() returns size of received transactions
    while (slave.available()) {
        printf("master -> slave : ");
        for (size_t i = 0; i < slave.size(); ++i) {
            printf("%d ", spi_slave_rx_buf[i]);
        }
        printf("\n");

        printf("slave -> master : ");
        for (size_t i = 0; i < BUFFER_SIZE; ++i) {
            printf("%d ", spi_master_rx_buf[i]);
        }
        printf("\n");

        // clear received data
        memset(spi_master_rx_buf, 0, BUFFER_SIZE);
        memset(spi_slave_rx_buf, 0, BUFFER_SIZE);

        // you should pop the received packet
        slave.pop();
    }

    delay(2000);
}
