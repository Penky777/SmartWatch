// ---- calendar_icon_drawer.c ----
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"

extern const unsigned char gImage_calendar[];   // your blob
static const char *TAG = "ICON";

// Read little-endian u16 from byte ptr
static inline uint16_t rd16(const uint8_t *p){ return (uint16_t)p[0] | ((uint16_t)p[1] << 8); }

// Draw a 1bpp bitmap (MSB->LSB) into a temporary RGB565 buffer and flush
static esp_err_t draw_mono_to_panel(esp_lcd_panel_handle_t panel,
                                    int x, int y, int w, int h,
                                    const uint8_t *bits, uint16_t fg565)
{
    // One row consumes ceil(w/8) bytes
    int row_bytes = (w + 7) >> 3;
    size_t pix_cnt = (size_t)w * h;
    size_t buf_bytes = pix_cnt * 2;
    // Small chunk buffer (2 rows) to save RAM
    int chunk_rows = 2;
    uint16_t *linebuf = heap_caps_malloc((size_t)w * chunk_rows * 2, MALLOC_CAP_DMA);
    if (!linebuf) {
        ESP_LOGE(TAG, "mono: no mem");
        return ESP_ERR_NO_MEM;
    }

    for (int row = 0; row < h; row += chunk_rows) {
        int rows_now = ((row + chunk_rows) <= h) ? chunk_rows : (h - row);
        // Build RGB565 for these rows
        for (int rr = 0; rr < rows_now; rr++) {
            const uint8_t *src = bits + (row + rr) * row_bytes;
            uint16_t *dst = linebuf + rr * w;
            int bit_idx = 0;
            uint8_t byte = src[0];
            for (int col = 0; col < w; col++) {
                if ((bit_idx & 7) == 0 && (bit_idx != 0)) {
                    src++;
                    byte = *src;
                }
                // MSB first: test bit 7..0
                int bit = 7 - (bit_idx & 7);
                *dst++ = (byte & (1 << bit)) ? fg565 : 0x0000;  // transparent as black; change if you want bg
                bit_idx++;
            }
        }
        // Flush this chunk
        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel,
                          x, y + row, x + w, y + row + rows_now,
                          linebuf));
    }

    heap_caps_free(linebuf);
    return ESP_OK;
}

// Draw raw RGB565 block directly
static esp_err_t draw_rgb565_to_panel(esp_lcd_panel_handle_t panel,
                                      int x, int y, int w, int h,
                                      const uint8_t *pix565 /*2*w*h bytes*/)
{
    // If the blob is not DMA-capable, copy by small strips
    const int strip_rows = 40;
    size_t strip_bytes = (size_t)w * strip_rows * 2;
    uint16_t *dmabuf = heap_caps_malloc(strip_bytes, MALLOC_CAP_DMA);
    if (!dmabuf) return ESP_ERR_NO_MEM;

    int done_rows = 0;
    const uint8_t *src = pix565;
    while (done_rows < h) {
        int nrows = (h - done_rows > strip_rows) ? strip_rows : (h - done_rows);
        size_t nbytes = (size_t)w * nrows * 2;
        memcpy(dmabuf, src, nbytes);
        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel,
                          x, y + done_rows, x + w, y + done_rows + nrows,
                          dmabuf));
        src += nbytes;
        done_rows += nrows;
    }
    heap_caps_free(dmabuf);
    return ESP_OK;
}

// Public helper: parse + draw your gImage_calendar[]
esp_err_t draw_calendar_icon(esp_lcd_panel_handle_t panel, int x, int y, uint16_t fg565 /*for mono*/)
{
    const uint8_t *p = (const uint8_t *)gImage_calendar;
    size_t total = 6966; // if you know it at compile time; else pass in
    if (total < 6) {
        ESP_LOGE(TAG, "blob too small");
        return ESP_ERR_INVALID_SIZE;
    }

    // Try a common 6-byte header: [bpp_lo bpp_hi][w_lo w_hi][h_lo h_hi]
    uint16_t bpp = rd16(p + 0);
    uint16_t w   = rd16(p + 2);
    uint16_t h   = rd16(p + 4);
    const uint8_t *payload = p + 6;
    size_t payload_len = total - 6;

    // Sanity fallback: if w/h look crazy, assume no header and you’ll set w/h manually
    if (w == 0 || h == 0 || w > 320 || h > 320) {
        // Guess a typical icon size (adjust if needed)
        w = 240; h = 240;
        bpp = 1;               // assume mono
        payload = p;
        payload_len = total;
    }

    // If payload = w*h*2 → RGB565
    if (payload_len == (size_t)w * h * 2) {
        ESP_LOGI(TAG, "calendar: RGB565 %ux%u", w, h);
        return draw_rgb565_to_panel(panel, x, y, w, h, payload);
    }

    // Else assume 1bpp packed (MSB first), rows padded to bytes
    size_t need_mono = ((w + 7) >> 3) * (size_t)h;
    if (payload_len >= need_mono) {
        ESP_LOGI(TAG, "calendar: MONO %ux%u (1bpp)");
        return draw_mono_to_panel(panel, x, y, w, h, payload, fg565);
    }

    ESP_LOGE(TAG, "calendar: unrecognized format (w=%u h=%u bpp=%u payload=%u)", w, h, bpp, (unsigned)payload_len);
    return ESP_ERR_INVALID_ARG;
}
