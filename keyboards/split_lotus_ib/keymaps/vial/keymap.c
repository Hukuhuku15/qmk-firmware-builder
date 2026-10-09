#include QMK_KEYBOARD_H
#include <stdlib.h>

/* ------------------------------------------------------------------
 * レイヤー定義
 *   0〜3 : 現行の .vil から取り込んだレイヤー
 *   4    : マウスレイヤー(左右どちらかのトラックボール操作で自動ON)
 *   5    : 予備
 *   6    : 精密レイヤー(MO(6) / TG(6) などで手動ON。Vialで任意キーに割当)
 *   7    : 予備
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
 * カスタムキーコード(vial.json の customKeycodes と同じ順序)
 * ------------------------------------------------------------------ */
enum custom_keycodes {
    CPI_UP = QK_KB_0, // 右ボールのCPIを1段階上げる
    CPI_DN,           // 右ボールのCPIを1段階下げる
    SCRL_UP,          // 左ボールのスクロール速度を1段階上げる
    SCRL_DN,          // 左ボールのスクロール速度を1段階下げる
};

/* ------------------------------------------------------------------
 * 調整値(コンパイル時の定数。実行中に変えたいものは下の設定構造体=Vialメニュー側)
 * ------------------------------------------------------------------ */
// --- 自動レイヤー切替 ---
#define AUTO_LAYER_TIMEOUT_MS 800        // 操作が止まってからレイヤーが戻るまで
#define AUTO_LAYER_THRESHOLD 4           // 切替に必要な移動量(|x|+|y| の累積)
#define AUTO_LAYER_ACTIVITY_RESET_MS 100 // この時間止まったら累積をリセット
#define AUTO_LAYER_LOCKOUT_MS 300        // 打鍵直後はレイヤー自動ONを抑止(手の接触対策)

// --- 左ボール: スクロール方向 ---
#define SCROLL_INVERT_H 0 // 1で左右反転
#define SCROLL_INVERT_V 0 // 0:ボールを奥へ転がすと上スクロール / 1:逆(ナチュラル)

// --- 精密レイヤー(右ボール) ---
#define PRECISION_AXIS_DECIDE 3       // 縦横どちらかを判定するのに必要な移動量
#define PRECISION_AXIS_RELEASE_MS 150 // この時間止まったら軸ロック解除

// --- 非線形加速(右ボール) ---
#define ACCEL_WINDOW_MS 8 // 速度の測定窓

/* ------------------------------------------------------------------
 * 設定(EEPROMに保存。Vialのカスタムメニュー/キーから変更)
 * ------------------------------------------------------------------ */
#define SLOT_COUNT 7
#define CFG_VERSION 1

