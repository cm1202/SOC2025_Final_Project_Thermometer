/*****************************************************************//**
 * @file main_sampler_test.cpp
 *
 * @brief Basic test of nexys4 ddr mmio cores
 *
 * @author p chu
 * @version v1.0: initial release
 *********************************************************************/

// #define _DEBUG
#include "adsr_core.h"
#include "chu_init.h"
#include "ddfs_core.h"
#include "gpio_cores.h"
#include "i2c_core.h"
#include "ps2_core.h"
#include "sseg_core.h"
#include "spi_core.h"
#include "xadc_core.h"

GpoCore led(get_slot_addr(BRIDGE_BASE, S2_LED));
GpiCore sw(get_slot_addr(BRIDGE_BASE, S3_SW));
DebounceCore btn(get_slot_addr(BRIDGE_BASE, S7_BTN));
SsegCore sseg(get_slot_addr(BRIDGE_BASE, S8_SSEG));
I2cCore adt7420(get_slot_addr(BRIDGE_BASE, S10_I2C));
PwmCore pwm(get_slot_addr(BRIDGE_BASE, S6_PWM));
/**
 * Class definition of Interface
 */
enum class Interface {
   IDLE,
   LIVE,
   DIFF,

};

/**
 * Determine the current menu selection based on button presses.
 */
Interface check_btn(DebounceCore *db_p, GpoCore *led_p, bool &RST_BTN, bool &LIVE_BTN, bool &Z_BTN) {
   int s = db_p->read();
   Interface c_menu = Interface::IDLE;
   if (s == 1) {
      RST_BTN = true;
      LIVE_BTN = false;
      Z_BTN = false;
      c_menu = Interface::IDLE;
   } else if (s == 16) {
      LIVE_BTN = true;
      RST_BTN = false;
      Z_BTN = false;
      c_menu = Interface::LIVE;
   } else if (s == 4) {
      Z_BTN = true;
      RST_BTN = false;
      LIVE_BTN = false;
      c_menu = Interface::DIFF;
   }
   return c_menu;
}

/**
 * Track additional button presses while in the DIFF menu.
 */
void check_btn_r(DebounceCore *db_p, bool &AVG_BTN, bool &DIFF_BTN) {
   int s = db_p->read();
   if (s == 2) {
      AVG_BTN = true;
   } else if (s == 8) {
      DIFF_BTN = true;
   }
}


/** Clear every digit on the seven-segment display. */
void sseg_clear(SsegCore *sseg_t) {
   for (int i = 0; i < 9; i++) {
      sseg_t->write_1ptn(0xFF, i);
   }
}

/** Show "RST" to indicate the DIFF menu. */
void disp_RST(SsegCore *sseg_t) {
   sseg_t->set_dp(0x00);
   sseg_t->write_1ptn(0xAF, 2);  // R (small r)
   sseg_t->write_1ptn(0x92, 1);  // S (looks like 5)
   sseg_t->write_1ptn(0x87, 0);  // T (small t)
}

/** Show "AVG" to indicate the averaging option. */
void disp_AVG(SsegCore *sseg_t) {
   sseg_t->set_dp(0x00);
   sseg_t->write_1ptn(0x88, 2);  // A
   sseg_t->write_1ptn(0xC1, 1);  // V (looks like U)
   sseg_t->write_1ptn(0x90, 0);  // G (looks like 9/g)
}

/** Show "DIFF" to indicate the difference display. */
void disp_DIFF(SsegCore *sseg_t) {
   sseg_t->set_dp(0x00);
   sseg_t->write_1ptn(0xA1, 3);  // D (small d)
   sseg_t->write_1ptn(0xF9, 2);  // I (looks like 1)
   sseg_t->write_1ptn(0x8E, 1);  // F
   sseg_t->write_1ptn(0x8E, 0);  // F
}

