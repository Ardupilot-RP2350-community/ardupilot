#include "PIOSerialDriver.h"

#include "hardware/gpio.h"
#include "pico/stdlib.h"

#if __has_include("uart_rx.pio.h") && __has_include("uart_tx.pio.h")
#include "uart_rx.pio.h"
#include "uart_tx.pio.h"
#define AP_HAL_RP_PIO_UART_PROGRAMS_AVAILABLE 1
#else
#define AP_HAL_RP_PIO_UART_PROGRAMS_AVAILABLE 0
#endif

using namespace RP;

extern const AP_HAL::HAL& hal;

PIOSerialDriver::PIOSerialDriver(PIO pio, uint8_t tx_pin, uint8_t rx_pin, bool console) :
    _pio(pio),
    _tx_pin(tx_pin),
    _rx_pin(rx_pin),
    _baudrate(0),
    _initialized(false),
    _console(console),
    _receive_errors(0),
    _sm_tx(-1),
    _sm_rx(-1),
    _offset_tx(0),
    _offset_rx(0),
    _readbuf{UART_RX_BUFFER_SIZE},
    _writebuf{UART_TX_BUFFER_SIZE},
    _write_mutex{}
{}

void PIOSerialDriver::_begin(uint32_t baud, uint16_t rxSpace, uint16_t txSpace)
{
    if (_initialized) {
        _end();
    }

    _baudrate = baud;
    _readbuf.set_size(rxSpace >= 128 ? rxSpace : 128);
    _writebuf.set_size(txSpace >= 128 ? txSpace : 128);
    _readbuf.clear();
    _writebuf.clear();

#if AP_HAL_RP_PIO_UART_PROGRAMS_AVAILABLE
    uint sm_tx = 0;
    uint sm_rx = 0;

    if (!pio_claim_free_sm_and_add_program_for_gpio_range(&uart_tx_program, &_pio, &sm_tx, &_offset_tx, _tx_pin, 1, true)) {
        return;
    }

    if (!pio_claim_free_sm_and_add_program_for_gpio_range(&uart_rx_program, &_pio, &sm_rx, &_offset_rx, _rx_pin, 1, true)) {
        pio_remove_program_and_unclaim_sm(&uart_tx_program, _pio, sm_tx, _offset_tx);
        return;
    }

    _sm_tx = sm_tx;
    _sm_rx = sm_rx;

    uart_tx_program_init(_pio, _sm_tx, _offset_tx, _tx_pin, _baudrate);
    uart_rx_program_init(_pio, _sm_rx, _offset_rx, _rx_pin, _baudrate);

    set_options(_last_options);
    _initialized = true;
#else
    (void)_last_options;
#endif
}

void PIOSerialDriver::_end()
{
#if AP_HAL_RP_PIO_UART_PROGRAMS_AVAILABLE
    if (_sm_rx >= 0) {
        pio_remove_program_and_unclaim_sm(&uart_rx_program, _pio, _sm_rx, _offset_rx);
        _sm_rx = -1;
    }

    if (_sm_tx >= 0) {
        pio_remove_program_and_unclaim_sm(&uart_tx_program, _pio, _sm_tx, _offset_tx);
        _sm_tx = -1;
    }
#endif

    _initialized = false;
}

void PIOSerialDriver::_drain_rx_from_hardware()
{
#if AP_HAL_RP_PIO_UART_PROGRAMS_AVAILABLE
    if (!_initialized || _sm_rx < 0) {
        return;
    }

    while (!pio_sm_is_rx_fifo_empty(_pio, _sm_rx)) {
        const uint8_t c = (uint8_t)uart_rx_program_getc(_pio, _sm_rx);
        if (!_readbuf.write(&c, 1)) {
            _receive_errors++;
        }
    }
#endif
}

size_t PIOSerialDriver::_write(const uint8_t *buffer, size_t size)
{
    if (!_initialized || size == 0) {
        return 0;
    }

    _write_mutex.take_blocking();

    for (size_t i = 0; i < size; i++) {
        const uint8_t required_space = (is_console() && buffer[i] == '\n') ? 2 : 1;

        while (_writebuf.space() < required_space) {
            if (!hal.scheduler->is_system_initialized()) {
                _flush_tx_to_hardware();
            } else {
                _write_mutex.give();
                hal.scheduler->delay(1);
                _write_mutex.take_blocking();
            }
        }

        if (is_console() && buffer[i] == '\n') {
            const uint8_t cr = '\r';
            _writebuf.write(&cr, 1);
        }
        _writebuf.write(&buffer[i], 1);
    }

    if (!hal.scheduler->is_system_initialized()) {
        _flush_tx_to_hardware();
    }

    _write_mutex.give();
    return size;
}

