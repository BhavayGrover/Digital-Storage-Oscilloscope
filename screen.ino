void setup_screen() {
  tft.init();
  tft.setRotation(1); 
  spr.setColorDepth(8);
  spr.createSprite(160, 128);
  tft.fillScreen(TFT_BLACK);
}

float to_scale(float reading) {
  // --- DSO TRACKING FIXED ---
  // The original formula scaled around the top or skewed with offset.
  // This version centers the raw 20480 baseline exactly at pixel height 64 (the center line).
  float centered_signal = reading - 20480.0;
  
  // Convert offset (in Volts) back to equivalent raw ADC units to shift the baseline cleanly
  float raw_offset = (offset / 3.3) * 4095.0;
  
  // Combine signal and offset, then apply vertical division scaling factor
  float scaled_signal = (centered_signal + raw_offset) * (3300.0 / (v_div * 6.0));
  
  // Draw down from screen center (64) inverted because pixel coordinates grow downwards
  float temp = 64.0 - (scaled_signal * (127.0 / 4095.0));
  
  return temp;
}

float to_voltage(float reading) {
  return (reading - 20480.0) / 4095.0 * 3.3;
}

uint32_t from_voltage(float voltage) {
  return uint32_t(voltage / 3.3 * 4095 + 20480.0);
}

void update_screen(uint16_t *i2s_buff, float sample_rate) {
  float mean = 0;
  float max_v, min_v;
  peak_mean(i2s_buff, BUFF_SIZE, &max_v, &min_v, &mean);

  float freq = 0;
  float period = 0;
  uint32_t trigger0 = 0;
  uint32_t trigger1 = 0;

  bool digital_data = false;
  if (digital_wave_option == 1) {
    trigger_freq_analog(i2s_buff, sample_rate, mean, max_v, min_v, &freq, &period, &trigger0, &trigger1);
  } else if (digital_wave_option == 0) {
    digital_data = digital_analog(i2s_buff, max_v, min_v);
    if (!digital_data) {
      trigger_freq_analog(i2s_buff, sample_rate, mean, max_v, min_v, &freq, &period, &trigger0, &trigger1);
    } else {
      trigger_freq_digital(i2s_buff, sample_rate, mean, max_v, min_v, &freq, &period, &trigger0);
    }
  } else {
    trigger_freq_digital(i2s_buff, sample_rate, mean, max_v, min_v, &freq, &period, &trigger0);
  }

  // --- HOLD FEATURE LOGIC ---
  // If 'stop' is true, skip drawing new paths and freeze the screen
  if (!stop) {
    draw_sprite(freq, period, mean, max_v, min_v, trigger0, sample_rate, digital_data, true);
  } else {
    draw_sprite(freq, period, mean, max_v, min_v, trigger0, sample_rate, digital_data, false);
  }
}

void draw_sprite(float freq, float period, float mean, float max_v, float min_v, uint32_t trigger, float sample_rate, bool digital_data, bool new_data) {
  max_v = to_voltage(max_v);
  min_v = to_voltage(min_v);
  String frequency = (freq < 1000) ? String(freq) + "hz" : (freq < 100000) ? String(freq / 1000) + "khz" : "----";

  if (new_data) {
    spr.fillSprite(TFT_BLACK);
    draw_grid();
    if (auto_scale) {
      auto_scale = false;
      v_div = 1000.0 * max_v / 6.0;
      s_div = period / 3.5;
      if (s_div > 7000 || s_div <= 0) s_div = 7000;
      if (v_div <= 0) v_div = 550;
    }
    if (!(digital_wave_option == 2 && trigger == 0))
      draw_channel1(trigger, 0, i2s_buff, sample_rate);
  }

  if (menu) {
    spr.drawLine(0, 64, 160, 64, TFT_WHITE);
    int shift = 50; 
    spr.fillRect(shift, 0, 110, 128, TFT_BLACK);
    spr.drawRect(shift, 0, 110, 128, TFT_WHITE);
    
    // --- CLEANED VISUAL MENU DISPLAY ---
    // Selector bar matches your reduced options menu (strictly 1 to 4)
    spr.fillRect(shift + 1, 2 + 12 * (opt - 1), 108, 12, TFT_RED);

    spr.drawString("1.V/div:" + String(int(v_div)), shift + 5, 4);
    spr.drawString("2.T/div:" + String(int(s_div)), shift + 5, 16);
    spr.drawString("3.Offs:" + String(offset,1), shift + 5, 28);
    spr.drawString("4.HOLD:" + String(stop ? "ON" : "OFF"), shift + 5, 40);

    spr.drawLine(shift, 95, shift + 110, 95, TFT_WHITE);
    spr.drawString("Vmax:" + String(max_v, 1) + "V", shift + 5, 98);
    spr.drawString("Vmin:" + String(min_v, 1) + "V", shift + 5, 108);
    spr.drawString("Avg:" + String(mean, 1) + "V", shift + 5, 118);
  } else if (info) {
    spr.drawLine(0, 64, 160, 64, TFT_WHITE);
    spr.drawString("P-P: " + String(max_v - min_v, 2) + "V", 5, 5);
    spr.drawString(frequency, 5, 15);
    
    // Bottom labels exactly at the edge
    spr.drawString(String(int(v_div)) + "mV/div", 5, 118);
    spr.drawString(String(int(s_div)) + "uS/div", 90, 118);
    if (stop) {
      spr.drawString("HOLD", 55, 5);
    }
  }
  spr.pushSprite(0, 0);
}

void draw_grid() {
  for (int i = 0; i < 17; i++) {
    spr.drawPixel(i * 10, 32, TFT_WHITE);
    spr.drawPixel(i * 10, 64, TFT_WHITE);
    spr.drawPixel(i * 10, 96, TFT_WHITE);
  }
  for (int i = 0; i < 128; i += 10) {
    for (int j = 0; j < 160; j += 40) {
      spr.drawPixel(j, i, TFT_WHITE);
    }
  }
}

void draw_channel1(uint32_t trigger0, uint32_t trigger1, uint16_t *i2s_buff, float sample_rate) {
  float data_per_pixel = (s_div / 40.0) / (sample_rate / 1000);
  uint32_t index_offset = (uint32_t)(toffset / data_per_pixel);
  trigger0 += index_offset;  
  
  float n_data = 0, o_data = to_scale(i2s_buff[trigger0]);
  for (uint32_t i = 1; i < 160; i++) { 
    uint32_t index = trigger0 + (uint32_t)((i + 1) * data_per_pixel);
    if (index < BUFF_SIZE) {
        n_data = to_scale(i2s_buff[index]);
        spr.drawLine(i - 1, o_data, i, n_data, TFT_BLUE);
        o_data = n_data;
    } else { break; }
  }
}