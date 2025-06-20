#include QMK_KEYBOARD_H
#include "keychron_common.h"
#include "quantum.h"
#include <math.h> 
#include "send_string.h"
#include <stdarg.h>
#include <stdio.h>

enum layers {
    MAC_BASE,
    MAC_FN,
    WIN_BASE,
    WIN_FN,
};

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [MAC_BASE] = LAYOUT_ansi_108(
        KC_ESC,          KC_BRID,  KC_BRIU,  KC_MCTRL, KC_LNPAD, RGB_VAD,  RGB_VAI,  KC_MPRV,  KC_MPLY,  KC_MNXT,  KC_MUTE,  KC_VOLD,     KC_VOLU,     KC_SNAP,  KC_SIRI,  RGB_MOD,  KC_F13,   KC_F14,   KC_F15,   KC_F16,
        KC_GRV,   KC_1,    KC_2,     KC_3,     KC_4,     KC_5,     KC_6,     KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,  KC_EQL,      KC_BSPC,     KC_INS,   KC_HOME,  KC_PGUP,  KC_NUM,   KC_PSLS,  KC_PAST,  KC_PMNS,
        KC_TAB,   KC_Q,    KC_W,     KC_E,     KC_R,     KC_T,     KC_Y,     KC_U,     KC_I,     KC_O,     KC_P,     KC_LBRC,  KC_RBRC,     KC_BSLS,     KC_DEL,   KC_END,   KC_PGDN,  KC_P7,    KC_P8,    KC_P9,
        KC_CAPS,  KC_A,    KC_S,     KC_D,     KC_F,     KC_G,     KC_H,     KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,                           KC_ENT,                                                        KC_P4,    KC_P5,    KC_P6,    KC_PPLS,
        KC_LSFT,           KC_Z,    KC_X,     KC_C,     KC_V,     KC_B,     KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,                           KC_RSFT,              KC_UP,              KC_P1,    KC_P2,    KC_P3,
        KC_LCTL,  KC_LOPTN, KC_LCMMD,                                         KC_SPC,                                       KC_RCMMD, KC_ROPTN, MO(MAC_FN), KC_RCTL,     KC_LEFT,  KC_DOWN,  KC_RGHT,  KC_P0,               KC_PDOT,  KC_PENT),

    [MAC_FN] = LAYOUT_ansi_108(
        _______,           KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,    KC_F7,    KC_F8,    KC_F9,    KC_F10,   KC_F11,      KC_F12,      _______,  _______,  RGB_TOG,  _______,  _______,  _______,  _______,
        _______,  BT_HST1,  BT_HST2,  BT_HST3,  P2P4G,    _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,     _______,     _______,  _______,  _______,  _______,  _______,  _______,  _______,
        RGB_TOG,  RGB_MOD,  RGB_VAI,  RGB_HUI,  RGB_SAI,  RGB_SPI,  _______,  _______,  _______,  _______,  _______,  _______,  _______,     _______,     _______,  _______,  _______,  _______,  _______,  _______,
        _______,  RGB_RMOD, RGB_VAD,  RGB_HUD,  RGB_SAD,  RGB_SPD,  _______,  _______,  _______,  _______,  _______,  _______,                         _______,                                                        _______,  _______,  _______,  _______,
        _______,           _______,  _______,  _______,  _______,  BAT_LVL,  NK_TOGG,  _______,  _______,  _______,  _______,                           _______,              _______,              _______,  _______,  _______,
        _______,  _______,  _______,                                          _______,                                        _______,  _______,  _______,     _______,     _______,  _______,  _______,  _______,               _______,  _______),

    [WIN_BASE] = LAYOUT_ansi_108(
        KC_ESC,          KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,    KC_F7,    KC_F8,    KC_F9,    KC_F10,   KC_F11,      KC_F12,      KC_PSCR,  KC_CTANA, RGB_MOD,  _______,  _______,  _______,  _______,
        KC_GRV,   KC_1,    KC_2,     KC_3,     KC_4,     KC_5,     KC_6,     KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,  KC_EQL,      KC_BSPC,     KC_INS,   KC_HOME,  KC_PGUP,  KC_NUM,   KC_PSLS,  KC_PAST,  KC_PMNS,
        KC_TAB,   KC_Q,    KC_W,     KC_E,     KC_R,     KC_T,     KC_Y,     KC_U,     KC_I,     KC_O,     KC_P,     KC_LBRC,  KC_RBRC,     KC_BSLS,     KC_DEL,   KC_END,   KC_PGDN,  KC_P7,    KC_P8,    KC_P9,
        KC_CAPS,  KC_A,    KC_S,     KC_D,     KC_F,     KC_G,     KC_H,     KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,                           KC_ENT,                                                        KC_P4,    KC_P5,    KC_P6,    KC_PPLS,
        KC_LSFT,           KC_Z,    KC_X,     KC_C,     KC_V,     KC_B,     KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,                           KC_RSFT,              KC_UP,              KC_P1,    KC_P2,    KC_P3,
        KC_LCTL,  KC_LWIN,  KC_LALT,                                          KC_SPC,                                       KC_RALT,  KC_RWIN,  MO(WIN_FN), KC_RCTL,     KC_LEFT,  KC_DOWN,  KC_RGHT,  KC_P0,               KC_PDOT,  KC_PENT),

    [WIN_FN] = LAYOUT_ansi_108(
        _______,           KC_BRID,  KC_BRIU,  KC_TASK,  KC_FILE,  RGB_VAD,  RGB_VAI,  KC_MPRV,  KC_MPLY,  KC_MNXT,  KC_MUTE,  KC_VOLD,     KC_VOLU,     _______,  _______,  RGB_TOG,  _______,  _______,  _______,  _______,
        _______,  BT_HST1,  BT_HST2,  BT_HST3,  P2P4G,    _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,     _______,     _______,  _______,  _______,  _______,  _______,  _______,  _______,
        RGB_TOG,  RGB_MOD,  RGB_VAI,  RGB_HUI,  RGB_SAI,  RGB_SPI,  _______,  _______,  _______,  _______,  _______,  _______,  _______,     _______,     _______,  _______,  _______,  _______,  _______,  _______,
        _______,  RGB_RMOD, RGB_VAD,  RGB_HUD,  RGB_SAD,  RGB_SPD,  _______,  _______,  _______,  _______,  _______,  _______,                         _______,                                                        _______,  _______,  _______,  _______,
        _______,           _______,  _______,  _______,  _______,  BAT_LVL,  NK_TOGG,  _______,  _______,  _______,  _______,                           _______,              _______,              _______,  _______,  _______,
        _______,  _______,  _______,                                          _______,                                        _______,  _______,  _______,     _______,     _______,  _______,  _______,  _______,               _______,  _______)
}; 

