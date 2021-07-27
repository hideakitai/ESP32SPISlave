#include <ESP32SPISlave.h>

ESP32SPISlave slave;

static constexpr uint32_t MAX_TRANSFER_SIZE {32};
static constexpr uint32_t BUFFER_SIZE {MAX_TRANSFER_SIZE* 256}; // 8192 bytes
uint8_t spi_slave_tx_buf[BUFFER_SIZE];
uint8_t spi_slave_rx_buf[BUFFER_SIZE];
uint32_t bytes_offset {0};

void set_buffer()
{
    for (uint32_t i = 0; i < BUFFER_SIZE; i++)
    {
        spi_slave_tx_buf[i] = (0xFF - i) & 0xFF;
    }
    memset(spi_slave_rx_buf, 0, BUFFER_SIZE);
}


void setup()
{
    Serial.begin(115200);

    set_buffer();

    delay(5000);

    slave.setDataMode(SPI_MODE3);
    slave.setQueueSize(1); // transaction queue size

    // begin() after setting
    // HSPI = CS: 15, CLK: 14, MOSI: 13, MISO: 12
    slave.begin(); // default SPI is HSPI
}

void loop()
{
    // if there is no transaction in queue, add transaction
    if (slave.remained() == 0)
        slave.queue(spi_slave_rx_buf + bytes_offset, spi_slave_tx_buf + bytes_offset, MAX_TRANSFER_SIZE);

    // if transaction has completed from master,
    // available() returns size of results of transaction,
    // and buffer is automatically updated

    while (slave.available())
    {
        // show received data
        for (size_t i = bytes_offset; i < bytes_offset + MAX_TRANSFER_SIZE; ++i)
            printf("%d ", spi_slave_rx_buf[i]);
        printf("\n");

        slave.pop();
        bytes_offset += MAX_TRANSFER_SIZE;

        if (bytes_offset >= BUFFER_SIZE) {
            printf("Finished all transaction. Return to first byte");
            bytes_offset = 0;
        }
    }
}