/** Show "LIVE" to indicate live temperature mode. */
void disp_LIVE(SsegCore *sseg_t) {
   sseg_t->set_dp(0x00);
   sseg_t->write_1ptn(0xC7, 3);  // L
   sseg_t->write_1ptn(0xF9, 2);  // I
   sseg_t->write_1ptn(0xC1, 1);  // V (uses U pattern)
   sseg_t->write_1ptn(0x86, 0);  // E
}



/**
 * Display a temperature difference on the seven-segment display and drive the
 * PWM outputs based on direction.
 */
void temp_diff(float temp, SsegCore *sseg_t, PwmCore *pwm_p) {
   sseg_t->set_dp(0x00);

   bool is_negative = false;
   if (temp < 0.0) {
      is_negative = true;
      temp = -temp;
   }

   int temp_input = static_cast<int>(temp * 1000.0 + 0.5);
   int temp_array[5];
   temp_array[0] = (temp_input / 10000) % 10;
   temp_array[1] = (temp_input / 1000) % 10;
   temp_array[2] = (temp_input / 100) % 10;
   temp_array[3] = (temp_input / 10) % 10;
   temp_array[4] = (temp_input / 1) % 10;

   sseg_t->write_1ptn(sseg_t->h2s(12), 0);
   sseg_t->set_dp(0x10);

   sseg_t->write_1ptn(sseg_t->h2s(temp_array[4]), 1);
   sseg_t->write_1ptn(sseg_t->h2s(temp_array[3]), 2);
   sseg_t->write_1ptn(sseg_t->h2s(temp_array[2]), 3);
   sseg_t->write_1ptn(sseg_t->h2s(temp_array[1]), 4);

   if (is_negative) {
      sseg_t->write_1ptn(0xBF, 5);  // Dash
      pwm_p->set_duty(1023, 0);
      pwm_p->set_duty(0, 1);
      pwm_p->set_duty(0, 2);
   } else if (temp == 0) {
      pwm_p->set_duty(0, 0);
      pwm_p->set_duty(1, 1);
      pwm_p->set_duty(0, 2);
   } else {
      pwm_p->set_duty(0, 0);
      pwm_p->set_duty(0, 1);
      pwm_p->set_duty(1023, 2);

      if (temp_array[0] == 0) {
         sseg_t->write_1ptn(0xFF, 5);
      } else {
         sseg_t->write_1ptn(sseg_t->h2s(temp_array[0]), 5);
      }
   }
}

/** Display a temperature reading in Celsius. */
void temp_disp_C(float temp, SsegCore *sseg_t) {
   sseg_t->set_dp(0x00);
   int temp_input = static_cast<int>(temp * 1000.0);
   int temp_array[5];
   temp_array[0] = (temp_input / 10000) % 10;
   temp_array[1] = (temp_input / 1000) % 10;
   temp_array[2] = (temp_input / 100) % 10;
   temp_array[3] = (temp_input / 10) % 10;
   temp_array[4] = (temp_input / 1) % 10;

   sseg_t->write_1ptn(sseg_t->h2s(12), 0);
   sseg_t->set_dp(0x10);

   sseg_t->write_1ptn(sseg_t->h2s(temp_array[0]), 5);
   sseg_t->write_1ptn(sseg_t->h2s(temp_array[1]), 4);
   sseg_t->write_1ptn(sseg_t->h2s(temp_array[2]), 3);
   sseg_t->write_1ptn(sseg_t->h2s(temp_array[3]), 2);
   sseg_t->write_1ptn(sseg_t->h2s(temp_array[4]), 1);
}

