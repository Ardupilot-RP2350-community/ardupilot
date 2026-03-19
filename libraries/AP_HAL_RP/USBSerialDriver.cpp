#include "USBSerialDriver.h"
#include <AP_Math/AP_Math.h>
#include "pico/platform.h"
#include "pico/unique_id.h"
#include "tusb.h"
#include <array>

using namespace RP;

extern const AP_HAL::HAL& hal;

namespace {

constexpr uint8_t USB_CDC_PORT_COUNT = 2;
constexpr uint8_t USB_CONFIGURATION_INDEX = 1;
constexpr uint8_t USB_STRING_INDEX_MANUFACTURER = 1;
constexpr uint8_t USB_STRING_INDEX_PRODUCT = 2;
constexpr uint8_t USB_STRING_INDEX_SERIAL = 3;
constexpr uint8_t USB_STRING_INDEX_CDC0 = 4;
constexpr uint8_t USB_STRING_INDEX_CDC1 = 5;
constexpr uint8_t USB_CDC_DATA_EP_SIZE = 64;
constexpr uint8_t USB_CDC_NOTIFICATION_EP_SIZE = 8;

static_assert(CFG_TUD_CDC == USB_CDC_PORT_COUNT, "USB CDC descriptor count must match USB_CDC_PORT_COUNT");

enum usb_interface_index : uint8_t {
    ITF_NUM_CDC0 = 0,
    ITF_NUM_CDC0_DATA,
    ITF_NUM_CDC1,
    ITF_NUM_CDC1_DATA,
    ITF_NUM_TOTAL
};

enum usb_endpoint_number : uint8_t {
    EPNUM_CDC0_NOTIF = 0x81,
    EPNUM_CDC0_OUT   = 0x02,
    EPNUM_CDC0_IN    = 0x82,
    EPNUM_CDC1_NOTIF = 0x83,
    EPNUM_CDC1_OUT   = 0x04,
    EPNUM_CDC1_IN    = 0x84,
};

constexpr uint8_t USB_DESCRIPTOR_LENGTH = TUD_CONFIG_DESC_LEN + (CFG_TUD_CDC * TUD_CDC_DESC_LEN);

const tusb_desc_device_t usb_device_descriptor = []() {
    tusb_desc_device_t descriptor{};
    descriptor.bLength = sizeof(tusb_desc_device_t);
    descriptor.bDescriptorType = TUSB_DESC_DEVICE;
    descriptor.bcdUSB = 0x0200;
    descriptor.bDeviceClass = TUSB_CLASS_MISC;
    descriptor.bDeviceSubClass = MISC_SUBCLASS_COMMON;
    descriptor.bDeviceProtocol = MISC_PROTOCOL_IAD;
    descriptor.bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE;
    descriptor.idVendor = 0x2DAE;
    descriptor.idProduct = 0x1010;
    descriptor.bcdDevice = 0x0100;
    descriptor.iManufacturer = USB_STRING_INDEX_MANUFACTURER;
    descriptor.iProduct = USB_STRING_INDEX_PRODUCT;
    descriptor.iSerialNumber = USB_STRING_INDEX_SERIAL;
    descriptor.bNumConfigurations = 1;
    return descriptor;
}();

const uint8_t usb_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(USB_CONFIGURATION_INDEX, ITF_NUM_TOTAL, 0, USB_DESCRIPTOR_LENGTH,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC0, USB_STRING_INDEX_CDC0, EPNUM_CDC0_NOTIF, USB_CDC_NOTIFICATION_EP_SIZE,
                       EPNUM_CDC0_OUT, EPNUM_CDC0_IN, USB_CDC_DATA_EP_SIZE),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC1, USB_STRING_INDEX_CDC1, EPNUM_CDC1_NOTIF, USB_CDC_NOTIFICATION_EP_SIZE,
                       EPNUM_CDC1_OUT, EPNUM_CDC1_IN, USB_CDC_DATA_EP_SIZE),
};

const char *const usb_string_descriptors[] = {
    nullptr,
    "ArduPilot",
    "ArduPilot Dual CDC",
    nullptr,
    "ArduPilot Console",
    "ArduPilot USB Serial",
};