bool turning_on_auto = false;
int bpmGoc = 125;
int bpmHienTai = 125;
float thoi_gian_moi_nhip = 1920.0f;
float thoi_diem_per = 0.0f;
float thoi_diem_per_tiep_theo = 0.0f;
int count = 1;
bool tryingToPer = false;
bool shift_held = false;
int arrow_left_count = 0.0f;
int arrow_up_count = 0;
bool space_tap_pending = false;
uint16_t space_timer = 0;
bool shift_tap_pending = false;
uint16_t shift_timer = 0;
int random_delay = 60;
bool co_chinh_nhac = false;
#define DOUBLE_SHIFT_DELAY 400
// #define DEBUG

void matrix_scan_user(void) {
 if (turning_on_auto) {
        if (timer_read32() - thoi_diem_per_tiep_theo >= thoi_gian_moi_nhip) {
            if (tryingToPer) 
            {
                register_code(KC_SPC);
                random_delay = rand() % 30 + 60;
                wait_ms(random_delay);        
                unregister_code(KC_SPC);
                #ifdef DEBUG
                    send_string("Per");
                #endif
            } 
            tryingToPer = false;
            count += 1;
            thoi_diem_per_tiep_theo = thoi_diem_per + (count * 60000.0f * 4.0f / bpmHienTai);
            
        }
    } 
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) { 
    
    if (!process_record_keychron_common(keycode, record)) {
        return false;
    }
    switch (keycode) {
        case KC_ESC:
            if (record->event.pressed) {
                turning_on_auto = false;
            }
            return true;
        case KC_LSFT: 
            if (record->event.pressed) { 
                if (shift_tap_pending && timer_elapsed(shift_timer) < DOUBLE_SHIFT_DELAY) {
                    shift_tap_pending = false;
                    turning_on_auto = !turning_on_auto;
                    #ifdef DEBUG
                        if (turning_on_auto) {
                            send_string("Bat");
                        } else {
                            send_string("Tat"); 
                        } 
                    #endif 
                } else {
                    shift_tap_pending = true;
                    shift_timer = timer_read();
                }
                shift_held = true;
            } else {
                shift_held = false;
                if (shift_tap_pending && timer_elapsed(shift_timer) >= DOUBLE_SHIFT_DELAY) {
                    shift_tap_pending = false;
                }
                // Nếu có nhấn mũi tên trong khi giữ shift
                if (co_chinh_nhac) {
                    // Lưu trạng thái Shift hiện tại và vô hiệu hóa nó tạm thời bởi vì nếu vẫn giữ shift mà in chữ send_string ra thì nó biến thành kí tự đặc biệt trên màn hình
                    uint8_t original_mods = get_mods();
                    del_mods(MOD_MASK_SHIFT); // Xóa trạng thái shift

                    bpmHienTai = bpmGoc + arrow_left_count * 10 + arrow_up_count;
                    thoi_gian_moi_nhip = 60000.0f / bpmHienTai * 4.0f; // Chắc chắn là phép tính float
                    #ifdef DEBUG
                        // Gửi thông tin debug
                        char buffer[32];
                        snprintf(buffer, sizeof(buffer), "%d", bpmHienTai);
                        send_string(buffer);
                        // send_string(" ---  ");
                        // int phan_nguyen = (int)thoi_gian_moi_nhip;
                        // int phan_thap_phan = (int)((thoi_gian_moi_nhip - phan_nguyen) * 1000); // Lấy 3 chữ số thập phân
                        // snprintf(buffer, sizeof(buffer), "%d.%03d", phan_nguyen, phan_thap_phan);
                        // send_string(buffer);
                    #endif
                    // Khôi phục trạng thái Shift ban đầu
                    set_mods(original_mods);

                    arrow_left_count = 0;
                    arrow_up_count = 0;
                    co_chinh_nhac = false; // Đặt lại cờ chỉnh nhịp
                }
            }
            return true; // Xử lý Shift ở đây

        case KC_LEFT:
            if (record->event.pressed && shift_held) {
                co_chinh_nhac = true; // Đặt cờ để biết có chỉnh nhịp
                arrow_left_count--;
            }
            return true; // Luôn cho phép arrow key hoạt động bình thường
        case KC_RIGHT:
            if (record->event.pressed && shift_held) {
                co_chinh_nhac = true; // Đặt cờ để biết có chỉnh nhịp
                arrow_left_count++;
            }
            return true; // Luôn cho phép arrow key hoạt động bình thường

        case KC_UP:
            if (record->event.pressed && shift_held) {
                co_chinh_nhac = true; // Đặt cờ để biết có chỉnh nhịp
                arrow_up_count++;
            }
            return true; // Luôn cho phép arrow key hoạt động bình thường
        case KC_DOWN:
            if (record->event.pressed && shift_held) {
                co_chinh_nhac = true; // Đặt cờ để biết có chỉnh nhịp
                arrow_up_count--;
            }
            return true; // Luôn cho phép arrow key hoạt động bình thường

        case KC_SPC: // Space chỉ cập nhật thoi_diem_per 
            if (record->event.pressed) {
                if (turning_on_auto) {
                    // Nếu space sớm 200ms thì delay lại chờ đúng thời điểm mới nhả ở hàm matrix_scan_user
                    if (timer_read32() - thoi_diem_per_tiep_theo > thoi_gian_moi_nhip - 200) {
                        tryingToPer = true;
                        return false;
                    }
                } else {
                    thoi_diem_per = timer_read32();
                    count = 1; // Đặt lại count về 1 khi bắt đầu một chu kỳ mới
                    thoi_diem_per_tiep_theo = thoi_diem_per + (count * 60000.0f * 4.0f / bpmHienTai);
                    return true;
                }
            }
            break; // Thêm break để thoát khỏi switch case
    }
    return true; // Trả về true cho các keycode khác để chúng được xử lý bình thường
}