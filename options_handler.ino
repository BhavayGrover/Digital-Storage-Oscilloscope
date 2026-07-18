int voltage_division[6] = { //screen has 4 divisions, 31 pixels each (125 pixels of height)
  550, //fullscreen 3.3V peak-peak
  500,
  375,
  180,
  100,
  50
};


/*each sample represents 1us (1Msps),
   thus, the time division is the number
   of samples per screen division
*/
float time_division[9] = { //screen has 4 divisions, 60 pixel each (240 pixel of width)
  10,
  25,
  50,
  100,
  250,
  500,
  1000,
  2500,
  5000
};

void menu_handler() {
  button();
}

void button() {
  if ( btnok == 1 || btnbk == 1 || btnpl == 1 || btnmn == 1)
  {
    menu_action = true;
  }
  if (menu == true)
  {
    if (set_value) {
      switch (opt) {
        case Vdiv:
          if (btnpl == 1) {
            volts_index++;
            if (volts_index >= sizeof(voltage_division) / sizeof(*voltage_division)) {
              volts_index = 0;
            }
            btnpl = 0;
          }
          else if (btnmn == 1) {
            volts_index--;
            if (volts_index < 0) {
              volts_index = sizeof(voltage_division) / sizeof(*voltage_division) - 1;
            }
            btnmn = 0;
          }

          v_div = voltage_division[volts_index];
          break;

        case Sdiv:
          if (btnmn == 1) {
            tscale_index++;
            if (tscale_index >= sizeof(time_division) / sizeof(*time_division)) {
              tscale_index = 0;
            }
            btnmn = 0;
          }
          else if (btnpl == 1) {
            tscale_index--;
            if (tscale_index < 0) {
              tscale_index = sizeof(time_division) / sizeof(*time_division) - 1;
            }
            btnpl = 0;
          }

          s_div = time_division[tscale_index];
          break;

        case Offset:
          if (btnmn == 1) {
            offset += 0.1 * (v_div * 4) / 3300;
            btnmn = 0;
          }
          else if (btnpl == 1) {
            offset -= 0.1 * (v_div * 4) / 3300;
            btnpl = 0;
          }

          if (offset > 3.3)
            offset = 3.3;
          if (offset < -3.3)
            offset = -3.3;

          break;

        default:
          break;

      }
      if (btnbk == 1)
      {
        set_value = 0;
        btnbk = 0;
      }
    }
    else
    {
      if (btnpl == 1)
      {
        opt++;
        // Limit menu navigation strictly to your retained options
        if (opt > Stop) 
        {
          opt = 1;
        }
        Serial.print("option : ");
        Serial.println(opt);
        btnpl = 0;
      }
      if (btnmn == 1)
      {
        opt--;
        if (opt < 1)
        {
          opt = Stop; // Wrap directly back to the HOLD option
        }
        Serial.print("option : ");
        Serial.println(opt);
        btnmn = 0;
      }
      if (btnbk == 1)
      {
        hide_menu();
        btnbk = 0;
      }
      if (btnok == 1) {
        switch (opt) {
          case Vdiv:
            set_value = true;
            break;

          case Sdiv:
            set_value = true;
            break;

          case Offset:
            set_value = true;
            break;

          case Stop: // This functions as your 1-click ON/OFF HOLD feature
            stop = !stop; 
            set_value = false;
            break;

          default:
            break;

        }

        btnok = 0;
      }
    }
  }
  else
  {
    if (btnok == 1)
    {
      opt = 1;
      show_menu();
      btnok = 0;
    }
    if (btnbk == 1)
    {
      if (info == true)
      {
        hide_all();
      }
      else
      {
        info = true;
      }
      btnbk = 0;
    }
    if (btnpl == 1) {
      volts_index++;
      if (volts_index >= sizeof(voltage_division) / sizeof(*voltage_division)) {
        volts_index = 0;
      }
      btnpl = 0;
      v_div = voltage_division[volts_index];
    }
    if (btnmn == 1) {
      tscale_index++;
      if (tscale_index >= sizeof(time_division) / sizeof(*time_division)) {
        tscale_index = 0;
      }
      btnmn = 0;
      s_div = time_division[tscale_index];
    }
  }

}

void hide_menu() {
  menu = false;
}

void hide_all() {
  menu = false;
  info = false;
}

void show_menu() {
  menu = true;
}

String strings_vdiv() {
  return "";
}

String strings_sdiv() {
  return "";
}

String strings_offset() {
  return "";
}

String strings_toffset() {
  return "";
}

String strings_freq() {
  return "";
}

String strings_peak() {
  return "";
}

String strings_vmax() {
  return "";
}

String strings_vmin() {
  return "";
}

String strings_filter() {
  return "";
}