typedef struct __attribute__((packed)) {
    uint8_t  version;
    uint8_t  cpi_index;        // 現在の右ボールCPIスロット(0〜6)
    uint8_t  scroll_index;     // 現在の左ボールスクロール速度スロット(0〜6)
    uint8_t  accel_enable;     // 非線形加速 on/off
    uint8_t  accel_curve;      // カーブ(指数×10)  10=直線 / 20=2乗 / 50=5乗
    uint8_t  precision_div;    // 精密レイヤーの分周(移動量を1/N)
    uint8_t  precision_lock;   // 精密レイヤーの縦横ロック on/off
    uint8_t  reserved;
    uint16_t cpi[SLOT_COUNT];  // 右ボールCPIスロット(小→大)
    uint8_t  scroll_div[SLOT_COUNT]; // スクロール分周スロット(大→小 = 遅→速)
    uint8_t  pad;
    uint16_t accel_max_gain;   // 高速側の最大倍率(%)
    uint16_t accel_low_speed;  // この速さ(カウント/秒)までは等倍
    uint16_t accel_high_speed; // この速さ以上で最大倍率
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

// 範囲外の値を丸める(EEPROM破損・Vialからの不正値対策)
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

// 右ボールのCPIをセンサーに反映(分割キーボードでは相手側へも自動で伝わる)
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
 * キーマップ(matrix 形式 [row][col]。存在しない位置は XXXXXXX)
 * ------------------------------------------------------------------ */
#define TRNS_ROW { _______, _______, _______, _______, _______, _______, _______ }
#define TRNS_LAYER { TRNS_ROW, TRNS_ROW, TRNS_ROW, TRNS_ROW, TRNS_ROW, TRNS_ROW, TRNS_ROW, TRNS_ROW, TRNS_ROW, TRNS_ROW }

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
        { _______, RSFT_T(KC_SPC), TT(3),   RCTL_T(KC_LNG1),  XXXXXXX, KC_SLSH,          KC_INT1   },
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
        { _______, _______,  _______, _______, XXXXXXX, KC_DOWN, KC_RGHT },
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
        { XXXXXXX, KC_P0,          KC_PDOT, KC_PCMM, XXXXXXX, _______, _______ },
    },
    [L_SYM] = {
        { _______, _______,        _______,         _______,         _______,          _______,          XXXXXXX },
        { _______, KC_SCLN,        LSFT(KC_MINS),   KC_SLSH,         LSFT(KC_SCLN),    LSFT(KC_7),       XXXXXXX },
        { KC_INT3, LSFT(KC_5),     KC_RBRC,         LSFT(KC_8),      LSFT(KC_RBRC),    LSFT(KC_COMM),    XXXXXXX },
        { _______, _______,        LSFT(KC_LBRC),   LSFT(KC_INT1),     LSFT(KC_3),       LSFT(KC_6),       _______ },
        { _______, _______,        XXXXXXX,         _______,         _______,          _______,          _______ },
        { XXXXXXX, _______,        _______,         _______,         _______,          XXXXXXX,          XXXXXXX },
        { XXXXXXX, LSFT(KC_2),     KC_MINS,         KC_NUBS,         LSFT(KC_BSLS),    KC_QUOT,          _______ },
        { XXXXXXX, LSFT(KC_DOT),   LSFT(KC_NUHS),   LSFT(KC_9),      KC_NUHS,          KC_EQL,           LSFT(KC_4) },
        { _______, KC_EQL,         KC_LBRC,         LSFT(KC_SLSH),   KC_LEFT,          KC_UP,            _______ },
        { _______, _______,        _______,         _______,         XXXXXXX,          KC_DOWN,          KC_RGHT },
    },
    // 4〜7 は全て透過。クリックやCPIキーなどは Vial から割り当てる
    [L_MOUSE]   = TRNS_LAYER,
    [L_SPARE5]  = TRNS_LAYER,
    [L_PRECISE] = TRNS_LAYER,
    [L_SPARE7]  = TRNS_LAYER,
};

/* ------------------------------------------------------------------
 * 自動マウスレイヤー
 * ------------------------------------------------------------------ */
static bool     mouse_layer_on   = false;
static uint32_t mouse_layer_timer = 0;
static bool     lockout_active   = false;
static uint32_t lockout_timer    = 0;

static void mouse_layer_enable(void) {
    if (!mouse_layer_on) {
        layer_on(L_MOUSE);
        mouse_layer_on = true;
    }
    mouse_layer_timer = timer_read32();
}

static void mouse_layer_disable(void) {
    if (mouse_layer_on) {
        layer_off(L_MOUSE);
        mouse_layer_on = false;
    }
}

typedef struct {
    uint16_t sum;
    uint32_t last;
} activity_t;

static activity_t act_left, act_right;

// 動きを累積し、閾値を超えたら true
static bool activity_update(activity_t *a, int16_t x, int16_t y) {
    if (x == 0 && y == 0) {
        return false;
    }
    uint32_t now = timer_read32();
    if (TIMER_DIFF_32(now, a->last) > AUTO_LAYER_ACTIVITY_RESET_MS) {
        a->sum = 0;
    }
    a->last = now;
    a->sum += abs(x) + abs(y);
    if (a->sum > 1000) {
        a->sum = 1000; // オーバーフロー防止
    }
    return a->sum >= AUTO_LAYER_THRESHOLD;
}

// このキーを押しても自動レイヤーを解除しないか
static bool key_keeps_auto_layer(uint16_t keycode, keyrecord_t *record) {
    // マウスボタン・ホイール等
    if (IS_MOUSEKEY(keycode)) {
        return true;
    }
    // CPI/スクロール調整キー
    if (keycode >= QK_KB_0 && keycode <= QK_KB_31) {
        return true;
    }
    // 修飾キー(Ctrl+スクロール、Shift+クリック等のため)
    if (IS_MODIFIER_KEYCODE(keycode)) {
        return true;
    }
    // モッドタップ/レイヤータップ: ホールド扱いなら維持、タップ(文字入力)なら解除
    if (IS_QK_MOD_TAP(keycode) || IS_QK_LAYER_TAP(keycode)) {
        return record->tap.count == 0;
    }
    // レイヤー切替キー
    if (IS_QK_MOMENTARY(keycode) || IS_QK_TOGGLE_LAYER(keycode) || IS_QK_LAYER_TAP_TOGGLE(keycode)) {
        return true;
    }
    // マウス/精密レイヤー上に置いたキー(Vialで割り当てたクリック等)
    keypos_t pos = record->event.key;
    if (pos.row >= MATRIX_ROWS || pos.col >= MATRIX_COLS) {
        return false; // コンボ等で合成されたイベントは通常打鍵扱い
    }
    uint8_t layer = layer_switch_get_layer(pos);
    return layer == L_MOUSE || layer == L_PRECISE;
}

