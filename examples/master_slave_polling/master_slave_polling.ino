#include <ESP32DMASPIMaster.h>
#include <ESP32SPISlave.h>

static constexpr uint8_t VSPI_SS {SS};  // default: GPIO 5
SPIClass master(VSPI);
ESP32SPISlave slave;

static const uint32_t BUFFER_SIZE {32};
uint8_t spi_master_tx_buf[BUFFER_SIZE];
uint8_t spi_master_rx_buf[BUFFER_SIZE];
uint8_t spi_slave_tx_buf[BUFFER_SIZE];
uint8_t spi_slave_rx_buf[BUFFER_SIZE];

void pirnt_array_range(const char* title, uint8_t* buf, uint32_t start, uint32_t len) {
    if (len == 1)
        printf("%s [%d]: ", title, start);
    else
        printf("%s [%d-%d]: ", title, start, start + len - 1);

    for (uint32_t i = 0; i < len; i++) {
        printf("%02X ", buf[start + i]);
    }
    printf("\n");
}

void print_if_not_matched(const char* a_title, uint8_t* a_buf, const char* b_title, uint8_t* b_buf, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        uint32_t j = 1;

        if (a_buf[i] == b_buf[i]) continue;

        while ((a_buf[i + j] != b_buf[i + j]) && ((i + j) < size)) {
            j++;
        }

        pirnt_array_range(a_title, a_buf, i, j);
        pirnt_array_range(b_title, b_buf, i, j);
        i += j - 1;
    }
}

void set_buffer() {
    for (uint32_t i = 0; i < BUFFER_SIZE; i++) {
        spi_master_tx_buf[i] = i & 0xFF;
        spi_slave_tx_buf[i] = (0xFF - i) & 0xFF;
    }
    memset(spi_master_rx_buf, 0, BUFFER_SIZE);
    memset(spi_slave_rx_buf, 0, BUFFER_SIZE);

    // dump_buffer();
}

void dump_buffer() {
    Serial.print("Master Sent : ");
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        Serial.print(spi_master_tx_buf[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    Serial.print("Slave Recv  : ");
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        Serial.print(spi_slave_rx_buf[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    Serial.print("Slave Sent  : ");
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        Serial.print(spi_slave_tx_buf[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    Serial.print("Master Recv : ");
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        Serial.print(spi_master_rx_buf[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    // SPI Master
    // VSPI = CS: 5, CLK: 18, MOSI: 23, MISO: 19
    pinMode(VSPI_SS, OUTPUT);
    digitalWrite(VSPI_SS, HIGH);
    master.begin();

    // SPI Slave
    // HSPI = CS: 15, CLK: 14, MOSI: 13, MISO: 12
    slave.setDataMode(SPI_MODE0);
    slave.begin(HSPI);

    // connect same name pins each other
    // CS - CS, CLK - CLK, MOSI - MOSI, MISO - MISO

    set_buffer();
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
        printf("slave received size = %d\n", slave.size());
        // dump_buffer();

        if (memcmp(spi_slave_rx_buf, spi_master_tx_buf, BUFFER_SIZE)) {
            printf("[ERROR] Master -> Slave Received Data has not matched !!\n");
            print_if_not_matched("Received ", spi_slave_rx_buf, "Sent ", spi_master_tx_buf, BUFFER_SIZE);
        } else {
            printf("Master -> Slave Transfer Success\n");
        }

        if (memcmp(spi_master_rx_buf, spi_slave_tx_buf, BUFFER_SIZE)) {
            printf("[ERROR] Slave -> Master Received Data has not matched !!\n");
            print_if_not_matched("Received ", spi_master_rx_buf, "Sent ", spi_slave_tx_buf, BUFFER_SIZE);
        } else {
            printf("Slave -> Master Transfer Success\n");
        }

        set_buffer();

        // you should pop the received packet
        slave.pop();
    }

    delay(2000);
}