void PIOSerialDriver::_flush_tx_to_hardware()
{
#if AP_HAL_RP_PIO_UART_PROGRAMS_AVAILABLE
    if (_sm_tx < 0) {
        return;
    }

    uint16_t n = _writebuf.available();
    while (n > 0 && !pio_sm_is_tx_fifo_full(_pio, _sm_tx)) {
        uint8_t c = 0;
        if (_writebuf.read(&c, 1)) {
            uart_tx_program_putc(_pio, _sm_tx, c);
        }
        n--;
    }
#endif
}

ssize_t PIOSerialDriver::_read(uint8_t *buffer, uint16_t count)
{
    if (!_initialized) {
        return -1;
    }

    _drain_rx_from_hardware();
    return (ssize_t)_readbuf.read(buffer, count);
}

uint32_t PIOSerialDriver::_available()
{
    _drain_rx_from_hardware();
    return _readbuf.available();
}

bool PIOSerialDriver::tx_pending()
{
#if AP_HAL_RP_PIO_UART_PROGRAMS_AVAILABLE
    if (!_initialized || _sm_tx < 0) {
        return false;
    }

    return (_writebuf.available() > 0) || !pio_sm_is_tx_fifo_empty(_pio, _sm_tx);
#else
    return false;
#endif
}

void PIOSerialDriver::_timer_tick()
{
    if (!_initialized) {
        return;
    }

    _write_mutex.take_blocking();
    _flush_tx_to_hardware();
    _write_mutex.give();

    _drain_rx_from_hardware();
}

void PIOSerialDriver::_flush()
{
    while (tx_pending()) {
        _timer_tick();
    }
}

bool PIOSerialDriver::_discard_input(void)
{
    _readbuf.clear();
    return true;
}

bool PIOSerialDriver::set_options(uint16_t options)
{
    _last_options = options;

    if (!_initialized) {
        return true;
    }

    gpio_set_outover(_tx_pin, (options & OPTION_TXINV) ? GPIO_OVERRIDE_INVERT : GPIO_OVERRIDE_NORMAL);
    gpio_set_inover(_rx_pin, (options & OPTION_RXINV) ? GPIO_OVERRIDE_INVERT : GPIO_OVERRIDE_NORMAL);

    if (options & OPTION_HDPLEX) {
        gpio_set_oeover(_tx_pin, GPIO_OVERRIDE_NORMAL);
    }

    return true;
}

void PIOSerialDriver::configure_parity(uint8_t v)
{
    (void)v;
    // PIO UART programs currently implement 8N1 only.
}

void PIOSerialDriver::set_stop_bits(int n)
{
    (void)n;
    // PIO UART programs currently implement 8N1 only.
}

uint64_t PIOSerialDriver::receive_time_constraint_us(uint16_t nbytes)
{
    if (_baudrate == 0) {
        return AP_HAL::micros64();
    }

    const uint64_t byte_time_us = 10000000ULL / _baudrate;
    return AP_HAL::micros64() - (nbytes * byte_time_us);
}

uint32_t PIOSerialDriver::txspace()
{
    if (!_initialized) {
        return 0;
    }

    return _writebuf.space();
}

void PIOSerialDriver::vprintf(const char *fmt, va_list ap)
{
    if (!_initialized) {
        return;
    }

    char buffer[128];
    int n = hal.util->vsnprintf(buffer, sizeof(buffer), fmt, ap);
    if (n > 0) {
        size_t len = (size_t)n;
        if (len >= sizeof(buffer)) {
            len = sizeof(buffer) - 1;
        }
        _write((const uint8_t *)buffer, len);
    }
}

size_t PIOSerialDriver::write(uint8_t c)
{
    return _write(&c, 1);
}

size_t PIOSerialDriver::write(const uint8_t *buffer, size_t size)
{
    return _write(buffer, size);
}
