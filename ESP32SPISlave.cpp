#include "ESP32SPISlave.h"

ARDUINO_ESP32_SPI_SLAVE_NAMESPACE_BEGIN

void spi_slave_setup_done(spi_slave_transaction_t* trans) {
    // printf("[callback] SPI slave setup finished\n");
}

void spi_slave_trans_done(spi_slave_transaction_t* trans) {
    // printf("[callback] SPI slave transaction finished\n");
    ((Slave*)trans->user)->results.push_back(trans->trans_len);
    ((Slave*)trans->user)->transactions.pop_front();
}

bool Slave::begin(const uint8_t spi_bus, const int8_t sck, const int8_t miso, const int8_t mosi, const int8_t ss) {
    if ((sck == -1) && (miso == -1) && (mosi == -1) && (ss == -1)) {
        bus_cfg.sclk_io_num = (spi_bus == VSPI) ? SCK : 14;
        bus_cfg.miso_io_num = (spi_bus == VSPI) ? MISO : 12;
        bus_cfg.mosi_io_num = (spi_bus == VSPI) ? MOSI : 13;
        if_cfg.spics_io_num = (spi_bus == VSPI) ? SS : 15;
    } else {
        bus_cfg.sclk_io_num = sck;
        bus_cfg.miso_io_num = miso;
        bus_cfg.mosi_io_num = mosi;
        if_cfg.spics_io_num = ss;
    }

    bus_cfg.max_transfer_sz = SPI_MAX_SIZE_NO_DMA;

    if_cfg.flags = 0;  // Bitwise OR of SPI_SLAVE_* flags
    if_cfg.queue_size = queue_size;
    if_cfg.mode = mode;
    // TODO: callbacks
    if_cfg.post_setup_cb = spi_slave_setup_done;
    if_cfg.post_trans_cb = spi_slave_trans_done;

    host = (spi_bus == HSPI) ? HSPI_HOST : VSPI_HOST;
    esp_err_t e = spi_slave_initialize(host, &bus_cfg, &if_cfg, 0);  // do not use DMA

    return (e == ESP_OK);
}

bool Slave::end() {
    return (spi_slave_free(host) == ESP_OK);
}

bool Slave::wait(uint8_t* rx_buf, const size_t size) {
    return wait(rx_buf, NULL, size);
}

bool Slave::wait(uint8_t* rx_buf, const uint8_t* tx_buf, const size_t size) {
    if (!transactions.empty()) {
        printf("[ERROR] cannot execute transfer if queued transaction exists. queue size = %d\n", transactions.size());
        return false;
    }

    addTransaction(rx_buf, tx_buf, size);

    esp_err_t e = spi_slave_transmit(host, &transactions.back(), portMAX_DELAY);
    if (e != ESP_OK) {
        printf("[ERROR] SPI device transmit failed : %d\n", e);
        transactions.pop_back();
        return false;
    }

    return (e == ESP_OK);
}

bool Slave::queue(uint8_t* rx_buf, const size_t size) {
    return queue(rx_buf, NULL, size);
}

bool Slave::queue(uint8_t* rx_buf, const uint8_t* tx_buf, const size_t size) {
    if (transactions.size() >= queue_size) {
        printf("[WARNING] queue is full with transactions. discard new transaction request\n");
        return false;
    }

    addTransaction(rx_buf, tx_buf, size);
    esp_err_t e = spi_slave_queue_trans(host, &transactions.back(), portMAX_DELAY);

    return (e == ESP_OK);
}

void Slave::yield() {
    size_t n = transactions.size();
    for (uint8_t i = 0; i < n; ++i) {
        spi_slave_transaction_t* r_trans;
        esp_err_t e = spi_slave_get_trans_result(host, &r_trans, portMAX_DELAY);
        if (e != ESP_OK) {
            printf("[ERROR] SPI slave get trans result failed %d / %d : %d\n", i, n, e);
        }
    }
}

size_t Slave::remained() const {
    return transactions.size();
}

size_t Slave::available() const {
    return results.size();
}

uint32_t Slave::size() const {
    return results.front() / 8;
}

void Slave::pop() {
    results.pop_front();
}

void Slave::setDataMode(const uint8_t m) {
    mode = m;
}

void Slave::setQueueSize(const int s) {
    queue_size = s;
}

void Slave::addTransaction(uint8_t* rx_buf, const uint8_t* tx_buf, const size_t size) {
    transactions.emplace_back(spi_slave_transaction_t());
    // transactions.back().cmd = ;
    // transactions.back().addr = ;
    transactions.back().length = 8 * size;  // in bit size
    transactions.back().user = (void*)this;
    transactions.back().tx_buffer = tx_buf;
    transactions.back().rx_buffer = rx_buf;
}

void Slave::pushResult(const uint32_t s) {
    results.push_back(s);
}

ARDUINO_ESP32_SPI_SLAVE_NAMESPACE_END
