# ESP32SPISlave

SPI Slave library for ESP32

## ESP32DMASPI

This is the simple SPI slave library and does NOT use DMA. Please use [ESP32DMASPI](https://github.com/hideakitai/ESP32DMASPI) to transfer more than 32 bytes with DMA.

## Feature

- Support SPI Slave mode based on [ESP32's SPI Slave Driver](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_slave.html#spi-slave-driver)
- There are two ways to receive/send transactions
  - `wait()` to receive/send transaction one by one
  - `queue()` and `yield()` to receive/send multiple transactions at once (more efficient than `wait()` many times)

### Supported ESP32 Version

| IDE         | ESP32 Board Version |
| ----------- | ------------------- |
| Arduino IDE | `>= 2.0.11`         |
| PlatformIO  | `>= 5.0.0`          |

## Usage

### Wait for the transaction one by one

```C++
#include <ESP32SPISlave.h>

ESP32SPISlave slave;

static constexpr uint32_t BUFFER_SIZE {32};
uint8_t spi_slave_tx_buf[BUFFER_SIZE];
uint8_t spi_slave_rx_buf[BUFFER_SIZE];

void setup() {
    // note: the default pins are different depending on the board
    // please refer to README Section "SPI Buses and SPI Pins" for more details
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
    // note: the default pins are different depending on the board
    // please refer to README Section "SPI Buses and SPI Pins" for more details
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
    // note: the default pins are different depending on the board
    // please refer to README Section "SPI Buses and SPI Pins" for more details
    slave.setDataMode(SPI_MODE0);
    slave.begin(HSPI);

    xTaskCreatePinnedToCore(task_wait_spi, "task_wait_spi", 2048, NULL, 2, &task_handle_wait_spi, CORE_TASK_SPI_SLAVE);
    xTaskNotifyGive(task_handle_wait_spi);

    xTaskCreatePinnedToCore(task_process_buffer, "task_process_buffer", 2048, NULL, 2, &task_handle_process_buffer, CORE_TASK_PROCESS_BUFFER);
}

void loop() {
}
```

## SPI Buses and SPI Pins

This library's `bool begin(const uint8_t spi_bus = HSPI)` function uses `HSPI` as the default SPI bus as same as `SPI` library of `arduino-esp32` ([reference](https://github.com/espressif/arduino-esp32/blob/099b432d10fb4ca1529c52241bcadcb8a4386f17/libraries/SPI/src/SPI.h#L61)).

The pins for SPI buses are automatically attached as follows. "Default SPI Pins" means the pins defined there are the same as `MOSI`, `MISO`, `SCK`, and `SS`.

| Board     | SPI       | MOSI | MISO | SCK | SS  | Default SPI Pins |
| --------- | --------- | ---- | ---- | --- | --- | ---------------- |
| `esp32`   | HSPI      | 13   | 12   | 14  | 15  | No               |
| `esp32`   | VSPI/FSPI | 23   | 19   | 18  | 5   | Yes              |
| `esp32s2` | HSPI/FSPI | 35   | 37   | 36  | 34  | Yes              |
| `esp32s3` | HSPI/FSPI | 11   | 13   | 12  | 10  | Yes              |
| `esp32c3` | HSPI/FSPI | 6    | 5    | 4   | 7   | Yes              |

Depending on your board, the default SPI pins are defined in `pins_arduino.h`. For example, `esp32`'s default SPI pins are found [here](https://github.com/espressif/arduino-esp32/blob/e1f14331f173a00a9062f616bc9a62c358b9076f/variants/esp32/pins_arduino.h#L20-L23) (`MOSI: 23, MISO: 19, SCK: 18, SS: 5`). Please refer to [arduino-esp32/variants](https://github.com/espressif/arduino-esp32/tree/e1f14331f173a00a9062f616bc9a62c358b9076f/variants)  for your board's default SPI pins.

The supported SPI buses are different from the ESP32 chip. Please note that there may be a restriction to use `FSPI` for your SPI bus. (Note: though `arduino-esp32` still uses `FSPI` and `HSPI` for all chips (v2.0.11), these are deprecated for the chips after `esp32s2`)

| Chip     | FSPI              | HSPI              | VSPI               |
| -------- | ----------------- | ----------------- | ------------------ |
| ESP32    | SPI1_HOST(`0`) [^1] | SPI2_HOST(`1`) [^2] | SPI3_HOST (`2`) [^3] |
| ESP32-S2 | SPI2_HOST(`1`)      | SPI3_HOST(`2`)      | -                  |
| ESP32-S3 | SPI2_HOST(`1`)      | SPI3_HOST(`2`)      | -                  |
| ESP32-C3 | SPI2_HOST(`1`)      | SPI2_HOST(`1`)      | -                  |

[^1]: SPI bus attached to the flash (can use the same data lines but different SS)
[^2]: SPI bus normally mapped to pins 12 - 15 on ESP32 but can be matrixed to any pins
[^3]: SPI bus normally attached to pins 5, 18, 19, and 23 on ESP32 but can be matrixed to any pins

<details>
<summary>Reference</summary>

- https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/spi_master.html
- https://github.com/espressif/arduino-esp32/blob/099b432d10fb4ca1529c52241bcadcb8a4386f17/cores/esp32/esp32-hal-spi.h#L28-L37
- https://github.com/espressif/arduino-esp32/blob/099b432d10fb4ca1529c52241bcadcb8a4386f17/libraries/SPI/src/SPI.cpp#L346-L350
- https://github.com/espressif/arduino-esp32/blob/099b432d10fb4ca1529c52241bcadcb8a4386f17/cores/esp32/esp32-hal-spi.h#L28-L37
- https://github.com/espressif/arduino-esp32/blob/099b432d10fb4ca1529c52241bcadcb8a4386f17/cores/esp32/esp32-hal-spi.c#L719-L752
- https://github.com/espressif/arduino-esp32/blob/099b432d10fb4ca1529c52241bcadcb8a4386f17/tools/sdk/esp32/include/driver/include/driver/sdspi_host.h#L23-L29
- https://github.com/espressif/arduino-esp32/blob/099b432d10fb4ca1529c52241bcadcb8a4386f17/tools/sdk/esp32/include/hal/include/hal/spi_types.h#L26-L31
- https://github.com/espressif/arduino-esp32/blob/099b432d10fb4ca1529c52241bcadcb8a4386f17/tools/sdk/esp32/include/hal/include/hal/spi_types.h#L77-L87

</details>

## APIs

```C++
// use SPI with the default pin assignment
bool begin(const uint8_t spi_bus = HSPI);
// use SPI with your own pin assignment
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
size_t available() const;  // size of completed (received) transaction results
size_t remained() const;   // size of queued (not completed) transactions
uint32_t size() const;     // size of the received bytes of the oldest queued transaction result
void pop();                // pop the oldest transaction result

// ===== Main Configurations =====
// set these optional parameters before begin() if you want
void setDataMode(const uint8_t m);

// ===== Optional Configurations =====
void setSlaveFlags(const uint32_t flags);  // OR of SPI_SLAVE_* flags
void setQueueSize(const int n);
void setSpiMode(const uint8_t m);
```

## License

MIT
