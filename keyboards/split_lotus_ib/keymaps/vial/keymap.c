#include QMK_KEYBOARD_H
#include <stdlib.h>

/* ------------------------------------------------------------------
 * レイヤー定義
 * ------------------------------------------------------------------ */
enum layers {
    L_BASE = 0,
    L_ALT,
    L_NUM,
    L_SYM,
    L_MOUSE,
    L_SPARE5,
    L_PRECISE,
    L_SPARE7,
};

/* ------------------------------------------------------------------
 * カスタムキーコード
 * ------------------------------------------------------------------ */
enum custom_keycodes {
    CPI_UP = QK_KB_0, 
    CPI_DN,           
    SCRL_UP,          
    SCRL_DN,          
};

/* ------------------------------------------------------------------
 * 調整値・定数定義
 * ------------------------------------------------------------------ */
#define AUTO_LAYER_TIMEOUT_MS 800        
#define AUTO_LAYER_THRESHOLD 4           
#define AUTO_LAYER_ACTIVITY_RESET_MS 100 
#define AUTO_LAYER_LOCKOUT_MS 300        

#define SCROLL_INVERT_H 0 
#define SCROLL_INVERT_V 0 

#define PRECISION_AXIS_DECIDE 3       
#define PRECISION_AXIS_RELEASE_MS 150 

#define ACCEL_WINDOW_MS 8 

/* ------------------------------------------------------------------
 * 設定構造体と永続化処理
 * ------------------------------------------------------------------ */
#define SLOT_COUNT 7
#define CFG_VERSION 1

typedef struct __attribute__((packed)) {
    uint8_t  version;
    uint8_t  cpi_index;        
    uint8_t  scroll_index;     
    uint8_t  accel_enable;     
    uint8_t  accel_curve;      
    uint8_t  precision_div;    
    uint8_t  precision_lock;   
    uint8_t  reserved;
    uint16_t cpi[SLOT_COUNT];  
    uint8_t  scroll_div[SLOT_COUNT]; 
    uint8_t  pad;
    uint16_t accel_max_gain;   
    uint16_t accel_low_speed;  
    uint16_t accel_high_speed; 
} trackball_cfg_t;

_Static_assert(sizeof(trackball_cfg_t) <= EECONFIG_USER_DATA_SIZE, "EECONFIG_USER_DATA_SIZE is too small");

static trackball_cfg_t cfg;

static void cfg_defaults(void) {
    const trackball_cfg_t d = {
        .version          = CFG_VERSION,
        .cpi_index        = 1,
        .scroll_index     = 3,
        .accel_enable     = 1,
        .accel_curve      = 20,
        .precision_div    = 4,
        .precision_lock   = 1,
        .cpi              = {608, 800, 1000, 1200, 1600, 2000, 2400},
        .scroll_div       = {24, 16, 13, 10, 8, 6, 4},
        .accel_max_gain   = 200,
        .accel_low_speed  = 500,
        .accel_high_speed = 3500,
    };
    cfg = d;
}

static void cfg_save(void) {
    eeconfig_update_user_datablock(&cfg, 0, sizeof(cfg));
}

static void cfg_sanitize(void) {
    if (cfg.cpi_index >= SLOT_COUNT) cfg.cpi_index = 1;
    if (cfg.scroll_index >= SLOT_COUNT) cfg.scroll_index = 3;
    if (cfg.accel_curve < 10) cfg.accel_curve = 10;
    if (cfg.accel_curve > 50) cfg.accel_curve = 50;
    if (cfg.precision_div < 1) cfg.precision_div = 1;
    if (cfg.precision_div > 16) cfg.precision_div = 16;
    if (cfg.accel_max_gain < 100) cfg.accel_max_gain = 100;
    if (cfg.accel_max_gain > 500) cfg.accel_max_gain = 500;
    if (cfg.accel_high_speed <= cfg.accel_low_speed) cfg.accel_high_speed = cfg.accel_low_speed + 1;
    for (uint8_t i = 0; i < SLOT_COUNT; i++) {
        if (cfg.cpi[i] < 608) cfg.cpi[i] = 608;
        if (cfg.cpi[i] > 4800) cfg.cpi[i] = 4800;
        if (cfg.scroll_div[i] < 1) cfg.scroll_div[i] = 1;
        if (cfg.scroll_div[i] > 40) cfg.scroll_div[i] = 40;
    }
}

static void cfg_load(void) {
    trackball_cfg_t tmp;
    if (eeconfig_is_user_datablock_valid() && eeconfig_read_user_datablock(&tmp, 0, sizeof(tmp)) == sizeof(tmp) && tmp.version == CFG_VERSION) {
        cfg = tmp;
        cfg_sanitize();
    } else {
        cfg_defaults();
        cfg_save();
    }
}

static void apply_cpi(void) {
    pointing_device_set_cpi_on_side(false, cfg.cpi[cfg.cpi_index]);
}

