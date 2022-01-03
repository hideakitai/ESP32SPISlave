# ESP32SPISlave

SPI Slave library for ESP32

## ESP32DMASPI

This is the simple SPI slave library and does NOT use DMA. Please use [ESP32DMASPI](https://github.com/hideakitai/ESP32DMASPI) to transfer more than 32 bytes with DMA.

## Feature

- Support SPI Slave mode based on [ESP32's SPI Slave Driver](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_slave.html#spi-slave-driver)
- There are two ways to receive/send transactions
  - `wait()` to receive/send transaction one by one
  - `queue()` and `yield()` to receive/send multiple transactions at once (more efficient than `wait()` many times)

## Usage

### Wait for the transaction one by one

```C++
#include <ESP32SPISlave.h>

ESP32SPISlave slave;

static constexpr uint32_t BUFFER_SIZE {32};
uint8_t spi_slave_tx_buf[BUFFER_SIZE];
uint8_t spi_slave_rx_buf[BUFFER_SIZE];

void setup() {
    // HSPI = CS: 15, CLK: 14, MOSI: 13, MISO: 12 -> default
    // VSPI = CS:  5, CLK: 18, MOSI: 23, MISO: 19
    slave.setDataMode(SPI_MODE0);
    slave.begin(HSPI);
}

void loop() {
    // block until the transaction comes from master
    slave.wait(spi_slave_rx_buf, spi_slave_tx_buf, BUFFER_SIZE);

    // if transaction has completed from master,
    // available() returns size of results of transaction,
    // and `spi_slave_rx_buf` is automatically updated
    while (slave.available()) {
        // do something with `spi_slave_rx_buf`

        slave.pop();
    }
}
```

### Queue the transaction (polling)

```C++
#include <ESP32SPISlave.h>

ESP32SPISlave slave;

static constexpr uint32_t BUFFER_SIZE {32};
uint8_t spi_slave_tx_buf[BUFFER_SIZE];
uint8_t spi_slave_rx_buf[BUFFER_SIZE];

void setup() {
    // HSPI = CS: 15, CLK: 14, MOSI: 13, MISO: 12
    // VSPI = CS:  5, CLK: 18, MOSI: 23, MISO: 19
    slave.setDataMode(SPI_MODE0);
    slave.begin(VSPI);
}

void loop() {
    // if there is no transaction in queue, add transaction
    if (slave.remained() == 0)
        slave.queue(spi_slave_rx_buf, spi_slave_tx_buf, BUFFER_SIZE);

    // if transaction has completed from master,
    // available() returns size of results of transaction,
    // and `spi_slave_rx_buf` is automatically updated
    while (slave.available()) {
        // do something with `spi_slave_rx_buf`

        slave.pop();
    }
}
```

### Queue the transaction (task)

```C++
#include <ESP32SPISlave.h>

ESP32SPISlave slave;

static constexpr uint32_t BUFFER_SIZE {32};
uint8_t spi_slave_tx_buf[BUFFER_SIZE];
uint8_t spi_slave_rx_buf[BUFFER_SIZE];

constexpr uint8_t CORE_TASK_SPI_SLAVE {0};
constexpr uint8_t CORE_TASK_PROCESS_BUFFER {0};

static TaskHandle_t task_handle_wait_spi = 0;
static TaskHandle_t task_handle_process_buffer = 0;

void task_wait_spi(void* pvParameters) {
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // block until the transaction comes from master
        slave.wait(spi_slave_rx_buf, spi_slave_tx_buf, BUFFER_SIZE);

        xTaskNotifyGive(task_handle_process_buffer);
    }
}

void task_process_buffer(void* pvParameters) {
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // do something with `spi_slave_rx_buf`

        slave.pop();

        xTaskNotifyGive(task_handle_wait_spi);
    }
}

void setup() {
    // HSPI = CS: 15, CLK: 14, MOSI: 13, MISO: 12
    // VSPI = CS: 5, CLK: 18, MOSI: 23, MISO: 19
    slave.setDataMode(SPI_MODE0);
    slave.begin(HSPI);

    xTaskCreatePinnedToCore(task_wait_spi, "task_wait_spi", 2048, NULL, 2, &task_handle_wait_spi, CORE_TASK_SPI_SLAVE);
    xTaskNotifyGive(task_handle_wait_spi);

    xTaskCreatePinnedToCore(task_process_buffer, "task_process_buffer", 2048, NULL, 2, &task_handle_process_buffer, CORE_TASK_PROCESS_BUFFER);
}

void loop() {
}
```

## APIs

````C++
bool begin(const uint8_t spi_bus = HSPI);
bool begin(const uint8_t spi_bus, const int8_t sck, const int8_t miso, const int8_t mosi, const int8_t ss);
bool end();

// wait for transaction one by one
bool wait(uint8_t* rx_buf, const size_t size);  // no data to master
bool wait(uint8_t* rx_buf, const uint8_t* tx_buf, const size_t size);

// queueing transaction
// wait (blocking) and timeout occurs if queue is full with transaction
// (but designed not to queue transaction more than queue_size, so there is no timeout argument)
bool queue(uint8_t* rx_buf, const size_t size);  // no data to master
bool queue(uint8_t* rx_buf, const uint8_t* tx_buf, const size_t size);

// wait until all queued transaction will be done by master
// if yield is finished, all the buffer is updated to latest
void yield();

// transaction result info
size_t available() const;
size_t remained() const;
uint32_t size() const;
void pop();

// ===== Main Configurations =====
// set these optional parameters before begin() if you want
void setDataMode(const uint8_t m);

// ===== Optional Configurations =====
void setSlaveFlags(const uint32_t flags);  // OR of SPI_SLAVE_* flags
void setQueueSize(const int n);
void setSpiMode(const uint8_t m);
```

## TODO (PR welcome!!)

- more configs? (SPI ISR callbacks, SPI cmd/addr/user feature, etc...)

## License

MIT
````