static uint8_t step_index(uint8_t idx, int8_t delta) {
    int8_t v = (int8_t)idx + delta;
    if (v < 0) v = 0;
    if (v > SLOT_COUNT - 1) v = SLOT_COUNT - 1;
    return (uint8_t)v;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    // CPI / スクロール速度の段階切替
    if (keycode >= CPI_UP && keycode <= SCRL_DN) {
        if (record->event.pressed) {
            switch (keycode) {
                case CPI_UP:
                    cfg.cpi_index = step_index(cfg.cpi_index, +1);
                    apply_cpi();
                    break;
                case CPI_DN:
                    cfg.cpi_index = step_index(cfg.cpi_index, -1);
                    apply_cpi();
                    break;
                case SCRL_UP:
                    cfg.scroll_index = step_index(cfg.scroll_index, +1);
                    break;
                case SCRL_DN:
                    cfg.scroll_index = step_index(cfg.scroll_index, -1);
                    break;
            }
            cfg_save();
            if (mouse_layer_on) {
                mouse_layer_timer = timer_read32();
            }
        }
        return false;
    }

    if (!record->event.pressed) {
        return true;
    }
    if (key_keeps_auto_layer(keycode, record)) {
        if (mouse_layer_on) {
            mouse_layer_timer = timer_read32(); // クリック中などは維持
        }
        return true;
    }
    // 通常の打鍵: 自動レイヤー解除 + 一定時間は再ONを抑止
    mouse_layer_disable();
    lockout_active = true;
    lockout_timer  = timer_read32();
    return true;
}

void housekeeping_task_user(void) {
    if (!is_keyboard_master()) {
        return;
    }
    if (mouse_layer_on && timer_elapsed32(mouse_layer_timer) >= AUTO_LAYER_TIMEOUT_MS) {
        mouse_layer_disable();
    }
}

/* ------------------------------------------------------------------
 * 非線形加速(右ボール)
 *   速さ(カウント/秒)に応じて倍率を 1.0 → 最大倍率 に変化させる。
 *   t = (速さ - 低速閾値) / (高速閾値 - 低速閾値)  を 0〜1 に丸め、
 *   倍率 = 1 + (最大 - 1) * t^カーブ
 * ------------------------------------------------------------------ */
static uint32_t accel_win_start = 0;
static uint32_t accel_win_sum   = 0;
static int32_t  accel_gain_q10  = 1024; // 倍率(1024 = 1.0)
static int32_t  accel_frac_x = 0, accel_frac_y = 0;

// t(0〜32768) の c10/10 乗。整数乗の間を線形補間(libm不要)
static uint32_t pow_q15(uint32_t t, uint8_t c10) {
    uint8_t  n = c10 / 10, f = c10 % 10;
    uint32_t p = 32768;
    for (uint8_t i = 0; i < n; i++) {
        p = (p * t) >> 15;
    }
    uint32_t p1 = (p * t) >> 15;
    return (p * (10 - f) + p1 * f) / 10;
}

static int32_t accel_calc_gain_q10(uint32_t speed) {
    if (!cfg.accel_enable || speed <= cfg.accel_low_speed) {
        return 1024;
    }
    uint32_t t;
    if (speed >= cfg.accel_high_speed) {
        t = 32768;
    } else {
        t = ((speed - cfg.accel_low_speed) << 15) / (cfg.accel_high_speed - cfg.accel_low_speed);
    }
    int32_t max_q10 = ((int32_t)cfg.accel_max_gain * 1024) / 100;
    return 1024 + (((max_q10 - 1024) * (int32_t)pow_q15(t, cfg.accel_curve)) >> 15);
}

