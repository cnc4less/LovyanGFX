#ifndef LOVYANGFX_CONFIG_HPP_
#define LOVYANGFX_CONFIG_HPP_

namespace lgfx
{
  struct LGFX_Config
  {
    static constexpr spi_host_device_t spi_host = VSPI_HOST;
    static constexpr int dma_channel = 1;
    static constexpr int spi_mosi = 23;
    static constexpr int spi_miso = 19;
    static constexpr int spi_sclk = 18;
  };

  struct Panel_Dummy : public Panel_ST7735S
  {
    Panel_Dummy() {
      spi_3wire  = true;
      freq_read  = 8000000;
      panel_width  = 0;
      panel_height = 0;
    }
  };

  struct Panel_M5StickC : public Panel_ST7735S
  {
    Panel_M5StickC() {
      spi_3wire  = true;
      invert     = true;
      spi_cs     =  5;
      spi_dc     = 23;
      gpio_rst   = 18;
      panel_width  = 80;
      panel_height = 160;
      offset_x     = 26;
      offset_y     = 1;
      offset_rotation = 2;
    }
  protected:
    const std::uint8_t* getInitCommands(std::uint8_t listno) const override {
      static constexpr std::uint8_t list[] = {
          CMD::GAMMASET, 1, 0x08,  // Gamma set, curve 4
          0xFF,0xFF, // end
      };
      if (listno == 2)  return list;
      return Panel_ST7735S::getInitCommands(listno);
    }
  };

  struct Panel_M5StickCPlus: public Panel_ST7789
  {
    Panel_M5StickCPlus() {
      spi_3wire  = true;
      invert     = true;
      spi_cs     =  5;
      spi_dc     = 23;
      gpio_rst   = 18;
      freq_write = 80000000;
      freq_read  = 16000000;
      freq_fill  = 80000000;
      spi_mode_read = 1;
      len_dummy_read_pixel = 16;
      panel_width  = 135;
      panel_height = 240;
      offset_x = 52;
      offset_y = 40;
    }
  };

  struct Panel_M5Stack : public Panel_ILI9342
  {
    bool isIPS = false;

    Panel_M5Stack(void) {
      spi_3wire = true;
      spi_cs = 14;
      spi_dc = 27;
      rotation = 1;
      offset_rotation = 3;
      gpio_rst = 33;
      gpio_bl  = 32;
      pwm_ch_bl = 7;
    }

    void init(void) override {
      gpio_lo(gpio_rst);
      lgfxPinMode(gpio_rst, pin_mode_t::input);
      delay(1);
      isIPS = gpio_in(gpio_rst);       // get panel type (IPS or TN)

      Panel_ILI9342::init();
    }

  protected:

    const std::uint8_t* getInvertDisplayCommands(std::uint8_t* buf, bool invert) override {
      if (!isIPS) return Panel_ILI9342::getInvertDisplayCommands(buf, invert);
      this->invert = invert;
      buf[2] = buf[0] = invert ? CommandCommon::INVOFF : CommandCommon::INVON;
      buf[3] = buf[1] = 0;
      buf[4] = CMD::GAMMASET;
      buf[5] = 1;
    //buf[6] = 0x08;  // Gamma set, curve 8
    //buf[6] = 0x04;  // Gamma set, curve 4
      buf[6] = 0x02;  // Gamma set, curve 2
    //buf[6] = 0x01;  // Gamma set, curve 1
      buf[8] = buf[7] = 0xFF;
      return buf;
    }
  };

  struct Panel_TTGO_TWatch : public Panel_ST7789
  {
    Panel_TTGO_TWatch() : Panel_ST7789() {
      freq_write = 80000000;
      freq_read  = 20000000;
      freq_fill  = 80000000;
      panel_height = 240;
      invert    = true;
      spi_3wire = true;
      spi_cs    =  5;
      spi_dc    = 27;
      gpio_bl   = 12;
      pwm_ch_bl = 7;
    }
  };

//*/
}

