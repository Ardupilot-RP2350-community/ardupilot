#include "RCOutput.h"
#include <AP_Math/AP_Math.h>

#if defined(HAL_RCOUT_DRIVER_ENABLED) && HAL_RCOUT_DRIVER_ENABLED == 1

#include "pwm_multi.pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/sync.h"
#include "pico/multicore.h"
#include "pico/platform.h"
#include "pico/time.h"
#include <cstring>

using namespace RP;

extern const AP_HAL::HAL& hal;

uint32_t RCOutput::_bit_buffer[2][RCOUT_PWM_RESOLUTION] __attribute__((aligned(16)));
uint8_t RCOutput::_write_idx = 0;
struct RCOutput::DMA_ControlBlock RCOutput::_dma_blocks[2] __attribute__((aligned(8)));

RCOutput* RCOutput::_instance = nullptr;
volatile uint8_t RCOutput::_dma_busy_idx = 0;

void RCOutput::init() {
    _instance = this;

    memset(_serial_led_counts, 0, sizeof(_serial_led_counts));
    memset(_serial_led_mode, 0, sizeof(_serial_led_mode));
    memset(_serial_led_data, 0, sizeof(_serial_led_data));

    _pio_offset = pio_add_program(_pio, &pwm_multi_program);
    _sm = pio_claim_unused_sm(_pio, true);

    for (uint i = 0; i < MAX_CHANNELS; i++) {
        pio_gpio_init(_pio, RCOUT_GPIO_BASE + i);
    }
    pio_sm_set_consecutive_pindirs(_pio, _sm, RCOUT_GPIO_BASE, MAX_CHANNELS, true);

#if defined(RCOUT_SERIAL_LED_PIN)
    pio_gpio_init(_pio, RCOUT_SERIAL_LED_PIN);
    gpio_set_dir(RCOUT_SERIAL_LED_PIN, true);
#endif

    pio_sm_config c = pwm_multi_program_get_default_config(_pio_offset);
    sm_config_set_out_pins(&c, RCOUT_GPIO_BASE, MAX_CHANNELS);
    sm_config_set_out_shift(&c, true, false, 32);
    
    float div = (float)clock_get_hz(clk_sys) / (400.0f * (3.0f * RCOUT_PWM_RESOLUTION + 1.0f));
    sm_config_set_clkdiv(&c, div);
    pio_sm_init(_pio, _sm, _pio_offset, &c);

    _dma_chan = dma_claim_unused_channel(true);
    _dma_timer = dma_claim_unused_channel(true);

    dma_channel_config cfg_a = dma_channel_get_default_config(_dma_chan);
    channel_config_set_transfer_data_size(&cfg_a, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg_a, true);
    channel_config_set_dreq(&cfg_a, pio_get_dreq(_pio, _sm, true));
    channel_config_set_chain_to(&cfg_a, _dma_timer);

    dma_channel_configure(_dma_chan, &cfg_a, &_pio->txf[_sm], _bit_buffer[0], RCOUT_PWM_RESOLUTION, false);

    dma_channel_config cfg_b = dma_channel_get_default_config(_dma_timer);
    channel_config_set_transfer_data_size(&cfg_b, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg_b, true);
    channel_config_set_dreq(&cfg_b, pio_get_dreq(_pio, _sm, true));
    channel_config_set_chain_to(&cfg_b, _dma_chan);

    dma_channel_configure(_dma_timer, &cfg_b, &_pio->txf[_sm], _bit_buffer[1], RCOUT_PWM_RESOLUTION, false);

    dma_channel_set_irq0_enabled(_dma_chan, true);
    dma_channel_set_irq0_enabled(_dma_timer, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    pio_sm_set_enabled(_pio, _sm, false);
    pio_sm_put_blocking(_pio, _sm, RCOUT_PWM_RESOLUTION - 1);
    pio_sm_exec(_pio, _sm, pio_encode_pull(false, false));
    pio_sm_exec(_pio, _sm, pio_encode_mov(pio_isr, pio_osr));
    
    pio_sm_set_enabled(_pio, _sm, true);

    dma_channel_start(_dma_chan);

    DEV_PRINTF("init: 2-channel ping-pong started, res=%u\n", RCOUT_PWM_RESOLUTION);
}

bool RCOutput::set_serial_led_num_LEDs(const uint16_t chan, uint8_t num_leds, output_mode mode, uint32_t clock_mask)
{
    (void)clock_mask;
    uint8_t serial_led_gpio;
    if (!serial_led_get_gpio(chan, serial_led_gpio)) {
        return false;
    }
    if (chan >= MAX_CHANNELS || num_leds == 0 || num_leds > SERIAL_LED_MAX_LEDS) {
        return false;
    }
    if (mode != MODE_NEOPIXEL && mode != MODE_NEOPIXELRGB) {
        return false;
    }
    _serial_led_counts[chan] = num_leds;
    _serial_led_mode[chan] = mode;
    memset(_serial_led_data[chan], 0, sizeof(_serial_led_data[chan]));
    enable_ch(chan);
    return true;
}

bool RCOutput::set_serial_led_rgb_data(const uint16_t chan, int8_t led, uint8_t red, uint8_t green, uint8_t blue)
{
    uint8_t serial_led_gpio;
    if (!serial_led_get_gpio(chan, serial_led_gpio)) {
        return false;
    }
    if (chan >= MAX_CHANNELS || _serial_led_counts[chan] == 0) {
        return false;
    }
    if (led == -1) {
        for (uint8_t i = 0; i < _serial_led_counts[chan]; i++) {
            _serial_led_data[chan][i].red = red;
            _serial_led_data[chan][i].green = green;
            _serial_led_data[chan][i].blue = blue;
        }
        return true;
    }
    if (led < 0 || led >= _serial_led_counts[chan]) {
        return false;
    }
    _serial_led_data[chan][led].red = red;
    _serial_led_data[chan][led].green = green;
    _serial_led_data[chan][led].blue = blue;
    return true;
}

uint32_t RCOutput::cycles_from_ns(uint32_t ns) const
{
    const uint64_t clk_hz = clock_get_hz(clk_sys);
    const uint64_t cycles = (clk_hz * ns + 999999999ULL) / 1000000000ULL;
    return cycles == 0 ? 1U : uint32_t(cycles);
}

bool RCOutput::serial_led_get_gpio(const uint16_t chan, uint8_t &gpio) const
{
    if (chan >= MAX_CHANNELS) {
        return false;
    }

#if defined(RCOUT_SERIAL_LED_PIN)
#if defined(RCOUT_SERIAL_LED_CHAN)
    if (chan != RCOUT_SERIAL_LED_CHAN) {
        return false;
    }
#endif
    gpio = RCOUT_SERIAL_LED_PIN;
    return true;
#else
    gpio = RCOUT_GPIO_BASE + chan;
    return true;
#endif
}

bool RCOutput::serial_led_send_neopixel(const uint16_t chan, bool rgb_mode)
{
    if (chan >= MAX_CHANNELS || _serial_led_counts[chan] == 0) {
        return false;
    }
    uint8_t serial_led_gpio;
    if (!serial_led_get_gpio(chan, serial_led_gpio)) {
        return false;
    }
    const uint pin = serial_led_gpio;
    const uint32_t t0h = cycles_from_ns(350);
    const uint32_t t1h = cycles_from_ns(700);
    const uint32_t tbit = cycles_from_ns(1250);

    uint32_t irq_state = save_and_disable_interrupts();
    for (uint8_t i = 0; i < _serial_led_counts[chan]; i++) {
        const SerialLED &led = _serial_led_data[chan][i];
        uint32_t bits = rgb_mode ?
            ((uint32_t(led.red) << 16) | (uint32_t(led.green) << 8) | led.blue) :
            ((uint32_t(led.green) << 16) | (uint32_t(led.red) << 8) | led.blue);
        for (uint8_t bit = 0; bit < 24; bit++) {
            const bool one = (bits & 0x800000U) != 0;
            const uint32_t th = one ? t1h : t0h;
            gpio_put(pin, 1);
            busy_wait_at_least_cycles(th);
            gpio_put(pin, 0);
            busy_wait_at_least_cycles(tbit - th);
            bits <<= 1;
        }
    }
    restore_interrupts(irq_state);
    busy_wait_us_32(60);
    return true;
}

bool RCOutput::serial_led_send(const uint16_t chan)
{
    uint8_t serial_led_gpio;
    if (!serial_led_get_gpio(chan, serial_led_gpio)) {
        return false;
    }
    if (chan >= MAX_CHANNELS) {
        return false;
    }

    switch (_serial_led_mode[chan]) {
    case MODE_NEOPIXEL:
        return serial_led_send_neopixel(chan, false);
    case MODE_NEOPIXELRGB:
        return serial_led_send_neopixel(chan, true);
    default:
        return false;
    }
}

void RCOutput::handle_dma_irq() {
    uint32_t ints = dma_hw->ints0;
    dma_hw->ints0 = ints;

    if (ints & (1u << _dma_chan)) {
        _dma_busy_idx = 1;
        dma_channel_set_read_addr(_dma_chan, _bit_buffer[0], false);
    }

    if (ints & (1u << _dma_timer)) {
        _dma_busy_idx = 0;
        dma_channel_set_read_addr(_dma_timer, _bit_buffer[1], false);
    }
}

void RCOutput::handle_dma_irq(void *obj) {
    if (obj) {
        ((RCOutput *)obj)->handle_dma_irq();
    }
}

void RCOutput::dma_handler() {
    handle_dma_irq(_instance);
}

void RCOutput::set_freq(uint32_t chmask, uint16_t freq_hz) {
    if (freq_hz == 0) return;
    _current_freq = freq_hz;

    float div = (float)clock_get_hz(clk_sys) / ((float)freq_hz * 3.0f * (float)RCOUT_PWM_RESOLUTION);
    pio_sm_set_clkdiv(_pio, _sm, div);
}

uint16_t RCOutput::get_freq(uint8_t chan) {
    return _current_freq;
}

void RCOutput::enable_ch(uint8_t chan)
{
    if (chan >= MAX_CHANNELS) return;
    _enabled_mask |= (1u << chan);
    _need_update = true;
}

void RCOutput::disable_ch(uint8_t chan)
{
    if (chan >= MAX_CHANNELS) return;
    _enabled_mask &= ~(1u << chan);
    _need_update = true;
}

void RCOutput::write(uint8_t chan, uint16_t period_us)
{
    if (chan >= MAX_CHANNELS) return;
    if (_periods[chan] == period_us) return;

    _periods[chan] = period_us;
    _need_update = true;
    if (!_corked) push();
}

void RCOutput::push() {
    if (!_need_update || _corked) return;

    _write_idx = 1 - _dma_busy_idx;

    update_bit_buffer(); 
    
    _need_update = false;
}

uint16_t RCOutput::read(uint8_t chan)
{
    // Check if the channel number does not exceed the available 32 channels
    if (chan >= MAX_CHANNELS) {
        return 0;
    }
    // Return the stored value from our _periods array.
    // This is the value that was set via write() or initialized in init().
    return _periods[chan];
}

void RCOutput::read(uint16_t* period_us, uint8_t len)
{
    // Determine how many channels we can read
    uint8_t count = (len < MAX_CHANNELS) ? len : MAX_CHANNELS;
    for (uint8_t i = 0; i < count; i++) {
        // Return values from our internal _periods array
        period_us[i] = _periods[i];
    }
    // If more channels are requested than we have, we will reset the rest to zero
    for (uint8_t i = count; i < len; i++) {
        period_us[i] = 0;
    }
}

void RCOutput::update_bit_buffer() {
    uint32_t* target = _bit_buffer[_write_idx];
    memset(target, 0, RCOUT_PWM_RESOLUTION * sizeof(uint32_t));

    const uint32_t frame_us = 1000000u / _current_freq;

    for (uint8_t ch = 0; ch < MAX_CHANNELS; ch++) {
        if (!(_enabled_mask & (1u << ch))) continue;

        uint32_t pulse_us = _periods[ch];
        if (pulse_us > frame_us) pulse_us = frame_us;

        uint32_t steps = pulse_us * RCOUT_PWM_RESOLUTION / frame_us;
        if (steps > RCOUT_PWM_RESOLUTION) steps = RCOUT_PWM_RESOLUTION;

        for (uint32_t t = 0; t < steps; t++) {
            target[t] |= (1u << ch);
        }
    }

    __dmb();
}

#endif // HAL_RCOUT_DRIVER_ENABLED