// 移動量から速さを更新し、倍率を適用する
static void accel_apply(int16_t *x, int16_t *y) {
    int16_t  ax = abs(*x), ay = abs(*y);
    uint32_t mag = (ax > ay) ? (ax + (ay >> 1)) : (ay + (ax >> 1)); // 簡易ノルム
    accel_win_sum += mag;

    uint32_t now = timer_read32();
    uint32_t dt  = TIMER_DIFF_32(now, accel_win_start);
    if (dt >= ACCEL_WINDOW_MS) {
        accel_gain_q10  = accel_calc_gain_q10((accel_win_sum * 1000) / dt);
        accel_win_sum   = 0;
        accel_win_start = now;
    }

    // 端数を持ち越しながら倍率を掛ける
    accel_frac_x += (int32_t)*x * accel_gain_q10;
    accel_frac_y += (int32_t)*y * accel_gain_q10;
    int32_t ox = accel_frac_x / 1024;
    int32_t oy = accel_frac_y / 1024;
    accel_frac_x -= ox * 1024;
    accel_frac_y -= oy * 1024;
    *x = ox;
    *y = oy;
}

/* ------------------------------------------------------------------
 * トラックボール処理
 *   左: スクロール変換 / 右: ポインタ(通常は非線形加速、精密レイヤー時は減速+縦横ロック)
 * ------------------------------------------------------------------ */
static int16_t scroll_acc_h = 0, scroll_acc_v = 0;
static int16_t prec_acc_x = 0, prec_acc_y = 0;
static int16_t axis_sum_x = 0, axis_sum_y = 0;
static uint8_t axis_lock = 0; // 0:未判定 1:横 2:縦
static uint32_t axis_timer = 0;

static inline int8_t clamp8(int16_t v) {
    return v > 127 ? 127 : (v < -127 ? -127 : (int8_t)v);
}

report_mouse_t pointing_device_task_combined_user(report_mouse_t left_report, report_mouse_t right_report) {
    int16_t lx = left_report.x, ly = left_report.y;
    int16_t rx = right_report.x, ry = right_report.y;

    // ---- 自動レイヤー切替(左右どちらでも同じマウスレイヤー) ----
    if (lockout_active && timer_elapsed32(lockout_timer) >= AUTO_LAYER_LOCKOUT_MS) {
        lockout_active = false;
    }
    bool left_trig  = activity_update(&act_left, lx, ly);
    bool right_trig = activity_update(&act_right, rx, ry);
    if (mouse_layer_on && (lx || ly || rx || ry)) {
        mouse_layer_timer = timer_read32(); // 動いている間は維持
    }
    if (!lockout_active && (left_trig || right_trig)) {
        mouse_layer_enable();
    }

    // ---- 左ボール: スクロール ----
    uint8_t div = cfg.scroll_div[cfg.scroll_index];
    scroll_acc_h += SCROLL_INVERT_H ? -lx : lx;
    scroll_acc_v += SCROLL_INVERT_V ? ly : -ly;
    int16_t h = scroll_acc_h / div;
    int16_t v = scroll_acc_v / div;
    scroll_acc_h -= h * div;
    scroll_acc_v -= v * div;
    left_report.x = 0;
    left_report.y = 0;
    left_report.h = clamp8(h);
    left_report.v = clamp8(v);

    // ---- 右ボール: ポインタ ----
    if (layer_state_is(L_PRECISE)) {
        // 精密: 縦横ロック + 分周
        if (cfg.precision_lock) {
            if (rx || ry) {
                axis_timer = timer_read32();
                if (axis_lock == 0) {
                    axis_sum_x += rx;
                    axis_sum_y += ry;
                    if (abs(axis_sum_x) + abs(axis_sum_y) >= PRECISION_AXIS_DECIDE) {
                        axis_lock = (abs(axis_sum_x) >= abs(axis_sum_y)) ? 1 : 2;
                        rx = axis_sum_x; // 判定までの移動量を引き継ぐ
                        ry = axis_sum_y;
                    } else {
                        rx = 0;
                        ry = 0;
                    }
                }
            }
            if (timer_elapsed32(axis_timer) > PRECISION_AXIS_RELEASE_MS) {
                axis_lock  = 0;
                axis_sum_x = 0;
                axis_sum_y = 0;
            }
            if (axis_lock == 1) {
                ry = 0;
            } else if (axis_lock == 2) {
                rx = 0;
            }
        }
        prec_acc_x += rx;
        prec_acc_y += ry;
        rx = prec_acc_x / cfg.precision_div;
        ry = prec_acc_y / cfg.precision_div;
        prec_acc_x -= rx * cfg.precision_div;
        prec_acc_y -= ry * cfg.precision_div;
        accel_win_sum = 0; // 精密中は加速状態を持ち越さない
        accel_gain_q10 = 1024;
        right_report.x = clamp8(rx);
        right_report.y = clamp8(ry);
    } else {
        prec_acc_x = prec_acc_y = 0;
        axis_lock  = 0;
        axis_sum_x = axis_sum_y = 0;
        // 通常: 非線形加速
        accel_apply(&rx, &ry);
        right_report.x = clamp8(rx);
        right_report.y = clamp8(ry);
    }

    return pointing_device_combine_reports(left_report, right_report);
}