class LGFX : public lgfx::LGFX_SPI<lgfx::LGFX_Config>
{
public:
  LGFX(void) : lgfx::LGFX_SPI<lgfx::LGFX_Config>()
  {
  }

  enum board_t
  { board_unknown
  , board_M5Stack
  , board_M5StickC
  , board_M5StickCPlus
  , board_TTGO_TWatch
  };

  board_t getBoard(void) const { return board; }

  void init(void) override
  {
    static lgfx::Panel_Dummy panel_dummy;
    board = board_unknown;

    releaseBus();
    panel_dummy.spi_cs =  5;
    panel_dummy.spi_dc = 27;
    setPanel(&panel_dummy);
    _spi_mosi = 19;
    _spi_miso = -1;
    _spi_sclk = 18;
    initBus();
    delay(10);

    auto id = readPanelID();
    ESP_LOGI("LovyanGFX", "[Autodetect] panel id:%08x", id);
    if ((id & 0xFF) == 0x85) {  //  check panel (ST7789)
      ESP_LOGI("LovyanGFX", "[Autodetect] TWatch");
      board = board_TTGO_TWatch;
      static lgfx::Panel_TTGO_TWatch panel;
      setPanel(&panel);
      initPanel();
      return;
    }

    releaseBus();
    lgfx::gpio_lo(panel_dummy.spi_dc);
    lgfx::gpio_lo(panel_dummy.spi_cs);

    panel_dummy.spi_cs = 14;
    panel_dummy.spi_dc = 27;
    setPanel(&panel_dummy);
    _spi_mosi = 23;
    _spi_miso = 19;
    _spi_sclk = 18;
    initBus();
    delay(10);

    id = readPanelID();
    ESP_LOGI("LovyanGFX", "[Autodetect] panel id:%08x", id);
    if (id != 0 && id != ~0) {   // M5Stack
      ESP_LOGI("LovyanGFX", "[Autodetect] M5Stack");
      board = board_M5Stack;
      static lgfx::Panel_M5Stack panel;
      setPanel(&panel);
      initPanel();
      return;
    }

    releaseBus();
    lgfx::gpio_lo(panel_dummy.spi_dc);
    lgfx::gpio_lo(panel_dummy.spi_cs);

    panel_dummy.spi_cs =  5;
    panel_dummy.spi_dc = 23;
    panel_dummy.gpio_rst = 18;
    setPanel(&panel_dummy);
    _spi_mosi = 15;
    _spi_miso = 14;
    _spi_sclk = 13;
    initBus();
    lgfx::lgfxPinMode(panel_dummy.gpio_rst, lgfx::pin_mode_t::output);
    lgfx::gpio_lo(panel_dummy.gpio_rst);
    delay(1);
    lgfx::gpio_hi(panel_dummy.gpio_rst);
    delay(10);

    id = readPanelID();
    ESP_LOGI("LovyanGFX", "[Autodetect] panel id:%08x", id);
    if ((id & 0xFF) == 0x85) {  //  check panel (ST7789)
      ESP_LOGI("LovyanGFX", "[Autodetect] M5StickCPlus");
      board = board_M5StickCPlus;
      static lgfx::Panel_M5StickCPlus panel;
      setPanel(&panel);

      initPanel();
      return;
    }

    if ((id & 0xFF) == 0x7C) {  //  check panel (ST7735)
      ESP_LOGI("LovyanGFX", "[Autodetect] M5StickC");
      board = board_M5StickC;
      static lgfx::Panel_M5StickC panel;
      setPanel(&panel);

      initPanel();
      return;
    }

    releaseBus();
    lgfx::gpio_lo(panel_dummy.spi_dc);
    lgfx::gpio_lo(panel_dummy.spi_cs);

    ESP_LOGI("LovyanGFX", "[Autodetect] detect fail.");
  }
private:
  board_t board = board_unknown;
};

#endif