/** Display a temperature reading in Fahrenheit. */
void temp_disp_F(float temp, SsegCore *sseg_t) {
   sseg_t->set_dp(0x00);
   int temp_input = static_cast<int>(temp * 1000.0);
   int temp_array[5];
   temp_array[0] = (temp_input / 10000) % 10;
   temp_array[1] = (temp_input / 1000) % 10;
   temp_array[2] = (temp_input / 100) % 10;
   temp_array[3] = (temp_input / 10) % 10;
   temp_array[4] = (temp_input / 1) % 10;

   sseg_t->write_1ptn(sseg_t->h2s(15), 0);
   sseg_t->set_dp(0x10);

   sseg_t->write_1ptn(sseg_t->h2s(temp_array[0]), 5);
   sseg_t->write_1ptn(sseg_t->h2s(temp_array[1]), 4);
   sseg_t->write_1ptn(sseg_t->h2s(temp_array[2]), 3);
   sseg_t->write_1ptn(sseg_t->h2s(temp_array[3]), 2);
   sseg_t->write_1ptn(sseg_t->h2s(temp_array[4]), 1);
}



/**
 * Read the ADT7420 temperature sensor and return the temperature in Celsius.
 */
float adt7420_read(I2cCore *adt7420_p) {
   const uint8_t DEV_ADDR = 0x4b;
   uint8_t wbytes[2], bytes[2];
   uint16_t tmp;
   float tmpC;

   wbytes[0] = 0x0b;
   adt7420_p->write_transaction(DEV_ADDR, wbytes, 1, 1);
   adt7420_p->read_transaction(DEV_ADDR, bytes, 1, 0);
   uart.disp("read ADT7420 id (should be 0xcb): ");
   uart.disp(bytes[0], 16);
   uart.disp("\n\r");

   wbytes[0] = 0x00;
   adt7420_p->write_transaction(DEV_ADDR, wbytes, 1, 1);
   adt7420_p->read_transaction(DEV_ADDR, bytes, 2, 0);

   tmp = (uint16_t)bytes[0];
   tmp = (tmp << 8) + (uint16_t)bytes[1];
   if (tmp & 0x8000) {
      tmp = tmp >> 3;
      tmpC = (float)((int)tmp - 8192) / 16;
   } else {
      tmp = tmp >> 3;
      tmpC = (float)tmp / 16;
   }

   uart.disp(tmpC);
   uart.disp("\n\r");

   return tmpC;
}

/**
 * Read the ADT7420 and display either Celsius or Fahrenheit based on switches.
 */
void adt7420_check(I2cCore *adt7420_p, GpoCore *led_p, SsegCore *sseg_t,
                   GpiCore *sw_p) {
   const uint8_t DEV_ADDR = 0x4b;
   uint8_t wbytes[2], bytes[2];
   uint16_t tmp;
   float tmpC;
   float tmpF;
   int s;
   s = sw_p->read();

   wbytes[0] = 0x0b;
   adt7420_p->write_transaction(DEV_ADDR, wbytes, 1, 1);
   adt7420_p->read_transaction(DEV_ADDR, bytes, 1, 0);

   wbytes[0] = 0x00;
   adt7420_p->write_transaction(DEV_ADDR, wbytes, 1, 1);
   adt7420_p->read_transaction(DEV_ADDR, bytes, 2, 0);

   tmp = (uint16_t)bytes[0];
   tmp = (tmp << 8) + (uint16_t)bytes[1];
   if (tmp & 0x8000) {
      tmp = tmp >> 3;
      tmpC = (float)((int)tmp - 8192) / 16;
   } else {
      tmp = tmp >> 3;
      tmpC = (float)tmp / 16;
   }
   if (s == 1) {
      tmpF = (tmpC * 9 / 5) + 32;
      temp_disp_F(tmpF, sseg_t);
   } else {
      temp_disp_C(tmpC, sseg_t);
   }
}

/** Take multiple readings and return the average Celsius temperature. */
float measure_avg_tmp() {
   float total_tmp = 0;
   float avg_tmp;
   for (int j = 0; j < 30; j++) {
      total_tmp += adt7420_read(&adt7420);
   }
   avg_tmp = total_tmp / 30;

   uart.disp("\n\r");
   uart.disp("total temp: ");
   uart.disp(total_tmp);
   uart.disp("\n\r");
   uart.disp("avg_tmp: ");
   uart.disp(avg_tmp);
   uart.disp("\n\r");
   sleep_ms(1000);
   return avg_tmp;
}