/* ------------------------------------------------------------------
 * Vial カスタムメニュー(VIA custom UI)用ハンドラ
 *   vial.json の "menus" と value_id を揃えてある(チャンネルは 0)
 *   値は 16bit のものは [上位, 下位] の順
 * ------------------------------------------------------------------ */
enum trackball_value_id {
    id_cpi_index           = 1,  // dropdown 0〜6
    id_cpi_slot            = 2,  // range 16bit, 引数: スロット番号
    id_scroll_index        = 3,  // dropdown 0〜6
    id_scroll_slot         = 4,  // range 8bit,  引数: スロット番号
    id_accel_enable        = 5,  // toggle
    id_accel_max_gain      = 6,  // range 16bit (%)
    id_accel_curve         = 7,  // range 8bit  (×0.1)
    id_accel_low_speed     = 8,  // range 16bit (カウント/秒)
    id_accel_high_speed    = 9,  // range 16bit (カウント/秒)
    id_precision_div       = 10, // range 8bit
    id_precision_lock      = 11, // toggle
};

static inline uint16_t get_u16(const uint8_t *p) {
    return ((uint16_t)p[0] << 8) | p[1];
}
static inline void put_u16(uint8_t *p, uint16_t v) {
    p[0] = v >> 8;
    p[1] = v & 0xFF;
}

// d = [value_id, value_data...]
static void tb_set_value(uint8_t *d) {
    uint8_t *v = &d[1];
    switch (d[0]) {
        case id_cpi_index:
            cfg.cpi_index = v[0];
            cfg_sanitize();
            apply_cpi();
            break;
        case id_cpi_slot:
            if (v[0] < SLOT_COUNT) {
                cfg.cpi[v[0]] = get_u16(&v[1]);
                cfg_sanitize();
                if (v[0] == cfg.cpi_index) {
                    apply_cpi();
                }
            }
            break;
        case id_scroll_index:
            cfg.scroll_index = v[0];
            cfg_sanitize();
            break;
        case id_scroll_slot:
            if (v[0] < SLOT_COUNT) {
                cfg.scroll_div[v[0]] = v[1];
                cfg_sanitize();
            }
            break;
        case id_accel_enable:
            cfg.accel_enable = v[0] ? 1 : 0;
            break;
        case id_accel_max_gain:
            cfg.accel_max_gain = get_u16(v);
            cfg_sanitize();
            break;
        case id_accel_curve:
            cfg.accel_curve = v[0];
            cfg_sanitize();
            break;
        case id_accel_low_speed:
            cfg.accel_low_speed = get_u16(v);
            cfg_sanitize();
            break;
        case id_accel_high_speed:
            cfg.accel_high_speed = get_u16(v);
            cfg_sanitize();
            break;
        case id_precision_div:
            cfg.precision_div = v[0];
            cfg_sanitize();
            break;
        case id_precision_lock:
            cfg.precision_lock = v[0] ? 1 : 0;
            break;
    }
}

static void tb_get_value(uint8_t *d) {
    uint8_t *v = &d[1];
    switch (d[0]) {
        case id_cpi_index:
            v[0] = cfg.cpi_index;
            break;
        case id_cpi_slot:
            if (v[0] < SLOT_COUNT) {
                put_u16(&v[1], cfg.cpi[v[0]]);
            }
            break;
        case id_scroll_index:
            v[0] = cfg.scroll_index;
            break;
        case id_scroll_slot:
            if (v[0] < SLOT_COUNT) {
                v[1] = cfg.scroll_div[v[0]];
            }
            break;
        case id_accel_enable:
            v[0] = cfg.accel_enable;
            break;
        case id_accel_max_gain:
            put_u16(v, cfg.accel_max_gain);
            break;
        case id_accel_curve:
            v[0] = cfg.accel_curve;
            break;
        case id_accel_low_speed:
            put_u16(v, cfg.accel_low_speed);
            break;
        case id_accel_high_speed:
            put_u16(v, cfg.accel_high_speed);
            break;
        case id_precision_div:
            v[0] = cfg.precision_div;
            break;
        case id_precision_lock:
            v[0] = cfg.precision_lock;
            break;
    }
}