uint16_t usb_string_descriptor_buffer[32];

class USBSerialManager {
public:
    static USBSerialManager &instance()
    {
        static USBSerialManager manager;
        return manager;
    }

    void register_driver(USBSerialDriver *driver, uint8_t interface_num)
    {
        if (interface_num < _drivers.size()) {
            _drivers[interface_num] = driver;
        }
    }

    void init()
    {
        if (_initialized) {
            return;
        }

        tusb_rhport_init_t dev_init{};
        dev_init.role = TUSB_ROLE_DEVICE;
        dev_init.speed = TUSB_SPEED_AUTO;
        tusb_init(0, &dev_init);
        _initialized = true;
    }

    void task()
    {
        if (!_initialized) {
            return;
        }
        tud_task();
    }

    bool connected(uint8_t interface_num) const
    {
        return interface_num < _line_state_dtr.size() && _line_state_dtr[interface_num];
    }

    void line_state_changed(uint8_t interface_num, bool dtr)
    {
        if (interface_num < _line_state_dtr.size()) {
            _line_state_dtr[interface_num] = dtr;
        }
    }

    USBSerialDriver *driver(uint8_t interface_num) const
    {
        if (interface_num >= _drivers.size()) {
            return nullptr;
        }
        return _drivers[interface_num];
    }

private:
    bool _initialized = false;
    std::array<USBSerialDriver *, USB_CDC_PORT_COUNT> _drivers{};
    std::array<bool, USB_CDC_PORT_COUNT> _line_state_dtr{};
};

} // namespace

extern "C" const uint8_t *tud_descriptor_device_cb(void)
{
    return reinterpret_cast<const uint8_t *>(&usb_device_descriptor);
}

extern "C" const uint8_t *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index;
    return usb_configuration_descriptor;
}

extern "C" const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;

    uint8_t chr_count = 0;
    if (index == 0) {
        usb_string_descriptor_buffer[1] = 0x0409;
        chr_count = 1;
    } else if (index == USB_STRING_INDEX_SERIAL) {
        char serial[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1] = {};
        pico_get_unique_board_id_string(serial, sizeof(serial));
        while (chr_count < (sizeof(usb_string_descriptor_buffer) / sizeof(usb_string_descriptor_buffer[0]) - 1) &&
               serial[chr_count] != '\0') {
            usb_string_descriptor_buffer[1 + chr_count] = serial[chr_count];
            chr_count++;
        }
    } else if (index < ARRAY_SIZE(usb_string_descriptors) && usb_string_descriptors[index] != nullptr) {
        const char *str = usb_string_descriptors[index];
        while (chr_count < (sizeof(usb_string_descriptor_buffer) / sizeof(usb_string_descriptor_buffer[0]) - 1) &&
               str[chr_count] != '\0') {
            usb_string_descriptor_buffer[1 + chr_count] = str[chr_count];
            chr_count++;
        }
    } else {
        return nullptr;
    }

    usb_string_descriptor_buffer[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);
    return usb_string_descriptor_buffer;
}

extern "C" void tud_cdc_line_state_cb(uint8_t instance, bool dtr, bool rts)
{
    (void)rts;
    USBSerialManager::instance().line_state_changed(instance, dtr);
}

USBSerialDriver::USBSerialDriver(uint8_t interface_num) :
    _interface_num(interface_num),
    _baudrate(115200),
    _initialized(false),
    _readbuf{UART_RX_BUFFER_SIZE},
    _writebuf{UART_TX_BUFFER_SIZE},
    _write_mutex{}
{
    USBSerialManager::instance().register_driver(this, _interface_num);
}

void USBSerialDriver::_begin(uint32_t baud, uint16_t rxSpace, uint16_t txSpace)
{
    _baudrate = baud;
    _readbuf.set_size(rxSpace >= 128 ? rxSpace : 128);
    _writebuf.set_size(txSpace >= 128 ? txSpace : 128);
    USBSerialManager::instance().init();
    _initialized = true;
}

void USBSerialDriver::_end()
{
    _initialized = false;
}

bool USBSerialDriver::is_usb_connected() const
{
    return USBSerialManager::instance().connected(_interface_num);
}

