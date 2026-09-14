#include "camera/camera.h"

#include <M5Unified.h>
#include <esp_camera.h>

namespace {

// Выбирает формат кадра по запрошенной ширине.
framesize_t toFrameSize(uint16_t width)
{
    if (width >= 640) return FRAMESIZE_VGA;
    if (width >= 400) return FRAMESIZE_CIF;
    if (width >= 320) return FRAMESIZE_QVGA;
    if (width >= 240) return FRAMESIZE_HQVGA;
    return FRAMESIZE_QQVGA;
}

}  // namespace

bool EspCamera::begin(const Config& config)
{
    if (active_)
    {
        return true;
    }

    if (!M5.Display.width())
    {
        auto cfg = M5.config();
        M5.begin(cfg);
    }
    M5.Power.setExtPower(true);

    // Пины встроенной камеры GC0308 на M5Stack CoreS3.
    camera_config_t cam = {};
    cam.ledc_channel = LEDC_CHANNEL_0;
    cam.ledc_timer = LEDC_TIMER_0;
    cam.pin_d0 = 13;
    cam.pin_d1 = 46;
    cam.pin_d2 = 9;
    cam.pin_d3 = 10;
    cam.pin_d4 = 1;
    cam.pin_d5 = 8;
    cam.pin_d6 = 2;
    cam.pin_d7 = 3;
    cam.pin_xclk = 15;
    cam.pin_pclk = 21;
    cam.pin_vsync = 14;
    cam.pin_href = 47;
    cam.pin_sccb_sda = 11;
    cam.pin_sccb_scl = 12;
    cam.pin_pwdn = -1;
    cam.pin_reset = -1;
    cam.xclk_freq_hz = 20000000;
    cam.pixel_format = PIXFORMAT_RGB565;
    cam.frame_size = toFrameSize(config.width);
    cam.jpeg_quality = 12;
    cam.fb_count = 2;

    const esp_err_t err = esp_camera_init(&cam);
    active_ = (err == ESP_OK);
    Serial.printf("[camera] init err=%d active=%d frame=%d\n",
                  static_cast<int>(err), active_, static_cast<int>(cam.frame_size));
    return active_;
}

bool EspCamera::isActive() const
{
    return active_;
}

bool EspCamera::grabFrame(Frame& frame)
{
    if (!active_)
    {
        return false;
    }
    camera_fb_t* fb = esp_camera_fb_get();
    if (fb == nullptr)
    {
        return false;
    }

    const size_t pixels = static_cast<size_t>(fb->width) * fb->height;
    if (fb->format == PIXFORMAT_RGB565)
    {
        rgbBuffer_.assign(reinterpret_cast<const uint16_t*>(fb->buf),
                          reinterpret_cast<const uint16_t*>(fb->buf) + pixels);
    }
    else
    {
        // Неожиданный формат: считаем данные RGB565 (или копируем как есть).
        rgbBuffer_.assign(reinterpret_cast<const uint16_t*>(fb->buf),
                          reinterpret_cast<const uint16_t*>(fb->buf) + pixels);
    }
    esp_camera_fb_return(fb);

    frame.data = rgbBuffer_.data();
    frame.width = fb->width;
    frame.height = fb->height;
    return true;
}

void EspCamera::end()
{
    if (active_)
    {
        esp_camera_deinit();
        active_ = false;
    }
}