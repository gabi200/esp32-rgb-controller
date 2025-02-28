#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <driver/i2s.h>

// Mic config
#define SAMPLE_BUFFER_SIZE 512
#define SAMPLE_RATE 8000

#define I2S_MIC_CHANNEL I2S_CHANNEL_FMT_ONLY_LEFT

#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_26
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_22
#define I2S_MIC_SERIAL_DATA GPIO_NUM_21

i2s_config_t i2s_config = {
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
  .sample_rate = SAMPLE_RATE,
  .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
  .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
  .communication_format = I2S_COMM_FORMAT_I2S,
  .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
  .dma_buf_count = 4,
  .dma_buf_len = 1024,
  .use_apll = false,
  .tx_desc_auto_clear = false,
  .fixed_mclk = 0
};

i2s_pin_config_t i2s_mic_pins = {
  .bck_io_num = I2S_MIC_SERIAL_CLOCK,
  .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
  .data_out_num = I2S_PIN_NO_CHANGE,
  .data_in_num = I2S_MIC_SERIAL_DATA
};

// CONFIG START

const char* ssid = "REPLACE_WITH_YOUR_SSID";          
const char* password = "REPLACE_WITH_YOUR_PASSWORD";           

const int redPin = 13;
const int greenPin = 12;
const int bluePin = 14;
const int pir_pin = 18;
const int ldr_pin = 35;

// CONFIG END

// GLOBAL VARIABLES START

AsyncWebServer server(80);

struct sys_cfg {
  int r, g, b, mode, timeout;
  bool motion_detect, auto_off, auto_bright, indicate_time, sound_reaction, keep_color, power_sw;
} config;

unsigned long last_trig = 0;
unsigned long last_time_brightness = 0;
unsigned long last_time_mode = 0;
bool start_timer = false;
int mode_delay_ms = 1000;
int mode_stage = 0;

const int freq = 5000;
const int redChannel = 0;
const int greenChannel = 1;
const int blueChannel = 2;
const int resolution = 8;

// GLOBAL VARIABLES END

// HELPER FUNCTIONS START

void auto_off_check() {
  if (read_ldr() < 3) {
    config.power_sw = false;
  } else if (read_ldr() > 10) {
    config.power_sw = true;
  }
  power_control();
}

void set_auto_brightness() {
  int new_r, new_g, new_b;
  float brightness_coeff = (float)read_ldr() / (float)50;

  new_r = (int)(config.r * brightness_coeff);
  new_g = (int)(config.g * brightness_coeff);
  new_b = (int)(config.b * brightness_coeff);

  ledcWrite(redPin, new_r);
  ledcWrite(greenPin, new_g);
  ledcWrite(bluePin, new_b);

  last_time_brightness = millis();
}

void update_rgb() {
  ledcWrite(redPin, config.r);
  ledcWrite(greenPin, config.g);
  ledcWrite(bluePin, config.b);
}

void power_control() {
  if (config.power_sw == false) {
    ledcWrite(redPin, 0);
    ledcWrite(greenPin, 0);
    ledcWrite(bluePin, 0);
  } else {
    if (!config.auto_bright) {
      update_rgb();
    } else {
      set_auto_brightness();
    }
  }
}
void IRAM_ATTR movement_detect() {
  if (config.motion_detect) {
    last_trig = millis();
    start_timer = true;

    Serial.println("Movement detected");

    if (config.power_sw == false) {
      config.power_sw = true;
      power_control();
    }
  }
}

int read_ldr() {
  return map(adc1_get_raw(ADC1_CHANNEL_7), 4096, 0, 0, 100);
}

void read_mic() {
  int32_t raw_samples[SAMPLE_BUFFER_SIZE];
  size_t bytes_read = 0;

  i2s_read(I2S_NUM_0, raw_samples, sizeof(int32_t) * SAMPLE_BUFFER_SIZE, &bytes_read, portMAX_DELAY);
  int samples_read = bytes_read / sizeof(int32_t);

  for (int i = 0; i < samples_read; i++) {
    //const float fun_constant = 2.735;
    const float fun_constant = 3.69420;

    uint32_t sample = abs(raw_samples[i]);
    int sample_scaled = map(sample / 100000, 0, 1000, 0, 255);

    config.r = sample_scaled * fun_constant;
    if (sample_scaled % 2 == 0) {
      config.g = sample_scaled;
    } else {
      config.b = sample_scaled;
    }

    update_rgb();
  }
}