/** Simple sweep animation for LEDs. */
void led_animation(GpoCore *led_p) {
   for (int i = 0; i < 4; i++) {
      led_p->write(1, i);
      sleep_ms(100);
   }
   for (int i = 3; i > -1; i--) {
      led_p->write(0, i);
      sleep_ms(100);
   }
}

/** Full LED flash used during DIFF. */
void led_animation_rst(GpoCore *led_p) {
   for (int i = 0; i < 4; i++) {
      led_p->write(1, i);
   }
   sleep_ms(1000);
   for (int i = 3; i > -1; i--) {
      led_p->write(0, i);
   }
}


int main() {
   bool RST_BTN = false;
   bool LIVE_BTN = false;
   bool Z_BTN = false;
   bool AVG_BTN = false;
   bool DIFF_BTN = false;
   float saved_tmp;

   while (1) {
      Interface currentState = check_btn(&btn, &led, RST_BTN, LIVE_BTN, Z_BTN);
      switch (currentState) {
         case Interface::IDLE:
            disp_RST(&sseg);
            led_animation_rst(&led);
            sseg_clear(&sseg);
            saved_tmp = 0;
            DIFF_BTN = 0;
            led.write(0, 0);
            led.write(0, 1);
            led.write(0, 2);
            led.write(0, 3);
            check_btn(&btn, &led, RST_BTN, LIVE_BTN, Z_BTN);
            break;

         case Interface::LIVE:
            sseg_clear(&sseg);
            disp_LIVE(&sseg);
            sleep_ms(2000);
            while (LIVE_BTN == 1) {
               adt7420_check(&adt7420, &led, &sseg, &sw);
               check_btn(&btn, &led, RST_BTN, LIVE_BTN, Z_BTN);
               if (LIVE_BTN == 0) {
                  sseg_clear(&sseg);
                  break;
               }
            }
            break;

         case Interface::DIFF:
            disp_AVG(&sseg);
            while (Z_BTN == 1) {
               check_btn_r(&btn, AVG_BTN, DIFF_BTN);
               if (AVG_BTN) {
                  sseg_clear(&sseg);
                  disp_AVG(&sseg);
                  led_animation(&led);
                  sleep_ms(1000);
                  sseg_clear(&sseg);

                  float tmp = measure_avg_tmp();
                  temp_disp_C(tmp, &sseg);
                  saved_tmp = tmp;

                  sleep_ms(500);
                  AVG_BTN = false;
               }

               if (DIFF_BTN) {
                  sseg_clear(&sseg);
                  disp_DIFF(&sseg);
                  sleep_ms(2000);
                  sseg_clear(&sseg);
               }
               while (DIFF_BTN) {
                  float current = adt7420_read(&adt7420);
                  float difference = saved_tmp - current;
                  uart.disp("stored : ");
                  uart.disp(saved_tmp);
                  uart.disp("\n\r");
                  uart.disp("current : ");
                  uart.disp(current);
                  uart.disp("\n\r");
                  uart.disp("difference : ");
                  uart.disp(difference);
                  uart.disp("\n\r");
                  temp_diff(difference, &sseg, &pwm);
                  sleep_ms(500);
                  check_btn(&btn, &led, RST_BTN, LIVE_BTN, Z_BTN);
                  if (RST_BTN == 1 || LIVE_BTN == 1) {
                     pwm.set_duty(0, 0);
                     pwm.set_duty(0, 1);
                     pwm.set_duty(0, 2);
                     DIFF_BTN = false;
                  }
               }

               check_btn(&btn, &led, RST_BTN, LIVE_BTN, Z_BTN);
            }
            if (Z_BTN == 0) {
               sseg_clear(&sseg);
               break;
            }
            break;

         default:
            break;
      }
   }
}