void USBSerialDriver::_poll_rx()
{
    if (!_initialized) {
        return;
    }

    while (tud_cdc_n_available(_interface_num) > 0 && _readbuf.space() > 0) {
        uint8_t c;
        if (tud_cdc_n_read(_interface_num, &c, 1) != 1) {
            break;
        }
        _readbuf.write(&c, 1);
    }
}

void USBSerialDriver::_flush_tx()
{
    if (!_initialized || !tud_cdc_n_connected(_interface_num)) {
        return;
    }

    while (_writebuf.available() > 0 && tud_cdc_n_write_available(_interface_num) > 0) {
        uint8_t chunk[USB_CDC_DATA_EP_SIZE];
        const uint16_t available = _writebuf.available();
        const uint32_t usb_space = tud_cdc_n_write_available(_interface_num);
        const uint16_t count = MIN<uint16_t>(available, MIN<uint32_t>(sizeof(chunk), usb_space));
        if (!_writebuf.read(chunk, count)) {
            break;
        }
        if (tud_cdc_n_write(_interface_num, chunk, count) != count) {
            break;
        }
    }
    tud_cdc_n_write_flush(_interface_num);
}

size_t USBSerialDriver::_write(const uint8_t *buffer, size_t size)
{
    if (!_initialized || size == 0) {
        return 0;
    }

    _write_mutex.take_blocking();

    for (size_t i = 0; i < size; i++) {
        const uint8_t required_space = (buffer[i] == '\n') ? 2 : 1;
        while (_writebuf.space() < required_space) {
            if (!hal.scheduler->is_system_initialized()) {
                _flush_tx();
            } else {
                _write_mutex.give();
                hal.scheduler->delay(1);
                _write_mutex.take_blocking();
            }
        }

        if (buffer[i] == '\n') {
            const uint8_t cr = '\r';
            _writebuf.write(&cr, 1);
        }
        _writebuf.write(&buffer[i], 1);
    }

    if (!hal.scheduler->is_system_initialized()) {
        _flush_tx();
    }

    _write_mutex.give();
    return size;
}

ssize_t USBSerialDriver::_read(uint8_t *buffer, uint16_t count)
{
    if (!_initialized) {
        return -1;
    }
    _poll_rx();
    return (ssize_t)_readbuf.read(buffer, count);
}

uint32_t USBSerialDriver::_available()
{
    _poll_rx();
    return _readbuf.available();
}

bool USBSerialDriver::tx_pending()
{
    return _writebuf.available() > 0;
}

void USBSerialDriver::_timer_tick()
{
    if (!_initialized) {
        return;
    }

    _poll_rx();
    _write_mutex.take_blocking();
    _flush_tx();
    _write_mutex.give();
}

void USBSerialDriver::_flush()
{
    while (tx_pending()) {
        _timer_tick();
    }
}

bool USBSerialDriver::_discard_input(void)
{
    _readbuf.clear();
    return true;
}

bool USBSerialDriver::set_options(uint16_t options)
{
    _last_options = options;
    return true;
}

void USBSerialDriver::configure_parity(uint8_t v)
{
    (void)v;
}

void USBSerialDriver::set_stop_bits(int n)
{
    (void)n;
}

uint64_t USBSerialDriver::receive_time_constraint_us(uint16_t nbytes)
{
    (void)nbytes;
    return AP_HAL::micros64();
}

uint32_t USBSerialDriver::txspace()
{
    if (!_initialized) {
        return 0;
    }
    return _writebuf.space();
}

void USBSerialDriver::vprintf(const char *fmt, va_list ap)
{
    if (!_initialized) {
        return;
    }

    char buffer[128];
    int n = vsnprintf(buffer, sizeof(buffer), fmt, ap);
    if (n > 0) {
        size_t len = (size_t)n;
        if (len >= sizeof(buffer)) {
            len = sizeof(buffer) - 1;
        }
        _write((const uint8_t *)buffer, len);
    }
}

size_t USBSerialDriver::write(uint8_t c)
{
    return _write(&c, 1);
}

size_t USBSerialDriver::write(const uint8_t *buffer, size_t size)
{
    return _write(buffer, size);
}

void USBSerialDriver::backend_task()
{
    USBSerialManager::instance().task();
}
