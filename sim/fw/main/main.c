// Sim addition: minimal ESP-IDF host for LVGL's own demos on an ILI9341 (SPI2) panel with an XPT2046 touch controller, 320x240 landscape.
// ESP32-C3 pins: SCLK 6, MOSI 7, MISO 2, TFT CS 10, DC 4, RST 5, touch CS 3. The demo (widgets / music / benchmark) is chosen at build time.
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "demos/lv_demos.h"

#define PIN_SCLK 6
#define PIN_MOSI 7
#define PIN_CS 10
#define PIN_DC 4
#define PIN_RST 5
#define PIN_MISO 2
#define W 320
#define H 240
#define BUF_LINES 30
#define PIN_TCS 3
static spi_device_handle_t spi;

// Software chip select (the sim mirrors GPIO pad edges to the panel peer, not the SPI block's own CS).
static void tx(int dc, const void *p, size_t n)
{
    spi_transaction_t t = {.length = n * 8, .tx_buffer = p, .user = (void *)(intptr_t)dc};
    gpio_set_level(PIN_CS, 0);
    ESP_ERROR_CHECK(spi_device_transmit(spi, &t));
    gpio_set_level(PIN_CS, 1);
}
static void cmd(uint8_t c, const uint8_t *p, size_t n)
{
    tx(0, &c, 1);
    if (n) tx(1, p, n);
}
static void pre_cb(spi_transaction_t *t) { gpio_set_level(PIN_DC, (int)(intptr_t)t->user); }

static void flush_cb(lv_display_t *d, const lv_area_t *a, uint8_t *px)
{
    size_t n = lv_area_get_width(a) * lv_area_get_height(a);
    lv_draw_sw_rgb565_swap(px, n);
    cmd(0x2A, (uint8_t[]){a->x1 >> 8, a->x1, a->x2 >> 8, a->x2}, 4);
    cmd(0x2B, (uint8_t[]){a->y1 >> 8, a->y1, a->y2 >> 8, a->y2}, 4);
    uint8_t c = 0x2C;
    tx(0, &c, 1);
    tx(1, px, n * 2);
    lv_display_flush_ready(d);
}

static void panel_init(void)
{
    gpio_config_t io = {.pin_bit_mask = (1ULL << PIN_CS) | (1ULL << PIN_TCS) | (1ULL << PIN_DC) | (1ULL << PIN_RST), .mode = GPIO_MODE_OUTPUT};
    gpio_config(&io);
    gpio_set_level(PIN_CS, 1);
    gpio_set_level(PIN_TCS, 1);
    spi_bus_config_t bus = {.sclk_io_num = PIN_SCLK, .mosi_io_num = PIN_MOSI, .miso_io_num = PIN_MISO,
                            .quadwp_io_num = -1, .quadhd_io_num = -1, .max_transfer_sz = W * BUF_LINES * 2};
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));
    spi_device_interface_config_t dev = {.clock_speed_hz = 40 * 1000 * 1000, .mode = 0, .spics_io_num = -1,
                                         .queue_size = 1, .pre_cb = pre_cb};
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev, &spi));
    gpio_set_level(PIN_RST, 0); vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(PIN_RST, 1); vTaskDelay(pdMS_TO_TICKS(120));
    cmd(0x01, NULL, 0);                  // SWRESET
    vTaskDelay(pdMS_TO_TICKS(120));
    cmd(0x11, NULL, 0);                  // SLPOUT
    vTaskDelay(pdMS_TO_TICKS(120));
    cmd(0x3A, (uint8_t[]){0x55}, 1);     // 16 bpp
    cmd(0x36, (uint8_t[]){0x28}, 1);     // MV | BGR: landscape 320x240
    cmd(0x29, NULL, 0);                  // DISPON
}

// XPT2046-style resistive touch controller on the same SPI2 bus, own chip select (GPIO3): 0xB1 = pressure,
// 0xD1 = X, 0x91 = Y, each a 3-byte exchange with the 12-bit result in the last two bytes (bits 14..3).
static int touch_read(uint8_t c)
{
    uint8_t out[3] = {c, 0, 0}, in[3] = {0};
    spi_transaction_t t = {.length = 24, .tx_buffer = out, .rx_buffer = in, .user = (void *)1};
    gpio_set_level(PIN_TCS, 0);
    ESP_ERROR_CHECK(spi_device_transmit(spi, &t));
    gpio_set_level(PIN_TCS, 1);
    return ((in[1] << 8) | in[2]) >> 3;
}
static void indev_cb(lv_indev_t *i, lv_indev_data_t *d)
{
    if (touch_read(0xB1) > 100) {
        d->point.x = touch_read(0xD1) * (W - 1) / 4095;
        d->point.y = touch_read(0x91) * (H - 1) / 4095;
        d->state = LV_INDEV_STATE_PRESSED;
    } else d->state = LV_INDEV_STATE_RELEASED;
}

static uint32_t tick(void) { return (uint32_t)(esp_timer_get_time() / 1000); }

void app_main(void)
{
    panel_init();
    lv_init();
    lv_tick_set_cb(tick);
    lv_display_t *d = lv_display_create(W, H);
    lv_display_set_color_format(d, LV_COLOR_FORMAT_RGB565);
    static uint8_t *b1;
    b1 = heap_caps_malloc(W * BUF_LINES * 2, MALLOC_CAP_DMA);
    lv_display_set_buffers(d, b1, NULL, W * BUF_LINES * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(d, flush_cb);
#if defined(CONFIG_LV_USE_DEMO_BENCHMARK) && defined(DEMO_benchmark)
    lv_demo_benchmark();
#elif defined(DEMO_music)
    lv_demo_music();
#else
    lv_demo_widgets();
#endif
    lv_indev_t *in = lv_indev_create();
    lv_indev_set_type(in, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(in, indev_cb);
    printf("LVGL demo started\n");
    for (;;) {
        uint32_t t = lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(t < 5 ? 5 : t));
    }
}