String placeholder_processor(const String& var) {
  if (var == "R") {
    return String(config.r);
  } else if (var == "G") {
    return String(config.g);
  } else if (var == "B") {
    return String(config.b);
  } else if (var == "CURRENT_PATTERN") {
    return String(config.mode);
  } else if (var == "KEEP_CURRENT_COLOR_TRUE") {
    if (config.keep_color) {
      return "checked";
    }
  } else if (var == "RANDOM_COLOR_TRUE") {
    if (!config.keep_color) {
      return "checked";
    }
  } else if (var == "MOTION_DETECT_CHECKED") {
    if (config.motion_detect) {
      return "checked";
    }
  } else if (var == "TIMEOUT_VAL") {
    return String(config.timeout);
  }
  if (var == "AUTO_OFF_CHECKED") {
    if (config.auto_off) {
      return "checked";
    }
  } else if (var == "AUTO_BRIGHT_CHECKED") {
    if (config.auto_bright) {
      return "checked";
    }
  } else if (var == "TIME_PASSING_CHECKED") {
    if (config.indicate_time) {
      return "checked";
    }
  } else if (var == "SOUND_REACT_CHECKED") {
    if (config.sound_reaction) {
      return "checked";
    }
  }

  return String();
}

// HELPER FUNCTIONS END

// MAIN PROGRAM

void setup() {
  Serial.begin(115200);

  ledcAttachChannel(redPin, freq, resolution, redChannel);
  ledcAttachChannel(greenPin, freq, resolution, greenChannel);
  ledcAttachChannel(bluePin, freq, resolution, blueChannel);

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &i2s_mic_pins);

  // initial settings
  config.r = 0;
  config.g = 0;
  config.b = 0;
  config.mode = 0;
  config.timeout = 30;
  config.power_sw = false;
  config.motion_detect = false;
  config.auto_off = false;
  config.auto_bright = false;
  config.indicate_time = false;
  config.sound_reaction = false;
  config.keep_color = false;

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  pinMode(pir_pin, INPUT);
  pinMode(ldr_pin, INPUT);

  attachInterrupt(digitalPinToInterrupt(pir_pin), movement_detect, RISING);

  // Handle HTTP requests

  // handle index request
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    int params_nr = request->params();
    String cfg_id, cfg_val;
    bool settings_changed = false;

    for (int i = 0; i < params_nr; i++) {
      const AsyncWebParameter* p = request->getParam(i);

      if (p->name() == "r") {
        config.r = p->value().toInt();
      } else if (p->name() == "g") {
        config.g = p->value().toInt();
      } else if (p->name() == "b") {
        config.b = p->value().toInt();
      } else if (p->name() == "mode") {
        config.mode = p->value().toInt();
      } else if (p->name() == "cfg_id") {
        cfg_id = p->value();
        settings_changed = true;
      } else if (p->name() == "cfg_val") {
        cfg_val = p->value();
      } else if (p->name() == "timeout") {
        config.timeout = p->value().toInt();
      }
    }

    if (config.power_sw) {
      if (config.auto_bright) {
        set_auto_brightness();
      } else {
        update_rgb();
      }
    }

    if (settings_changed) {
      if (cfg_id == "keep_color") {
        config.keep_color = true;
      } else if (cfg_id == "random_color") {
        config.keep_color = false;
      } else if (cfg_id == "motion_detect") {
        if (cfg_val == "true") {
          config.motion_detect = true;
        } else {
          config.motion_detect = false;
        }
      } else if (cfg_id == "light_detect") {
        if (cfg_val == "true") {
          config.auto_off = true;
        } else {
          config.auto_off = false;
        }
      } else if (cfg_id == "auto_brightness") {
        if (cfg_val == "true") {
          config.auto_bright = true;
          set_auto_brightness();
        } else {
          config.auto_bright = false;
          update_rgb();
        }
      } else if (cfg_id == "signal_time") {
        if (cfg_val == "true") {
          config.indicate_time = true;
        } else {
          config.indicate_time = false;
        }
      } else if (cfg_id == "sound_react") {
        if (cfg_val == "true") {
          config.sound_reaction = true;
        } else {
          config.sound_reaction = false;
        }
      } else if (cfg_id == "power_sw") {
        if (config.power_sw == false) {
          config.power_sw = true;
        } else {
          config.power_sw = false;
        }
        power_control();
      }
    }

    request->send(SPIFFS, "/index.html", String(), false, placeholder_processor);
  });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, "/script.js", "text/javascript", false, placeholder_processor);
  });

  server.begin();
}

void loop() {
  if (config.sound_reaction) {
    read_mic();
  } else {
    if (start_timer && (millis() - last_trig > (config.timeout * 1000))) {
      config.power_sw = false;
      power_control();
      start_timer = false;
      Serial.println("Movement timer done");
    }

    if (millis() - last_time_brightness > 5000) {
      if (config.auto_bright) {
        set_auto_brightness();
      }
    }

    if (config.auto_off) {
      auto_off_check();
    }
  }
}