void keyboard_post_init_user(void) {
    cfg_load();
    if (is_keyboard_master()) {
        apply_cpi();
    }
}

/* ------------------------------------------------------------------
 * キーマップ定義
 * ------------------------------------------------------------------ */
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [L_BASE] = {
        { KC_ESC,  TG(1),          TD(2),   TD(3),            TD(4),   TD(5),            XXXXXXX },
        { KC_GRV,  TD(1),          KC_W,    KC_E,             KC_R,    KC_T,             XXXXXXX },
        { KC_TAB,  KC_Q,           KC_S,    KC_D,             KC_F,    KC_G,             XXXXXXX },
        { KC_LGUI, KC_A,           KC_X,    KC_C,             KC_V,    KC_B,             _______ },
        { KC_LALT, KC_Z,           XXXXXXX, LCTL_T(KC_LNG2),  TT(2),   LSFT_T(KC_ENT),   KC_BSPC },
        { XXXXXXX, TD(6),          TD(7),   TD(8),            TD(9),   XXXXXXX,          XXXXXXX },
        { XXXXXXX, KC_Y,           KC_U,    KC_I,             KC_O,    TD(0),            KC_BSPC },
        { XXXXXXX, KC_H,           KC_J,    KC_K,             KC_L,    KC_P,             _______ },
        { _______, KC_N,           KC_M,    KC_COMM,          KC_DOT,  KC_SCLN,          KC_ENT  },
        { _______, RSFT_T(KC_SPC), TT(3),   RCTL_T(KC_LNG1),  XXXXXXX, KC_SLSH,          KC_INT1 }
    },
    [L_ALT] = {
        { _______, TG(1),    KC_2,    KC_3,    KC_4,    KC_5,    XXXXXXX },
        { _______, KC_1,     KC_M,    KC_T,    KC_R,    KC_G,    XXXXXXX },
        { _______, KC_X,     KC_S,    KC_K,    KC_N,    KC_B,    XXXXXXX },
        { _______, KC_H,     KC_C,    KC_V,    KC_D,    KC_P,    _______ },
        { _______, KC_Z,     XXXXXXX, _______, _______, _______, _______ },
        { XXXXXXX, KC_6,     KC_7,    KC_8,    KC_9,    XXXXXXX, XXXXXXX },
        { XXXXXXX, KC_F,     KC_MINS, KC_E,    KC_O,    KC_0,    KC_BSPC },
        { XXXXXXX, KC_Y,     KC_I,    KC_A,    KC_U,    KC_J,    KC_Q    },
        { _______, KC_W,     KC_L,    KC_COMM, KC_LEFT, KC_UP,   KC_DOT  },
        { _______, _______,  _______, _______, XXXXXXX, KC_DOWN, KC_RGHT }
    },
    [L_NUM] = {
        { _______, _______,        _______, _______, _______, _______, XXXXXXX },
        { _______, _______,        _______, _______, _______, _______, XXXXXXX },
        { _______, _______,        _______, _______, _______, _______, XXXXXXX },
        { _______, _______,        _______, _______, _______, _______, _______ },
        { _______, _______,        XXXXXXX, _______, _______, _______, _______ },
        { XXXXXXX, LSFT(KC_MINS),  KC_PSLS, KC_PAST, KC_BSPC, XXXXXXX, XXXXXXX },
        { XXXXXXX, KC_P7,          KC_P8,   KC_P9,   KC_PMNS, _______, _______ },
        { XXXXXXX, KC_P4,          KC_P5,   KC_P6,   KC_PPLS, _______, _______ },
        { XXXXXXX, KC_P1,          KC_P2,   KC_P3,   KC_PENT, _______, _______ },
        { XXXXXXX, KC_P0,          KC_PDOT, KC_PCMM, XXXXXXX, _______, _______ }
    },
    [L_SYM] = {
        { _______, _______,        _______, _______, _______, _______, XXXXXXX },
        { _______, KC_SCLN,        KC_MINS, KC_SLSH, KC_COLN, KC_AMPR, XXXXXXX },
        { _______, _______,        _______, _______, _______, _______, XXXXXXX },
        { _______, _______,        _______, _______, _______, _______, _______ },
        { _______, _______,        XXXXXXX, _______, _______, _______, _______ },
        { XXXXXXX, _______,        _______, _______, _______, XXXXXXX, XXXXXXX },
        { XXXXXXX, _______,        _______, _______, _______, _______, _______ },
        { XXXXXXX, _______,        _______, _______, _______, _______, _______ },
        { XXXXXXX, _______,        _______, _______, _______, _______, _______ },
        { XXXXXXX, _______,        _______, _______, XXXXXXX, _______, _______ }
    },
    [L_MOUSE] = {
        { _______, _______, _______, _______, _______, _______, XXXXXXX },
        { _______, _______, _______, _______, _______, _______, XXXXXXX },
        { _______, _______, _______, _______, _______, _______, XXXXXXX },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, XXXXXXX, _______, _______, _______, _______ },
        { XXXXXXX, _______, _______, _______, _______, XXXXXXX, XXXXXXX },
        { XXXXXXX, _______, _______, _______, _______, _______, _______ },
        { XXXXXXX, _______, _______, _______, _______, _______, _______ },
        { XXXXXXX, _______, _______, _______, _______, _______, _______ },
        { XXXXXXX, _______, _______, _______, XXXXXXX, _______, _______ }
    },
    [L_SPARE5] = {
        { _______, _______, _______, _______, _______, _______, XXXXXXX },
        { _______, _______, _______, _______, _______, _______, XXXXXXX },
        { _______, _______, _______, _______, _______, _______, XXXXXXX },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, XXXXXXX, _______, _______, _______, _______ },
        { XXXXXXX, _______, _______, _______, _______, XXXXXXX, XXXXXXX },
        { XXXXXXX, _______, _______, _______, _______, _______, _______ },
        { XXXXXXX, _______, _______, _______, _______, _______, _______ },
        { XXXXXXX, _______, _______, _______, _______, _______, _______ },
        { XXXXXXX, _______, _______, _______, XXXXXXX, _______, _______ }
    },
    [L_PRECISE] = {
        { _______, _______, _______, _______, _______, _______, XXXXXXX },
        { _______, _______, _______, _______, _______, _______, XXXXXXX },
        { _______, _______, _______, _______, _______, _______, XXXXXXX },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, XXXXXXX, _______, _______, _______, _______ },
        { XXXXXXX, _______, _______, _______, _______, XXXXXXX, XXXXXXX },
        { XXXXXXX, _______, _______, _______, _______, _______, _______ },
        { XXXXXXX, _______, _______, _______, _______, _______, _______ },
        { XXXXXXX, _______, _______, _______, _______, _______, _______ },
        { XXXXXXX, _______, _______, _______, XXXXXXX, _______, _______ }
    },
    [L_SPARE7] = {
{ _______, _______, _______, _______, _______, _______, XXXXXXX },
{ _______, _______, _______, _______, _______, _______, XXXXXXX },
{ _______, _______, _______, _______, _______, _______, XXXXXXX },
{ _______, _______, _______, _______, _______, _______, _______ },
{ _______, _______, XXXXXXX, _______, _______, _______, _______ },
{ XXXXXXX, _______, _______, _______, _______, XXXXXXX, XXXXXXX },
{ XXXXXXX, _______, _______, _______, _______, _______, _______ },
{ XXXXXXX, _______, _______, _______, _______, _______, _______ },
{ XXXXXXX, _______, _______, _______, _______, _______, _______ },
{ XXXXXXX, _______, _______, _______, XXXXXXX, _______, _______ }
},
};
/* ==================================================================
 * VIA Custom UI 用データ送受信処理（ファイルの最末尾に配置）
 * ================================================================== */
