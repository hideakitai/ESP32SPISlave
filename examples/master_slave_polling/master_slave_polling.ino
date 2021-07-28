#include <ESP32DMASPIMaster.h>
#include <ESP32SPISlave.h>

#define VSPI_SS SS  // 5
SPIClass master(VSPI);
ESP32SPISlave slave;

// Reference: https://rabbit-note.com/2019/01/20/esp32-arduino-spi-slave/

static const uint32_t BUFFER_SIZE = 32;
uint8_t spi_master_tx_buf[BUFFER_SIZE];
uint8_t spi_master_rx_buf[BUFFER_SIZE];
uint8_t spi_slave_tx_buf[BUFFER_SIZE];
uint8_t spi_slave_rx_buf[BUFFER_SIZE];

void dump_buf(const char* title, uint8_t* buf, uint32_t start, uint32_t len) {
    printf("len = %d\n", len);
    if (len == 1)
        printf("%s [%d]: ", title, start);
    else
        printf("%s [%d-%d]: ", title, start, start + len - 1);

    for (uint32_t i = 0; i < len; i++)
        printf("%02X ", buf[start + i]);

    printf("\n");
}

void cmp_bug(const char* a_title, uint8_t* a_buf, const char* b_title, uint8_t* b_buf, uint32_t size) {
    printf("size = %d\n", size);
    for (uint32_t i = 0; i < size; i++) {
        uint32_t j = 1;

        if (a_buf[i] == b_buf[i])
            continue;

        while ((a_buf[i + j] != b_buf[i + j]) && ((i + j) < size))
            j++;

        dump_buf(a_title, a_buf, i, j);
        dump_buf(b_title, b_buf, i, j);
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

    dump_buffer();
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

    delay(5000);

    // VSPI = CS: 5, CLK: 18, MOSI: 23, MISO: 19
    pinMode(VSPI_SS, OUTPUT);
    digitalWrite(VSPI_SS, HIGH);
    master.begin();

    slave.setDataMode(SPI_MODE0);
    // begin() after setting
    // HSPI = CS: 15, CLK: 14, MOSI: 13, MISO: 12
    slave.begin(HSPI);

    // connect same name pins each other
    // CS - CS, CLK - CLK, MOSI - MOSI, MISO - MISO

    printf("Master Slave Polling Test Start\n");

    set_buffer();
}

void loop() {
    // just queue transaction
    // if transaction has completed from master, buffer is automatically updated
    slave.queue(spi_slave_rx_buf, spi_slave_tx_buf, BUFFER_SIZE);

    // static uint32_t count = 0;
    // if (count++ % 3 == 0)
    // {
    // start transaction
    master.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    digitalWrite(VSPI_SS, LOW);
    master.transferBytes(spi_master_tx_buf, spi_master_rx_buf, BUFFER_SIZE);
    // for (size_t i = 0; i < BUFFER_SIZE; ++i)
    //     spi_master_rx_buf[i] = master.transfer(spi_master_tx_buf[i]);
    digitalWrite(VSPI_SS, HIGH);
    master.endTransaction();
    // }

    // if slave has received transaction data, available() returns size of received transactions
    while (slave.available()) {
        printf("slave received size = %d\n", slave.size());
        dump_buffer();

        if (memcmp(spi_slave_rx_buf, spi_master_tx_buf, BUFFER_SIZE)) {
            printf("[ERROR] Master -> Slave Received Data has not matched !!\n");
            cmp_bug("Received ", spi_slave_rx_buf, "Sent ", spi_master_tx_buf, BUFFER_SIZE);
        } else {
            printf("Master -> Slave Transfer Success\n");
        }

        if (memcmp(spi_master_rx_buf, spi_slave_tx_buf, BUFFER_SIZE)) {
            printf("[ERROR] Slave -> Master Received Data has not matched !!\n");
            cmp_bug("Received ", spi_master_rx_buf, "Sent ", spi_slave_tx_buf, BUFFER_SIZE);
        } else {
            printf("Slave -> Master Transfer Success\n");
        }

        // memset(spi_master_rx_buf, 0, BUFFER_SIZE);
        // memset(spi_slave_rx_buf, 0, BUFFER_SIZE);
        set_buffer();

        slave.pop();
    }

    delay(2000);
}
