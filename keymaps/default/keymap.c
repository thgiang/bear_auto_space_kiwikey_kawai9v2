#include QMK_KEYBOARD_H
#include "oled_driver.h"
#include <avr/io.h>
#include <avr/interrupt.h>

enum custom_keycodes {
    KC_BPM_TOGGLE = SAFE_RANGE,
    KC_RGB_TOG,
};

static float bpm = 120.0f;
static bool is_loop_running = false; // Theo dõi trạng thái auto-space (có tự động nhấn Space không)
static bool is_timer_counting = false; // Theo dõi trạng thái Timer (có đang đếm không)

volatile static bool space_press_scheduled = false;
volatile static uint32_t current_interval_ticks = 0;

static bool space_is_held = false;
static uint32_t space_count = 0;
static uint32_t qmk_space_press_timestamp = 0;

static uint32_t last_oled_update = 0;
static bool rgb_enabled = true;

#define SPACE_HOLD_TIME_MS 100

static uint32_t get_interval_us(void) {
    uint32_t interval = (uint32_t)(60000000.0f / bpm * 4.0f);
    if (interval < 50000) {
        interval = 50000;
    }
    return interval;
}

static uint32_t get_ticks_for_us(uint32_t us) {
    return (uint32_t)(((float)us * F_CPU) / (1024.0f * 1000000.0f));
}

void setup_timer1_for_bpm(uint32_t interval_us) {
    cli();

    TCCR1A = 0;
    TCCR1B = 0;
    TCCR1C = 0;
    TCNT1 = 0;

    TCCR1B |= (1 << WGM12);

    current_interval_ticks = get_ticks_for_us(interval_us);
    if (current_interval_ticks > 0xFFFF) current_interval_ticks = 0xFFFF;
    OCR1A = (uint16_t)current_interval_ticks;

    TIMSK1 |= (1 << OCIE1A);

    sei();
}

void start_timer_counting(void) {
    TCCR1B |= (1 << CS10) | (1 << CS12); // Prescaler 1024
    TCNT1 = 0;
    space_count = 0; // Reset count khi timer bắt đầu đếm
    is_timer_counting = true;
}

void stop_timer_counting(void) {
    TCCR1B &= ~((1 << CS10) | (1 << CS11) | (1 << CS12)); // Dừng Timer
    TCNT1 = 0;
    is_timer_counting = false;
    space_press_scheduled = false; // Đảm bảo cờ được reset
    // Đảm bảo phím Space ảo được nhả nếu đang giữ
    if (space_is_held) {
        unregister_code(KC_SPACE);
        space_is_held = false;
    }
}

// ISR chỉ kích hoạt cờ space_press_scheduled
ISR(TIMER1_COMPA_vect) {
    // Nếu is_loop_running là true, thì ISR này sẽ kích hoạt việc nhấn phím.
    // Nếu chỉ đếm (is_timer_counting = true nhưng is_loop_running = false),
    // thì cờ space_press_scheduled sẽ được set nhưng không được dùng để nhấn phím.
    // Tức là, cờ này chỉ có tác dụng khi is_loop_running = true.
    // Tuy nhiên, để ISR luôn chính xác, ta cứ set cờ, và logic auto-space sẽ kiểm tra `is_loop_running`
    // trong `matrix_scan_user`.
    space_press_scheduled = true;
}

static void render_bpm(void) {
    uint32_t current_time = timer_read32();

    if (current_time - last_oled_update < 100) {
        return;
    }
    last_oled_update = current_time;

    oled_clear();
    oled_write_P(PSTR("BPM: "), false);
    char bpm_str[8];
    snprintf(bpm_str, sizeof(bpm_str), "%d.%d", (int)bpm, (int)((bpm - (int)bpm) * 10));
    oled_write(bpm_str, false);
    oled_write_P(PSTR("\n"), false);

    oled_write_P(PSTR("Space: "), false);
    if (is_loop_running) {
        oled_write_P(PSTR("ON\n"), false);
    } else {
        oled_write_P(PSTR("OFF\n"), false);
    }

    oled_write_P(PSTR("Count: "), false);
    char count_str[10];
    sprintf(count_str, "%lu", space_count);
    oled_write(count_str, false);


    oled_write_P(PSTR("\n<3 BEARPATCH.NET <3"), false);
}

bool oled_task_user(void) {
    render_bpm();
    return false;
}

bool encoder_update_user(uint8_t index, bool clockwise) {
    if (index == 0) {
        if (clockwise && bpm < 300.0f) {
            bpm += 0.5f;
        } else if (!clockwise && bpm > 60.0f) {
            bpm -= 0.5f;
        }
        // Luôn cấu hình lại timer với BPM mới ngay lập tức
        setup_timer1_for_bpm(get_interval_us());
        // Nếu timer đang đếm, khởi động lại để áp dụng BPM mới ngay
        if (is_timer_counting) {
            start_timer_counting();
        }
        return false;
    }
    return true;
}

void matrix_scan_user(void) {
    ATOMIC_BLOCK_RESTORESTATE {
        // Chỉ nhấn phím Space nếu is_loop_running VÀ cờ được đặt bởi ISR
        if (is_loop_running && space_press_scheduled) {
            if (!space_is_held) {
                register_code(KC_SPACE);
                space_is_held = true;
                space_count++;
                qmk_space_press_timestamp = timer_read32();
            }
            space_press_scheduled = false;
        }
    }

    if (space_is_held && (timer_read32() - qmk_space_press_timestamp >= SPACE_HOLD_TIME_MS)) {
        unregister_code(KC_SPACE);
        space_is_held = false;
    }
}

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        KC_NO,         KC_NO,         KC_RGB_TOG,
        KC_SPACE,      KC_UP,         KC_BPM_TOGGLE,
        KC_LEFT,       KC_DOWN,       KC_RIGHT,
        KC_NO,
        KC_NO
    )
};

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        switch (keycode) {
            case KC_SPACE:
                // Dừng và khởi động lại timer để lấy điểm bắt đầu mới
                stop_timer_counting();
                setup_timer1_for_bpm(get_interval_us());
                start_timer_counting(); // Bắt đầu đếm
                return true;

            case KC_BPM_TOGGLE:
                is_loop_running = !is_loop_running; // Bật/tắt chế độ auto-space
                if (is_loop_running) {
                    // Nếu bật auto-space và timer chưa đếm, bắt đầu đếm
                    if (!is_timer_counting) {
                        setup_timer1_for_bpm(get_interval_us());
                        start_timer_counting();
                    }
                    // Nếu timer đã đếm, auto-space sẽ bắt đầu theo nhịp hiện tại của timer
                } else {
                    // Nếu tắt auto-space, dừng việc nhấn phím ảo
                    // KHÔNG dừng timer counting, chỉ dừng việc nhấn phím
                    space_press_scheduled = false; // Đảm bảo cờ được reset
                    if (space_is_held) {
                        unregister_code(KC_SPACE);
                        space_is_held = false;
                        // Tắt cả timer nữa
                        stop_timer_counting();

                    }
                }
                return false;

            case KC_RGB_TOG:
                rgb_enabled = !rgb_enabled;
                if (rgb_enabled) {
                    rgb_matrix_enable();
                } else {
                    rgb_matrix_disable();
                }
                return false;
        }
    } else {
        switch (keycode) {
            case KC_SPACE:
                break;
        }
    }
    return true;
}

void keyboard_post_init_user(void) {
    is_loop_running = false;
    is_timer_counting = false;
    space_is_held = false;
    space_count = 0;
    space_press_scheduled = false;
    if (rgb_enabled) {
        rgb_matrix_enable();
    }
}