#ifdef VIA_ENABLE
#include "via.h"

// プロトタイプ宣言
void via_custom_value_command_kb(uint8_t *data, uint8_t length);

void raw_hid_receive_kb(uint8_t *data, uint8_t length) {
    uint8_t command_id = data[0];

    // 0x07 (Set) または 0x08 (Get) コマンドが来たらカスタム値を処理
    if (command_id == 0x07 || command_id == 0x08) {
        via_custom_value_command_kb(data, length);
        host_raw_hid_send(data, length);
        return;
    }
}

void via_custom_value_command_kb(uint8_t *data, uint8_t length) {
    uint8_t command_id = data[0]; // 0x07: Set / 0x08: Get
    uint8_t value_id   = data[2]; // vial.json で設定した各項目の "id"
    uint8_t value_data = data[4]; // UI側から送られてくる純粋な1バイトの値

    if (command_id == 0x07) { // WebUI側で値を変更し、保存（Set）するとき
        switch (value_id) {
            case 1: cfg.cpi_index     = value_data; apply_cpi(); break;
            case 2: cfg.scroll_index  = value_data; break;
            case 3: cfg.accel_enable  = value_data; break;
            case 4: cfg.accel_curve   = value_data; break;
            case 5: cfg.precision_div = value_data; break;
            case 6: cfg.precision_lock= value_data; break;
            default: break;
        }
        cfg_sanitize(); // 不正な0や範囲外の値を初期値に丸める
        cfg_save();     // EEPROMへ即座に永続保存
    } 
    else if (command_id == 0x08) { // 画面を開いた時や保存直後に値を読み出す（Get）とき
        switch (value_id) {
            case 1: data[4] = cfg.cpi_index; break;
            case 2: data[4] = cfg.scroll_index; break;
            case 3: data[4] = cfg.accel_enable; break;
            case 4: data[4] = cfg.accel_curve; break;
            case 5: data[4] = cfg.precision_div; break;
            case 6: data[4] = cfg.precision_lock; break;
            default: data[4] = 0; break;
        }
    }
}
