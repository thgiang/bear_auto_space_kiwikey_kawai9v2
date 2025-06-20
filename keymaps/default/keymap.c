#include QMK_KEYBOARD_H
#include "oled_driver.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "anim_bongocat_rle.h"

enum custom_keycodes {
    KC_BPM_TOGGLE = SAFE_RANGE,
    KC_RGB_TOG,
    KC_TURN_ON_OFF
};

static float bpm = 120.0f;
static bool is_loop_running = false; // Theo dõi trạng thái auto-space (có tự động nhấn Space không)
static bool is_timer_counting = false; // Theo dõi trạng thái Timer (có đang đếm không)
static bool is_keyboard_on = true; // Biến trạng thái toàn cục cho biết bàn phím có đang hoạt động không
static uint32_t last_activity_time = 0; // Thời điểm hoạt động cuối cùng
static const uint32_t AUTO_SHUTDOWN_TIMEOUT = 20UL * 60 * 1000; // 30 phút tính bằng milliseconds

volatile static bool space_press_scheduled = false;
volatile static uint32_t current_interval_ticks = 0;

static bool space_is_held = false;
static uint32_t space_count = 0;
static uint32_t qmk_space_press_timestamp = 0;

static uint32_t last_oled_update = 0;
static bool rgb_enabled = true;

static uint16_t SPACE_HOLD_TIME_MS = 60; // Giá trị mặc định

// Hàm lấy số ngẫu nhiên trong khoảng [min, max]
static uint16_t get_random_value(uint16_t min, uint16_t max) {
    return min + (rand() % (max - min + 1));
}

// Hàm cập nhật SPACE_HOLD_TIME_MS
static void update_space_hold_time(void) {
    // Lấy thời gian hiện tại để làm seed cho rand()
    srand(timer_read32());
    
    // Tính toán giá trị mới theo công thức: 60 - bpm/20 + rand(10, 30)
    // Giả sử bpm là 120 (có thể thay đổi tùy theo nhu cầu)
    uint16_t current_bpm_val = (uint16_t)bpm; // Sử dụng giá trị BPM hiện tại
    SPACE_HOLD_TIME_MS = 60 - (current_bpm_val / 20) + get_random_value(10, 30);
    // Đảm bảo SPACE_HOLD_TIME_MS không nhỏ hơn 10ms để tránh các vấn đề về tốc độ polling
    if (SPACE_HOLD_TIME_MS < 10) {
        SPACE_HOLD_TIME_MS = 10;
    }
}


static uint32_t get_interval_us(void) {
    uint32_t interval = (uint32_t)(60000000.0f / bpm * 4.0f);
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

    // Chỉ xóa phần hiển thị BPM
    oled_set_cursor(0, 0);
    oled_write_P(PSTR("BPM "), false);
    char bpm_str[8];
    snprintf(bpm_str, sizeof(bpm_str), "%d.%d", (int)bpm, (int)((bpm - (int)bpm) * 10));
    oled_write(bpm_str, false);
    if (is_loop_running) {
        oled_write_P(PSTR(" ON\n"), false);
    } else {
        oled_write_P(PSTR(" OFF\n"), false);
    }
}

bool oled_task_user(void) {
    static bool first_run = true;
    
    if (first_run && is_keyboard_on) { // Chỉ xóa lần đầu khi bàn phím bật
        oled_clear();
        first_run = false;
    }
    
    if (is_keyboard_on) {
        render_bpm();
        render_bongocat();
    } else {
        oled_clear(); // Xóa màn hình khi bàn phím tắt
        wait_ms(5);
        oled_off();
        first_run = true; // Đặt lại cờ để xóa màn hình khi bật lại
    }
    return false;
}

bool encoder_update_user(uint8_t index, bool clockwise) {
    if (!is_keyboard_on) { // Chỉ xử lý encoder khi bàn phím bật
        return false; // Trả về true để QMK xử lý các encoder khác nếu có
    }

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
    if (!is_keyboard_on) { // Không chạy logic nào khi bàn phím tắt
        return;
    }

    // Kiểm tra auto-shutdown
    if (timer_read32() - last_activity_time > AUTO_SHUTDOWN_TIMEOUT) {
        is_keyboard_on = false;
        rgb_matrix_disable();
        stop_timer_counting();
        is_loop_running = false;
        space_is_held = false;
        oled_clear();
        wait_ms(5);
        oled_off();
        return;
    }

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
        update_space_hold_time();
    }
}

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        KC_NO,          KC_NO,          KC_RGB_TOG,
        KC_SPACE,       KC_UP,          KC_BPM_TOGGLE,
        KC_LEFT,        KC_DOWN,        KC_RIGHT,
        KC_TURN_ON_OFF,
        KC_NO
    )
};

bool process_record_user(uint16_t keycode, keyrecord_t *record) {

    if (record->event.pressed) {
        last_activity_time = timer_read32();
        // Ấn nút bất kì để khởi động
        if (!is_keyboard_on) {
            is_keyboard_on = true;
            if (rgb_enabled) {
                rgb_matrix_enable(); // Bật RGB nếu nó được phép
            }
            oled_on(); // Bật OLED
        }

        switch (keycode) {
            case KC_TURN_ON_OFF:
                is_keyboard_on = !is_keyboard_on; // Chuyển đổi trạng thái bàn phím
                if (!is_keyboard_on) {
                    // Khi tắt bàn phím
                    rgb_matrix_disable(); // Tắt RGB
                    stop_timer_counting(); // Dừng tất cả timer
                    is_loop_running = false; // Đảm bảo auto-space tắt
                    space_is_held = false; // Đảm bảo không giữ Space ảo
                    oled_clear();
                    wait_ms(5);
                    oled_off();
                }
                return false; // Xử lý xong, không gửi keycode này đi

            default:
                if (!is_keyboard_on) { // Không xử lý các keycode khác khi bàn phím tắt
                    return false; // Chặn tất cả các phím khác
                }
                break; // Tiếp tục xử lý các keycode khác nếu bàn phím bật
        }

        // Các keycode khác chỉ được xử lý khi bàn phím bật
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
                        // Tắt cả timer nữa (đã thêm vào trong logic KC_BPM_TOGGLE trước đây, nay di chuyển vào KC_TURN_ON_OFF hoặc xử lý riêng nếu cần)
                        stop_timer_counting(); // Dừng hoàn toàn timer khi tắt BPM_TOGGLE
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
    } else { // record->event.released
        if (!is_keyboard_on) { // Không xử lý khi bàn phím tắt
            return false;
        }
        switch (keycode) {
            case KC_SPACE:
                break;
            // Không cần xử lý KC_TURN_ON_OFF khi nhả phím, vì nó là toggle
        }
    }
    return true;
}

void keyboard_post_init_user(void) {
    is_keyboard_on = true; // Đảm bảo bàn phím bật khi khởi động
    is_loop_running = false;
    is_timer_counting = false;
    space_is_held = false;
    space_count = 0;
    space_press_scheduled = false;
    last_activity_time = timer_read32(); // Khởi tạo thời gian hoạt động
    if (rgb_enabled) {
        rgb_matrix_enable();
    }
    oled_on(); // Đảm bảo OLED bật khi khởi động
}