// data = [command_id, channel_id, value_id, value_data...]
static void tb_custom_command(uint8_t *data, uint8_t length) {
    uint8_t *command_id = &data[0];
    uint8_t *channel_id = &data[1];
    if (*channel_id == id_custom_channel) {
        switch (*command_id) {
            case id_custom_set_value:
                tb_set_value(&data[2]);
                return;
            case id_custom_get_value:
                tb_get_value(&data[2]);
                return;
            case id_custom_save:
                cfg_save();
                return;
        }
    }
    *command_id = id_unhandled;
}

// Vial には VIA の custom UI を直接処理する経路がないため、
// "VIA custom UI for Vial" が送ってくるコマンド(id_unhandled で包まれる)を中継する。
// VIA 本家形式(id_custom_*)で直接届く場合にも対応。
void raw_hid_receive_kb(uint8_t *data, uint8_t length) {
    uint8_t *command_id = &(data[0]);
    if (*command_id == id_unhandled) {
        tb_custom_command(&data[1], length - 1);
    } else if (*command_id == id_custom_set_value || *command_id == id_custom_get_value || *command_id == id_custom_save) {
        tb_custom_command(data, length);
    }
}

#ifdef VIA_ENABLE
#include "via.h"

/* ==================================================================
 * 1. WebUI（WebHID）からのカスタムコマンドを横取りする処理
 * ================================================================== */
void raw_hid_receive_kb(uint8_t *data, uint8_t length) {
    uint8_t *command_id = &(data[0]);

    // カスタムメニューの値変更（Set/Get）要求が来たら、独自の処理を呼び出して即座に返す
    if (*command_id == id_custom_set_value || *command_id == id_custom_get_value) {
        *command_id = id_handled;
        via_custom_value_command_kb(data, length);
        raw_hid_send(data, length);
        return;
    }
}

/* ==================================================================
 * 2. WebUIとキーボード側構造体（cfg）の値を正しく仲介・格納する処理
 * ================================================================== */
void via_custom_value_command_kb(uint8_t *data, uint8_t length) {
    uint8_t command_id = data[0]; // id_custom_set_value または id_custom_get_value
    uint8_t channel    = data[1]; // 通常は 0
    uint8_t value_id   = data[2]; // vial.json で設定した各項目の "id"
    
    // UI側から送られてくる値、またはUI側へ返す値を正しく 1バイト整数 として受け取る
    // (アドレスを直接弄らないため、値が数万などの異常値に化けるのを防ぎます)
    uint8_t value_data = data[3]; 

    switch (command_id) {
        case id_custom_set_value:
            // --- WebUIで変更された値をキーボードの cfg 構造体に1バイトで安全に代入 ---
            switch (value_id) {
                case 1: cfg.cpi_index     = value_data; apply_cpi(); break; // CPIスロット変更時はセンサーに即反映
                case 2: cfg.scroll_index  = value_data; break;
                case 3: cfg.accel_enable  = value_data; break;
                case 4: cfg.accel_curve   = value_data; break;
                case 5: cfg.precision_div = value_data; break;
                case 6: cfg.precision_lock= value_data; break;
                default: break;
            }
            // バリデーションチェックを走らせて範囲外の数値を丸める
            cfg_sanitize();
            break;

        case id_custom_get_value:
            // --- 現在のキーボード側の値を WebUI の画面に表示するために返す ---
            switch (value_id) {
                case 1: data[3] = cfg.cpi_index; break;
                case 2: data[3] = cfg.scroll_index; break;
                case 3: data[3] = cfg.accel_enable; break;
                case 4: data[3] = cfg.accel_curve; break;
                case 5: data[3] = cfg.precision_div; break;
                case 6: data[3] = cfg.precision_lock; break;
                default: data[3] = 0; break;
            }
            break;

        default:
            break;
    }
}
#endif
