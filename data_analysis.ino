void peak_mean(uint16_t *i2s_buffer, uint32_t len, float * max_value, float * min_value, float *pt_mean) {
  max_value[0] = i2s_buffer[0];
  min_value[0] = i2s_buffer[0];
  mean_filter filter(5);
  filter.init(i2s_buffer[0]);

  float mean = 0;
  for (uint32_t i = 1; i < len; i++) {
    float value = filter.filter((float)i2s_buffer[i]);
    if (value > max_value[0]) max_value[0] = value;
    if (value < min_value[0]) min_value[0] = value;
    mean += i2s_buffer[i];
  }
  mean /= float(BUFF_SIZE);
  // Using your screen.ino's original scale factor
  pt_mean[0] = (mean - 20480.0) / 4095.0 * 3.3; 
}

bool digital_analog(uint16_t *i2s_buffer, uint32_t max_v, uint32_t min_v) {
  uint32_t upper_threshold = max_v - 0.05 * (max_v - min_v);
  uint32_t lower_threshold = min_v + 0.05 * (max_v - min_v);
  uint32_t digital_data = 0;
  uint32_t analog_data = 0;
  for (uint32_t i = 0; i < BUFF_SIZE; i++) {
    if (i2s_buffer[i] > lower_threshold && i2s_buffer[i] < upper_threshold) analog_data++;
    else digital_data++;
  }
  return (analog_data < digital_data);
}

void trigger_freq_analog(uint16_t *i2s_buffer, float sample_rate, float mean, uint32_t max_v, uint32_t min_v, float *pt_freq, float *pt_period, uint32_t *pt_trigger0, uint32_t *pt_trigger1) {
  float freq = 0;
  float period = 0;
  bool signal_side = false;
  uint32_t trigger_count = 0;
  uint32_t trigger_num = 10;
  uint32_t trigger_temp[10] = {0};
  uint32_t trigger_index = 0;

  // FIXED CENTER matched to your screen.ino (2048 * 10 = 20480)
  uint32_t wave_center = 20480; 

  if (i2s_buffer[0] > wave_center) signal_side = true;

  for (uint32_t i = 1 ; i < BUFF_SIZE; i++) {
    // Hysteresis of 1000 (which is 100 scaled by 10)
    if (signal_side && i2s_buffer[i] < (wave_center - 1000)) {
      signal_side = false;
    }
    else if (!signal_side && i2s_buffer[i] > (wave_center + 1000)) {
      freq++;
      if (trigger_count < trigger_num) {
        trigger_temp[trigger_count] = i;
        trigger_count++;
      }
      signal_side = true;
    }
  }

  if (trigger_count >= 2) {
    freq = freq * 1000 / 50;
    period = (float)(sample_rate * 1000.0) / freq; 
    if (freq < 2000 && freq > 80) {
      period = 0;
      for (uint32_t i = 1; i < trigger_count; i++) period += trigger_temp[i] - trigger_temp[i - 1];
      period /= (trigger_count - 1);
      freq = sample_rate * 1000 / period;
    } else if (freq <= 80) {
      period = trigger_temp[1] - trigger_temp[0];
      freq = sample_rate * 1000 / period;
    }
  }

  uint32_t trigger2 = 0;
  if (trigger_count > 1) {
    trigger_index = trigger_temp[0];
    trigger2 = trigger_temp[1];
  }

  pt_trigger0[0] = trigger_index;
  pt_trigger1[0] = trigger2;
  pt_freq[0] = freq;
  pt_period[0] = period;
}

void trigger_freq_digital(uint16_t *i2s_buffer, float sample_rate, float mean, uint32_t max_v, uint32_t min_v, float *pt_freq, float *pt_period, uint32_t *pt_trigger0) {
  float freq = 0;
  float period = 0;
  bool signal_side = false;
  uint32_t trigger_count = 0;
  uint32_t trigger_temp[10] = {0};

  uint32_t wave_center = 20480; 
  if (i2s_buffer[0] > wave_center) signal_side = true;

  bool normal_high = (mean > 1.65) ? true : false;
  
  for (uint32_t i = 1 ; i < BUFF_SIZE; i++) {
    if (signal_side && i2s_buffer[i] < (wave_center - 2000)) {
      if (trigger_count < 10 && normal_high) { trigger_temp[trigger_count] = i; trigger_count++; }
      signal_side = false;
    }
    else if (!signal_side && i2s_buffer[i] > (wave_center + 2000)) {
      freq++;
      if (trigger_count < 10 && !normal_high) { trigger_temp[trigger_count] = i; trigger_count++; }
      signal_side = true;
    }
  }
  
  if (trigger_count > 1) {
    freq = freq * 1000 / 50;
    period = (float)(sample_rate * 1000.0) / freq;
    if (freq < 2000 && freq > 80) {
      period = 0;
      for (uint32_t i = 1; i < trigger_count; i++) period += trigger_temp[i] - trigger_temp[i - 1];
      period /= (trigger_count - 1);
      freq = sample_rate * 1000 / period;
    } else if (freq <= 80) {
      period = trigger_temp[1] - trigger_temp[0];
      freq = sample_rate * 1000 / period;
    }
  }

  pt_trigger0[0] = (trigger_count > 0) ? trigger_temp[0] : 0;        
  pt_freq[0] = freq;
  pt_period[0] = period;
}