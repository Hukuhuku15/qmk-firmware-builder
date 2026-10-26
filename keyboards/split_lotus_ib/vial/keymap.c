#include QMK_KEYBOARD_H
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------
 * レイヤー定義
 *   0〜3  : 現行の .vil から取り込んだレイヤー
 *   4     : マウスレイヤー(トラックボール操作で自動ON。番号はメニューで変更可)
 *   5     : 予備
 *   6     : 精密レイヤー(MO(6) / TG(6) などで手動ON)
 *   7     : ジェスチャー表(既定。番号は左右のボールごとにメニューで変更可)
 *           行0=ゆっくり / 行1=速い、列1〜4 = 上・下・左・右 の位置のキーが出力される
 *   8〜11 : TDGキーの動作表(8:タップ 9:長押し 10:ダブルタップ 11:ダブルタップ長押し)
 * ------------------------------------------------------------------ */
enum layers {
    L_BASE = 0,
    L_ALT,
    L_NUM,
    L_SYM,
    L_MOUSE,
    L_SPARE5,
    L_PRECISE,
    L_GESTURE,
    L_TDG_TAP,
    L_TDG_HOLD,
    L_TDG_DTAP,
    L_TDG_DHOLD,
    LAYER_TOTAL, // = 12 (config.h の DYNAMIC_KEYMAP_LAYER_COUNT と合わせる)
};

/* ------------------------------------------------------------------
 * カスタムキーコード(vial.json の customKeycodes と同じ順序)
 * ------------------------------------------------------------------ */
enum custom_keycodes {
    CPI_UP = QK_KB_0, // マウス動作のボールのCPIを1段階上げる
    CPI_DN,           // マウス動作のボールのCPIを1段階下げる
    SCRL_UP,          // スクロール動作のボールの速度を1段階上げる
    SCRL_DN,          // スクロール動作のボールの速度を1段階下げる
    LINV_X,           // 左ボール X軸反転(トグル)
    LINV_Y,           // 左ボール Y軸反転(トグル)
    RINV_X,           // 右ボール X軸反転(トグル)
    RINV_Y,           // 右ボール Y軸反転(トグル)
    TDG,              // ジェスチャータップ(動作表レイヤー8〜11を呼び出す)
    PICK1,            // 記号ピッカー ① 括弧
    PICK2,            // 記号ピッカー ② 演算子
    PICK3,            // 記号ピッカー ③ 区切り・引用
    PICK4,            // 記号ピッカー ④ その他
    PICK5,            // 記号ピッカー ⑤ 全角 括弧
    PICK6,            // 記号ピッカー ⑥ 全角 句読点
    PICK7,            // 記号ピッカー ⑦ 全角 記号
    CYC1,             // 切替入力 ① 開きカッコ   ( [ { <
    CYC2,             // 切替入力 ② 閉じカッコ   ) ] } >
    CYC3,             // 切替入力 ③ 引用符       ' " `
    CYC4,             // 切替入力 ④ 演算子       + - * / =
    CYC5,             // 切替入力 ⑤ 区切り       , . ; :
    CYC6,             // 切替入力 ⑥ 記号         ! ? @ # $ % ^ &
    CYC7,             // 切替入力 ⑦ 全角 開き    （ 「 ｛ ＜
    CYC8,             // 切替入力 ⑧ 全角 閉じ    ） 」 ｝ ＞
    BALL_MOUSE,       // ボールの機能: マウス       (仮想キー位置に置く。レイヤーごとに指定できる)
    BALL_SCROLL,      // ボールの機能: スクロール
    BALL_PRECISE,     // ボールの機能: 精密マウス
    BALL_GESTURE,     // ボールの機能: ジェスチャー
};

/* ------------------------------------------------------------------
 * 調整値(コンパイル時の定数。実行中に変えたいものは設定構造体=Vialメニュー側)
 * ------------------------------------------------------------------ */
// --- 自動レイヤー切替 ---
#define AUTO_LAYER_THRESHOLD 4           // 切替に必要な移動量(|x|+|y| の累積)
#define AUTO_LAYER_ACTIVITY_RESET_MS 100 // この時間止まったら累積をリセット
#define AUTO_LAYER_LOCKOUT_MS 300        // 打鍵直後はレイヤー自動ONを抑止(手の接触対策)
#define AUTO_LAYER_NONE 255              // 自動レイヤーなし

// --- 精密レイヤー ---
#define PRECISION_AXIS_DECIDE 3       // 縦横どちらかを判定するのに必要な移動量
#define PRECISION_AXIS_RELEASE_MS 150 // この時間止まったら軸ロック解除

// --- 加速の速さ測定窓 ---
#define ACCEL_WINDOW_MS 8

// --- スクロールの縦横のみ ---
#define SCRAXIS_WINDOW_MS 40 // 判定の窓
#define SCRAXIS_HOLD_MS 250  // ロックしてから再判定するまで
#define SCRAXIS_IDLE_MS 100  // この時間動きがなければロック解除

// --- ジェスチャー ---
#define GESTURE_IDLE_MS 150       // この時間動きがなければ距離の累積を捨てる
#define GESTURE_SPEED_WINDOW_MS 40 // 速さの測定窓
#define GESTURE_SPEED_MIN_DT_MS 20 // 途中経過の速さを出すときの最小時間
#define GESTURE_MAX_EMIT 8        // 1回の処理で出す打鍵の上限

// --- TDG(ジェスチャータップ) ---
#define TDG_SLOTS 8 // 同時に処理できるキー数

// --- 記号ピッカー ---
#define PICK_SETS 7        // セット数(PICK1〜PICK7)
#define PICK_FULL_FROM 4   // このセット番号(0始まり)以降は全角セット(IMEをONにして入力)
#define PICK_ITEMS 8       // 1セットの候補数
#define PICK_NONE 0xFF
#define CYC_KEYS 8         // 切替入力のキー数(CYC1〜CYC8)
#define CYC_MAX 8          // 1キーの候補の最大数
#define CYC_FULL_FROM 6    // このキー番号(0始まり)以降は全角(IMEをONにして入力)
#define CYC_NONE 0xFF
#define PICK_TAP_DELAY_MS 3 // 打鍵ごとの待ち(取りこぼし防止)
#define PICK_IME_WAIT_MS 30 // IME切替キーの後の待ち
#define NUM_IME_LAYER L_NUM    // このレイヤーに入るとIMEをOFFにし、出るときに元の状態に戻す
#define NUM_IME_CONTROL 1      // 1: 有効 / 0: 無効
#define PICK_IME_CONTROL 1  // 1: 開くときIMEをOFFにし、終わったら元の状態に戻す / 0: IMEには触れない

// --- ボールの動作 ---
enum ball_mode { MODE_MOUSE = 0, MODE_SCROLL = 1, MODE_GESTURE = 2, MODE_PRECISE = 3, MODE_AUTO = 4 /* 内部用: MO(n) が置かれている */ };

/* ------------------------------------------------------------------
 * 設定(EEPROMに保存。Vialのカスタムメニュー/キーから変更)
 * ------------------------------------------------------------------ */
#define SLOT_COUNT 7
#define CPI_Q_STEP 38 // センサーのCPI刻み(PAW3222: 16〜127 × 38)
#define CFG_VERSION 6

typedef struct __attribute__((packed)) {
    uint8_t  version;
    uint8_t  cpi_index;        // 現在のCPIスロット(0〜6)
    uint8_t  scroll_index;     // 現在のスクロール速度スロット(0〜6)
    uint8_t  accel_enable;     // 非線形加速 on/off
    uint8_t  accel_curve;      // カーブ(指数×10)  10=直線 / 20=2乗 / 50=5乗
    uint8_t  precision_div;    // 精密レイヤーの分周(移動量を1/N)
    uint8_t  precision_lock;   // 精密レイヤーの縦横ロック on/off
    uint8_t  invert_flags;     // 下のINV_* / SWAP / SCR_* のビット
    uint8_t  rot_left;         // 左ボール回転(5度単位 0〜71)
    uint8_t  rot_right;        // 右ボール回転(5度単位 0〜71)
    uint8_t  cpi_q[SLOT_COUNT];      // CPIスロット(CPI ÷ 38、小→大)
    uint8_t  scroll_div[SLOT_COUNT]; // スクロール分周スロット(大→小 = 遅→速、1〜255)
    uint16_t accel_max_gain;   // 高速側の最大倍率(%)
    uint16_t accel_low_speed;  // この速さ(カウント/秒)までは等倍
    uint16_t accel_high_speed; // この速さ以上で最大倍率
    uint8_t  straight_angle;   // 直進補正の角度しきい値(度)。0=無効
    uint8_t  straight_window;  // 直進判定の窓(ms)
    uint8_t  straight_lock;    // ロック時間(×10ms)。経過後に再判定
    uint8_t  x_scale;          // マウス最終出力 X倍率(×2 %)
    uint8_t  y_scale;          // マウス最終出力 Y倍率(×2 %)
    uint8_t  mode_left;        // 左ボールの動作(enum ball_mode)
    uint8_t  mode_right;       // 右ボールの動作
    uint8_t  auto_layer_left;  // 左ボール操作で自動ONになるレイヤー(255=なし)
    uint8_t  auto_layer_right; // 右ボール
    uint8_t  auto_timeout;     // 自動レイヤーのタイムアウト(×10ms)
    uint8_t  gesture_layer_left;  // 左ボールのジェスチャー表レイヤー
    uint8_t  gesture_layer_right; // 右ボール
    uint8_t  gesture_speed;    // 「速い」とみなす速さ(×10 カウント/秒)
    uint8_t  gesture_step_slow; // ゆっくり: 1回の打鍵に必要な移動量(カウント)
    uint8_t  gesture_step_fast; // 速い:     1回の打鍵に必要な移動量(カウント)
    uint8_t  tdg_term;         // TDGの長押し/次のタップを待つ時間(×10ms)
    uint8_t  gesture_fast_repeat; // 0:速いジェスチャーは単発(既定) 1:速くても連続入力
    uint8_t  cyc_timeout;      // 切替入力の自動確定までの時間(×10ms)。0=自動確定しない
    uint8_t  pick_sel[PICK_ITEMS];             // 記号ピッカーの選択キー(sel_table の番号)
    uint8_t  pick_sym[PICK_SETS][PICK_ITEMS];  // 記号ピッカーの記号(sym_table の番号。0=なし)
    uint8_t  cyc_sym[CYC_KEYS][CYC_MAX];       // 切替入力の記号(sym_table の番号。0=ここで終わり)
    uint8_t  reserved[8];
} trackball_cfg_t;

#define INV_LX        0x01 // 左ボール X反転
#define INV_LY        0x02 // 左ボール Y反転
#define INV_RX        0x04 // 右ボール X反転
#define INV_RY        0x08 // 右ボール Y反転
#define INV_SWAP      0x10 // センサーの左右を入れ替える
#define SCR_INV_H     0x20 // スクロール左右反転
#define SCR_INV_V     0x40 // スクロール上下反転(OFF:ボールを奥へ転がすと上スクロール)
#define SCR_AXIS_ONLY 0x80 // スクロールを縦横のみにする(斜めを検出しない)

_Static_assert(sizeof(trackball_cfg_t) <= EECONFIG_USER_DATA_SIZE, "EECONFIG_USER_DATA_SIZE is too small");

/* ------------------------------------------------------------------
 * 記号表(記号ピッカー / 切替入力で選べる記号。JIS配列のキーコード)
 *   番号 0 は「なし」。並び順は vial.json のドロップダウンと同じにすること。
 * ------------------------------------------------------------------ */
static const uint16_t sym_table[] = {
    KC_NO,                                                                  //  0 なし
    LSFT(KC_8), LSFT(KC_9), KC_RBRC, KC_NUHS, LSFT(KC_RBRC), LSFT(KC_NUHS), //  1 ( 2 ) 3 [ 4 ] 5 { 6 }
    LSFT(KC_COMM), LSFT(KC_DOT),                                            //  7 < 8 >
    LSFT(KC_SCLN), KC_MINS, LSFT(KC_QUOT), KC_SLSH, LSFT(KC_MINS),          //  9 + 10 - 11 * 12 / 13 =
    KC_EQL, LSFT(KC_EQL), LSFT(KC_5),                                       // 14 ^ 15 ~ 16 %
    LSFT(KC_1), LSFT(KC_SLSH), LSFT(KC_7), LSFT(KC_2), LSFT(KC_LBRC),       // 17 ! 18 ? 19 ' 20 " 21 `
    KC_QUOT, KC_SCLN, KC_COMM, KC_DOT,                                      // 22 : 23 ; 24 , 25 .
    KC_LBRC, LSFT(KC_3), LSFT(KC_4), LSFT(KC_6),                            // 26 @ 27 # 28 $ 29 &
    LSFT(KC_INT1), LSFT(KC_INT3), KC_INT3, KC_INT1,                         // 30 _ 31 | 32 ¥ 33 バックスラッシュ
    KC_SPC,                                                                 // 34 空白
};
#define SYM_COUNT (sizeof(sym_table) / sizeof(sym_table[0]))

// 記号ピッカーで「選ぶキー」の候補(押したキーのキーコードと比べる)
static const uint16_t sel_table[] = {
    KC_A, KC_B, KC_C, KC_D, KC_E, KC_F, KC_G, KC_H, KC_I, KC_J, KC_K, KC_L, KC_M,
    KC_N, KC_O, KC_P, KC_Q, KC_R, KC_S, KC_T, KC_U, KC_V, KC_W, KC_X, KC_Y, KC_Z,
    KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9, KC_0,
    KC_SCLN, KC_QUOT, KC_COMM, KC_DOT, KC_SLSH,
};
#define SEL_COUNT (sizeof(sel_table) / sizeof(sel_table[0]))

static uint16_t sym_kc(uint8_t idx) {
    return idx < SYM_COUNT ? sym_table[idx] : KC_NO;
}
static uint8_t sym_index_of(uint16_t kc) {
    for (uint8_t i = 1; i < SYM_COUNT; i++) {
        if (sym_table[i] == kc) {
            return i;
        }
    }
    return 0;
}
static uint8_t sel_index_of(uint16_t kc) {
    for (uint8_t i = 0; i < SEL_COUNT; i++) {
        if (sel_table[i] == kc) {
            return i;
        }
    }
    return 0;
}

// --- 初期値(Vialのメニューで変更できる。EEPROMクリア/設定初期化でここに戻る) ---
static const uint16_t pick_default[PICK_SETS][PICK_ITEMS] = {
    // ① 括弧          (        )         [        ]        {                 }                 <                 >
    { LSFT(KC_8), LSFT(KC_9), KC_RBRC, KC_NUHS, LSFT(KC_RBRC), LSFT(KC_NUHS), LSFT(KC_COMM), LSFT(KC_DOT) },
    // ② 演算子        +             -        *              /        =              ^       ~              %
    { LSFT(KC_SCLN), KC_MINS, LSFT(KC_QUOT), KC_SLSH, LSFT(KC_MINS), KC_EQL, LSFT(KC_EQL), LSFT(KC_5) },
    // ③ 区切り・引用  !          ?             '          "          `              :        ;        ,
    { LSFT(KC_1), LSFT(KC_SLSH), LSFT(KC_7), LSFT(KC_2), LSFT(KC_LBRC), KC_QUOT, KC_SCLN, KC_COMM },
    // ④ その他        @        #          $          &          _              |              ¥        \  (バックスラッシュ)
    { KC_LBRC, LSFT(KC_3), LSFT(KC_4), LSFT(KC_6), LSFT(KC_INT1), LSFT(KC_INT3), KC_INT3, KC_INT1 },
    // --- 以下は全角セット: IME ON で半角記号キーを打つと全角になる(IMEの設定による) ---
    // ⑤ 全角括弧      （          ）          「        」        ｛              ｝              ＜              ＞
    { LSFT(KC_8), LSFT(KC_9), KC_RBRC, KC_NUHS, LSFT(KC_RBRC), LSFT(KC_NUHS), LSFT(KC_COMM), LSFT(KC_DOT) },
    // ⑥ 全角 記号等   ・        〜             ！            ？               ：        ；        ＿              ＄
    { KC_SLSH, LSFT(KC_EQL), LSFT(KC_1), LSFT(KC_SLSH), KC_QUOT, KC_SCLN, LSFT(KC_INT1), LSFT(KC_4) },
    // ⑦ 全角記号      ＋             ＝              ＊              ＠        ＃          ＆          ％          ￥
    { LSFT(KC_SCLN), LSFT(KC_MINS), LSFT(KC_QUOT), KC_LBRC, LSFT(KC_3), LSFT(KC_6), LSFT(KC_5), KC_INT3 },
};
static const uint16_t cyc_default[CYC_KEYS][CYC_MAX] = {
    // CYC1 開きカッコ  (            [        {                <
    { LSFT(KC_8), KC_RBRC, LSFT(KC_RBRC), LSFT(KC_COMM), KC_NO, KC_NO, KC_NO, KC_NO },
    // CYC2 閉じカッコ  )            ]        }                >
    { LSFT(KC_9), KC_NUHS, LSFT(KC_NUHS), LSFT(KC_DOT), KC_NO, KC_NO, KC_NO, KC_NO },
    // CYC3 引用符      '            "           `
    { LSFT(KC_7), LSFT(KC_2), LSFT(KC_LBRC), KC_NO, KC_NO, KC_NO, KC_NO, KC_NO },
    // CYC4 演算子      +               -        *              /        =
    { LSFT(KC_SCLN), KC_MINS, LSFT(KC_QUOT), KC_SLSH, LSFT(KC_MINS), KC_NO, KC_NO, KC_NO },
    // CYC5 区切り      ,        .       ;        :
    { KC_COMM, KC_DOT, KC_SCLN, KC_QUOT, KC_NO, KC_NO, KC_NO, KC_NO },
    // CYC6 記号        !            ?              @        #            $            %            ^       &
    { LSFT(KC_1), LSFT(KC_SLSH), KC_LBRC, LSFT(KC_3), LSFT(KC_4), LSFT(KC_5), KC_EQL, LSFT(KC_6) },
    // CYC7 全角 開き   （           「        ｛              ＜
    { LSFT(KC_8), KC_RBRC, LSFT(KC_RBRC), LSFT(KC_COMM), KC_NO, KC_NO, KC_NO, KC_NO },
    // CYC8 全角 閉じ   ）           」        ｝               ＞
    { LSFT(KC_9), KC_NUHS, LSFT(KC_NUHS), LSFT(KC_DOT), KC_NO, KC_NO, KC_NO, KC_NO },
};
// 記号ピッカーの選択キーの初期値: A S D F J K L ;(左小指 → 右小指)
static const uint16_t sel_default[PICK_ITEMS] = {KC_A, KC_S, KC_D, KC_F, KC_J, KC_K, KC_L, KC_SCLN};

static trackball_cfg_t cfg;
static trackball_cfg_t cfg_saved; // EEPROMに書いた最後の内容(変更検出用)

static void cfg_defaults(void) {
    const trackball_cfg_t d = {
        .version          = CFG_VERSION,
        .cpi_index        = 1,
        .scroll_index     = 4, // = 分周10(従来の固定値と同じ)
        .accel_enable     = 1,
        .accel_curve      = 30,
        .precision_div    = 4,
        .precision_lock   = 1,
        // 左右入れ替えなし。左ボールのX/Y反転のみON、右ボールは反転なし
        .invert_flags     = INV_LX | INV_LY,
        .rot_left         = 18, // 90度(従来の ROTATION_90 相当)
        .rot_right        = 18, // 90度
        .cpi_q            = {16, 21, 26, 32, 42, 53, 63}, // 608 / 798 / 988 / 1216 / 1596 / 2014 / 2394
        .scroll_div       = {80, 48, 30, 18, 10, 6, 4},
        .accel_max_gain   = 500,
        .accel_low_speed  = 495,
        .accel_high_speed = 2398,
        .straight_angle   = 1,   // 1度以内なら軸に揃える
        .straight_window  = 30,  // 30ms分の動きで判定
        .straight_lock    = 25,  // 250msロックしたら再判定
        .x_scale          = 50,  // 100%
        .y_scale          = 50,  // 100%
        .mode_left        = MODE_SCROLL,
        .mode_right       = MODE_MOUSE,
        .auto_layer_left  = L_MOUSE,
        .auto_layer_right = L_MOUSE,
        .auto_timeout     = 80,  // 800ms
        .gesture_layer_left  = L_GESTURE,
        .gesture_layer_right = L_GESTURE,
        .gesture_speed    = 100, // 1000カウント/秒以上を「速い」
        .gesture_step_slow = 111,
        .gesture_step_fast = 127,
        .gesture_fast_repeat = 1,
        .tdg_term         = 20,  // 200ms
        .cyc_timeout      = 80,  // 800ms
    };
    cfg = d;
    for (uint8_t i = 0; i < PICK_ITEMS; i++) {
        cfg.pick_sel[i] = sel_index_of(sel_default[i]);
    }
    for (uint8_t st = 0; st < PICK_SETS; st++) {
        for (uint8_t i = 0; i < PICK_ITEMS; i++) {
            cfg.pick_sym[st][i] = sym_index_of(pick_default[st][i]);
        }
    }
    for (uint8_t k = 0; k < CYC_KEYS; k++) {
        for (uint8_t i = 0; i < CYC_MAX; i++) {
            cfg.cyc_sym[k][i] = sym_index_of(cyc_default[k][i]);
        }
    }
}

static void cfg_save(void) {
    cfg_saved = cfg;
    eeconfig_update_user_datablock(&cfg, 0, sizeof(cfg));
}

// 範囲外の値を丸める(EEPROM破損・Vialからの不正値対策)
static void cfg_sanitize(void) {
    if (cfg.cpi_index >= SLOT_COUNT) cfg.cpi_index = 1;
    if (cfg.scroll_index >= SLOT_COUNT) cfg.scroll_index = 4;
    if (cfg.accel_curve < 10) cfg.accel_curve = 10;
    if (cfg.accel_curve > 50) cfg.accel_curve = 50;
    if (cfg.precision_div < 1) cfg.precision_div = 1;
    if (cfg.precision_div > 16) cfg.precision_div = 16;
    if (cfg.accel_max_gain < 100) cfg.accel_max_gain = 100;
    if (cfg.accel_max_gain > 500) cfg.accel_max_gain = 500;
    if (cfg.accel_high_speed <= cfg.accel_low_speed) cfg.accel_high_speed = cfg.accel_low_speed + 1;
    if (cfg.rot_left >= 72) cfg.rot_left = 0;
    if (cfg.rot_right >= 72) cfg.rot_right = 0;
    if (cfg.straight_angle > 45) cfg.straight_angle = 45;
    if (cfg.straight_window < 10) cfg.straight_window = 10;
    if (cfg.straight_window > 200) cfg.straight_window = 200;
    if (cfg.straight_lock < 5) cfg.straight_lock = 5;
    if (cfg.straight_lock > 200) cfg.straight_lock = 200;
    if (cfg.x_scale < 12) cfg.x_scale = 12;
    if (cfg.x_scale > 200) cfg.x_scale = 200;
    if (cfg.y_scale < 12) cfg.y_scale = 12;
    if (cfg.y_scale > 200) cfg.y_scale = 200;
    if (cfg.mode_left > MODE_PRECISE) cfg.mode_left = MODE_SCROLL;
    if (cfg.mode_right > MODE_PRECISE) cfg.mode_right = MODE_MOUSE;
    if (cfg.auto_layer_left != AUTO_LAYER_NONE && cfg.auto_layer_left >= LAYER_TOTAL) cfg.auto_layer_left = L_MOUSE;
    if (cfg.auto_layer_right != AUTO_LAYER_NONE && cfg.auto_layer_right >= LAYER_TOTAL) cfg.auto_layer_right = L_MOUSE;
    if (cfg.auto_timeout < 10) cfg.auto_timeout = 10;
    if (cfg.gesture_layer_left >= LAYER_TOTAL) cfg.gesture_layer_left = L_GESTURE;
    if (cfg.gesture_layer_right >= LAYER_TOTAL) cfg.gesture_layer_right = L_GESTURE;
    if (cfg.gesture_speed < 10) cfg.gesture_speed = 10;
    if (cfg.gesture_step_slow < 5) cfg.gesture_step_slow = 5;
    if (cfg.gesture_step_fast < 5) cfg.gesture_step_fast = 5;
    if (cfg.tdg_term < 5) cfg.tdg_term = 5;
    for (uint8_t i = 0; i < PICK_ITEMS; i++) {
        if (cfg.pick_sel[i] >= SEL_COUNT) cfg.pick_sel[i] = 0;
        for (uint8_t st = 0; st < PICK_SETS; st++) {
            if (cfg.pick_sym[st][i] >= SYM_COUNT) cfg.pick_sym[st][i] = 0;
        }
    }
    for (uint8_t k = 0; k < CYC_KEYS; k++) {
        for (uint8_t i = 0; i < CYC_MAX; i++) {
            if (cfg.cyc_sym[k][i] >= SYM_COUNT) cfg.cyc_sym[k][i] = 0;
        }
    }
    for (uint8_t i = 0; i < SLOT_COUNT; i++) {
        if (cfg.cpi_q[i] < 16) cfg.cpi_q[i] = 16;
        if (cfg.cpi_q[i] > 127) cfg.cpi_q[i] = 127;
        if (cfg.scroll_div[i] < 1) cfg.scroll_div[i] = 1;
    }
}

static void cfg_load(void) {
    trackball_cfg_t tmp;
    if (eeconfig_is_user_datablock_valid() && eeconfig_read_user_datablock(&tmp, 0, sizeof(tmp)) == sizeof(tmp) && tmp.version == CFG_VERSION) {
        cfg = tmp;
        cfg_sanitize();
        cfg_saved = cfg;
    } else {
        cfg_defaults();
        cfg_save();
    }
}

// マウス動作のボールのCPIをセンサーへ反映(分割キーボードでは相手側へも自動で伝わる)
//   マウス動作でないボールのCPIは触らない(センサーの初期値のまま)
//   r=0:左ボール r=1:右ボール。入れ替えON時は QMK から見て左右が逆のセンサーになる
/* ------------------------------------------------------------------
 * ボールの機能(キーマップのレイヤーで指定する)
 *   仮想キー(実スイッチのないマトリクス位置。Vialのキーマップ画面だけに表示される)
 *     左ボール機能 (4,2) / 右ボール機能 (9,4) に BALL_MOUSE / BALL_SCROLL / BALL_PRECISE / BALL_GESTURE を置く。
 *     有効なレイヤーのうち一番上の指定が使われる。透過(▽)・KC_NO・機能以外のキーは下のレイヤーを見る。
 *     MO(n) を置くと「そのボールを動かしたとき、レイヤー n へ自動で移る」(オートレイヤー。戻る時間はメニュー)。
 *     どのレイヤーにも指定がなければ、Vialメニューの既定値(cfg.mode_left / mode_right)を使う。
 *   ジェスチャー方向キー(左右のボールで共有。透過なら下のレイヤーを見る)
 *     ゆっくり: 上(0,6) 下(1,6) 左(2,6) 右(5,0)   速い: 上(5,5) 下(5,6) 左(6,0) 右(7,0)
 * ------------------------------------------------------------------ */
static const keypos_t vk_fn[2] = {
    {.col = 2, .row = 4}, // 左ボール機能
    {.col = 4, .row = 9}, // 右ボール機能
};
static const keypos_t vk_gesture[2][4] = { // [0:ゆっくり 1:速い][上 下 左 右]
    {{.col = 6, .row = 0}, {.col = 6, .row = 1}, {.col = 6, .row = 2}, {.col = 0, .row = 5}},
    {{.col = 5, .row = 5}, {.col = 6, .row = 5}, {.col = 0, .row = 6}, {.col = 0, .row = 7}},
};
#define BALL_FIXED_CPI_Q 21 // スクロール/ジェスチャー時のCPI(×38 = 798)
#define MODE_UNKNOWN 0xFF

static uint8_t ball_cur_mode[2] = {MODE_UNKNOWN, MODE_UNKNOWN};

// 有効なレイヤーのうち一番上の、BALL_* が置かれたキーを探す(機能以外は下へ)
static uint16_t vk_lookup_fn(keypos_t pos) {
    layer_state_t st = layer_state | default_layer_state;
    for (int8_t l = LAYER_TOTAL - 1; l >= 0; l--) {
        if (!layer_state_cmp(st, l)) {
            continue;
        }
        uint16_t kc = keymap_key_to_keycode(l, pos);
        if ((kc >= BALL_MOUSE && kc <= BALL_GESTURE) || IS_QK_MOMENTARY(kc)) {
            return kc; // MO(n) は「ボールを動かすとレイヤー n へ自動で移る」の指定
        }
    }
    return KC_NO;
}

// 有効なレイヤーのうち一番上の、透過でないキーを探す(KC_NOならそこで止まる)
static uint16_t vk_lookup_key(keypos_t pos) {
    layer_state_t st = layer_state | default_layer_state;
    for (int8_t l = LAYER_TOTAL - 1; l >= 0; l--) {
        if (!layer_state_cmp(st, l)) {
            continue;
        }
        uint16_t kc = keymap_key_to_keycode(l, pos);
        if (kc != KC_TRNS) {
            return kc;
        }
    }
    return KC_NO;
}

// ボール i(0:左 1:右)の、今の機能
static uint8_t ball_function(uint8_t i) {
    uint16_t fkc = vk_lookup_fn(vk_fn[i]);
    if (IS_QK_MOMENTARY(fkc)) {
        return MODE_AUTO;
    }
    switch (fkc) {
        case BALL_MOUSE:
            return MODE_MOUSE;
        case BALL_SCROLL:
            return MODE_SCROLL;
        case BALL_PRECISE:
            return MODE_PRECISE;
        case BALL_GESTURE:
            return MODE_GESTURE;
    }
    return i == 0 ? cfg.mode_left : cfg.mode_right;
}

// 機能に合わせてセンサーのCPIを設定する(マウス/精密: メニューのCPI、スクロール/ジェスチャー: 固定)
static void ball_apply_cpi(uint8_t i, uint8_t mode) {
    bool     swap     = (cfg.invert_flags & INV_SWAP) != 0;
    bool     qmk_left = ((i == 0) != swap);
    uint8_t  q        = (mode == MODE_MOUSE || mode == MODE_PRECISE) ? cfg.cpi_q[cfg.cpi_index] : BALL_FIXED_CPI_Q;
    pointing_device_set_cpi_on_side(qmk_left, (uint16_t)q * CPI_Q_STEP);
    ball_cur_mode[i] = mode;
}

// CPI設定が変わったとき/起動時: 両方のボールを今の機能で設定し直す
static void apply_cpi(void) {
    for (uint8_t i = 0; i < 2; i++) {
        ball_apply_cpi(i, ball_function(i));
    }
}

/* ------------------------------------------------------------------
 * キーマップ(matrix 形式 [row][col]。存在しない位置は XXXXXXX)
 * ------------------------------------------------------------------ */
#define TRNS_ROW { _______, _______, _______, _______, _______, _______, _______ }
#define TRNS_LAYER { TRNS_ROW, TRNS_ROW, TRNS_ROW, TRNS_ROW, TRNS_ROW, TRNS_ROW, TRNS_ROW, TRNS_ROW, TRNS_ROW, TRNS_ROW }

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [L_BASE] = {
        { TDG, TG(1), TD(1), TD(2), TD(3), TD(4), KC_UP },
        { TDG, TD(1), KC_W, KC_E, KC_R, KC_T, KC_DOWN },
        { KC_TAB, KC_Q, KC_S, KC_D, KC_F, KC_G, KC_LEFT },
        { KC_LGUI, KC_A, KC_X, KC_C, KC_V, KC_B, MO(6) },
        { KC_LALT, KC_Z, BALL_GESTURE, LCTL_T(KC_LNG2), TT(2), LSFT_T(KC_ENT), KC_BSPC },
        { KC_RGHT, TD(6), TD(7), TD(8), TD(9), KC_UP, KC_DOWN },
        { KC_LEFT, KC_Y, KC_U, KC_I, KC_O, TD(0), KC_BSPC },
        { KC_RGHT, KC_H, KC_J, KC_K, KC_L, KC_P, KC_MINS },
        { _______, KC_N, KC_M, KC_COMM, KC_DOT, KC_SCLN, KC_ENT },
        { TT(5), RSFT_T(KC_SPC), TT(3), RCTL_T(KC_LNG1), MO(4), KC_SLSH, KC_INT1 },
    },
    [L_ALT] = {
        { TDG, TG(1), TDG, TDG, TDG, TDG, _______ },
        { TDG, TDG, KC_M, KC_T, KC_R, KC_G, _______ },
        { _______, KC_X, KC_S, KC_K, KC_N, KC_B, _______ },
        { _______, KC_H, KC_C, KC_V, KC_D, KC_P, _______ },
        { _______, KC_Z, _______, _______, _______, _______, _______ },
        { _______, TDG, TDG, TDG, TDG, _______, _______ },
        { _______, KC_F, KC_MINS, KC_E, KC_O, TDG, TDG },
        { _______, KC_Y, KC_I, KC_A, KC_U, KC_Q, XXXXXXX },
        { _______, KC_L, KC_W, KC_COMM, KC_DOT, KC_J, KC_ENT },
        { _______, _______, _______, _______, _______, XXXXXXX, XXXXXXX },
    },
    [L_NUM] = {
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, LSFT(KC_MINS), KC_PSLS, KC_PAST, KC_BSPC, _______, _______ },
        { _______, KC_P7, KC_P8, KC_P9, KC_PMNS, _______, _______ },
        { _______, KC_P4, KC_P5, KC_P6, KC_PPLS, _______, _______ },
        { XXXXXXX, KC_P1, KC_P2, KC_P3, KC_PENT, _______, _______ },
        { XXXXXXX, KC_P0, KC_PDOT, KC_PCMM, _______, _______, _______ },
    },
    [L_SYM] = {
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, CYC3, CYC4, CYC1, CYC7, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, CYC8, CYC2, CYC5, CYC6, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
    },
    [L_MOUSE] = {
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, KC_BTN2, KC_BTN1, KC_BTN3, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, BALL_SCROLL, _______, _______, _______, _______ },
        { _______, _______, KC_WH_U, _______, _______, _______, _______ },
        { _______, KC_WH_L, KC_WH_D, KC_WH_R, _______, SCRL_UP, CPI_UP },
        { _______, KC_BTN1, KC_BTN2, KC_BTN3, _______, SCRL_DN, CPI_DN },
        { KC_BTN3, _______, _______, _______, _______, _______, _______ },
        { KC_BTN1, KC_BTN2, _______, _______, BALL_MOUSE, _______, _______ },
    },
    [L_SPARE5] = {
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
    },
    [L_PRECISE] = {
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
    },
    [L_GESTURE] = {
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
    },
    [L_TDG_TAP] = {
        { KC_ESC, _______, KC_2, KC_3, KC_4, KC_5, _______ },
        { KC_GRV, KC_1, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, KC_6, KC_7, KC_8, KC_9, _______, _______ },
        { _______, _______, _______, _______, _______, KC_0, KC_BSPC },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
    },
    [L_TDG_HOLD] = {
        { _______, _______, KC_F2, KC_F3, KC_F4, KC_F5, _______ },
        { _______, KC_F1, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, KC_F6, KC_F7, KC_F8, KC_F9, _______, _______ },
        { _______, _______, _______, _______, _______, KC_F10, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
    },
    [L_TDG_DTAP] = {
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
    },
    [L_TDG_DHOLD] = {
        { QK_MACRO_0, _______, KC_F12, _______, _______, _______, _______ },
        { QK_MACRO_1, KC_F11, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, KC_DEL },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
        { _______, _______, _______, _______, _______, _______, _______ },
    },
};


/* ------------------------------------------------------------------
 * ボールごとの状態(0:左 1:右)
 * ------------------------------------------------------------------ */
typedef struct {
    int32_t fx, fy; // 端数の持ち越し(1024 = 1カウント)
} rot_frac_t;

typedef struct {
    uint16_t sum;
    uint32_t last;
} activity_t;

typedef struct {
    // 自動レイヤー判定
    activity_t act;
    // 軸の向き(回転)
    rot_frac_t rot;
    // マウス: 直進補正
    uint8_t  st_lock; // 0:なし 1:横(Yを捨てる) 2:縦(Xを捨てる)
    uint32_t st_lock_start, st_win_start, st_last_motion;
    uint16_t st_win_x, st_win_y, st_last_x, st_last_y;
    // マウス: 加速
    uint32_t accel_win_start, accel_win_sum;
    int32_t  accel_gain_q10; // 倍率(1024 = 1.0)
    int32_t  accel_frac_x, accel_frac_y;
    // マウス: 最終倍率
    int32_t  scale_frac_x, scale_frac_y;
    // マウス: 精密レイヤー
    int16_t  prec_acc_x, prec_acc_y, axis_sum_x, axis_sum_y;
    uint8_t  axis_lock; // 0:未判定 1:横 2:縦
    uint32_t axis_timer;
    // スクロール
    int16_t  scroll_acc_h, scroll_acc_v;
    uint8_t  sa_lock; // 縦横のみ: 0:なし 1:横 2:縦
    uint32_t sa_lock_start, sa_win_start, sa_last_motion;
    uint16_t sa_win_x, sa_win_y, sa_last_x, sa_last_y;
    // ジェスチャー
    int16_t  g_x, g_y;
    uint32_t g_last_motion, g_spd_start;
    bool     g_fast_done; // 速いジェスチャーで1回打鍵済み(動きが止まるまで追加の打鍵をしない)
    uint32_t g_spd_sum, g_spd; // 直近の速さ(カウント/秒)
} ball_t;

static ball_t ball[2];

static void balls_init(void) {
    memset(ball, 0, sizeof(ball));
    for (uint8_t i = 0; i < 2; i++) {
        ball[i].accel_gain_q10 = 1024;
    }
}

static inline int8_t clamp8(int16_t v) {
    return v > 127 ? 127 : (v < -127 ? -127 : (int8_t)v);
}

/* ------------------------------------------------------------------
 * 自動レイヤー(ボールごとに番号を設定可)
 * ------------------------------------------------------------------ */
static uint8_t  auto_cur         = AUTO_LAYER_NONE; // 現在ONにしている自動レイヤー
static uint32_t auto_timer       = 0;
static bool     lockout_active   = false;
static uint32_t lockout_timer    = 0;

static void auto_layer_enable(uint8_t layer) {
    if (layer == AUTO_LAYER_NONE) {
        return;
    }
    if (auto_cur != layer) {
        if (auto_cur != AUTO_LAYER_NONE) {
            layer_off(auto_cur);
        }
        layer_on(layer);
        auto_cur = layer;
    }
    auto_timer = timer_read32();
}

static void auto_layer_disable(void) {
    if (auto_cur != AUTO_LAYER_NONE) {
        layer_off(auto_cur);
        auto_cur = AUTO_LAYER_NONE;
    }
}

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
    // CPI/スクロール/反転の調整キー(TDGは通常の打鍵扱い)
    if (keycode >= CPI_UP && keycode <= RINV_Y) {
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
    // 自動/精密レイヤー上に置いたキー(Vialで割り当てたクリック等)
    keypos_t pos = record->event.key;
    if (pos.row >= MATRIX_ROWS || pos.col >= MATRIX_COLS) {
        return false; // コンボ等で合成されたイベントは通常打鍵扱い
    }
    uint8_t layer = layer_switch_get_layer(pos);
    return layer == L_PRECISE || (auto_cur != AUTO_LAYER_NONE && layer == auto_cur);
}

/* ------------------------------------------------------------------
 * TDG(ジェスチャータップ)
 *   TDGを置いたキーと同じ位置にある、レイヤー8〜11のキーを呼び出す
 *     8:タップ 9:長押し 10:ダブルタップ 11:ダブルタップ長押し
 *   空(KC_NO / 透過)の動作は「未割り当て」: 長押し・ダブルが全て空なら普通のキーと同じ速さで動く
 * ------------------------------------------------------------------ */
enum { TDG_FREE = 0, TDG_PRESS1, TDG_WAIT2, TDG_PRESS2, TDG_HELD };

typedef struct {
    uint8_t  state;
    keypos_t pos;
    uint32_t timer;
    uint16_t held; // 押しっぱなしにしているキー
} tdg_t;

static tdg_t tdg[TDG_SLOTS];

static uint16_t tdg_kc(keypos_t pos, uint8_t layer) {
    uint16_t kc = keymap_key_to_keycode(layer, pos);
    return kc > KC_TRNS ? kc : KC_NO; // KC_NO(0) と KC_TRNS(1) は未割り当て
}

static tdg_t *tdg_find(keypos_t pos, uint8_t state) {
    for (uint8_t i = 0; i < TDG_SLOTS; i++) {
        if (tdg[i].state == state && tdg[i].pos.row == pos.row && tdg[i].pos.col == pos.col) {
            return &tdg[i];
        }
    }
    return NULL;
}

// キーコードの出力: Vial標準の仕組み(タップダンスの動作と同じ)で、あらゆるキーコード
//   (文字・修飾・マクロ・レイヤー切替・マウス・カスタムキーなど)を出せる
static void kc_down(uint16_t kc) { vial_keycode_down(kc); }
static void kc_up(uint16_t kc) { vial_keycode_up(kc); }
static void kc_tap(uint16_t kc) { vial_keycode_tap(kc); }

static void tdg_tap(uint16_t kc) {
    if (kc != KC_NO) {
        kc_tap(kc);
    }
}

static void tdg_hold(tdg_t *t, uint16_t kc) {
    if (kc != KC_NO) {
        kc_down(kc);
        t->held  = kc;
        t->state = TDG_HELD;
    } else {
        t->state = TDG_FREE;
    }
}

// 次のタップ待ちのものを、今のキー(タップ)として確定する
static void tdg_flush_wait(void) {
    for (uint8_t i = 0; i < TDG_SLOTS; i++) {
        if (tdg[i].state == TDG_WAIT2) {
            tdg_tap(tdg_kc(tdg[i].pos, L_TDG_TAP));
            tdg[i].state = TDG_FREE;
        }
    }
}

static void tdg_press(keypos_t pos) {
    tdg_t *t = tdg_find(pos, TDG_WAIT2);
    if (t) { // 2回目の押下
        t->state = TDG_PRESS2;
        t->timer = timer_read32();
        return;
    }
    tdg_flush_wait(); // 他のキーの待ちは先に確定
    for (uint8_t i = 0; i < TDG_SLOTS; i++) {
        if (tdg[i].state == TDG_FREE) {
            t      = &tdg[i];
            t->pos = pos;
            break;
        }
    }
    if (!t) {
        return;
    }
    bool has_hold  = tdg_kc(pos, L_TDG_HOLD) != KC_NO;
    bool has_dtap  = tdg_kc(pos, L_TDG_DTAP) != KC_NO;
    bool has_dhold = tdg_kc(pos, L_TDG_DHOLD) != KC_NO;
    if (!has_hold && !has_dtap && !has_dhold) {
        tdg_hold(t, tdg_kc(pos, L_TDG_TAP)); // 普通のキーと同じ(待たない)
    } else {
        t->state = TDG_PRESS1;
        t->timer = timer_read32();
    }
}

static void tdg_release(keypos_t pos) {
    tdg_t *t = tdg_find(pos, TDG_HELD);
    if (t) {
        kc_up(t->held);
        t->state = TDG_FREE;
        return;
    }
    t = tdg_find(pos, TDG_PRESS1);
    if (t) { // 長押しになる前に離した = タップ
        if (tdg_kc(pos, L_TDG_DTAP) != KC_NO || tdg_kc(pos, L_TDG_DHOLD) != KC_NO) {
            t->state = TDG_WAIT2;
            t->timer = timer_read32();
        } else {
            tdg_tap(tdg_kc(pos, L_TDG_TAP));
            t->state = TDG_FREE;
        }
        return;
    }
    t = tdg_find(pos, TDG_PRESS2);
    if (t) { // 2回目を長押しになる前に離した = ダブルタップ
        uint16_t dtap = tdg_kc(pos, L_TDG_DTAP);
        if (dtap != KC_NO) {
            tdg_tap(dtap);
        } else {
            tdg_tap(tdg_kc(pos, L_TDG_TAP));
            tdg_tap(tdg_kc(pos, L_TDG_TAP));
        }
        t->state = TDG_FREE;
    }
}

static void tdg_task(void) {
    uint32_t term = (uint32_t)cfg.tdg_term * 10;
    for (uint8_t i = 0; i < TDG_SLOTS; i++) {
        tdg_t *t = &tdg[i];
        if (t->state == TDG_FREE || t->state == TDG_HELD || timer_elapsed32(t->timer) < term) {
            continue;
        }
        switch (t->state) {
            case TDG_PRESS1: { // 押しっぱなし = 長押し(空ならタップのキーを押し続ける)
                uint16_t hold = tdg_kc(t->pos, L_TDG_HOLD);
                tdg_hold(t, hold != KC_NO ? hold : tdg_kc(t->pos, L_TDG_TAP));
                break;
            }
            case TDG_WAIT2: // 2回目が来なかった = タップ
                tdg_tap(tdg_kc(t->pos, L_TDG_TAP));
                t->state = TDG_FREE;
                break;
            case TDG_PRESS2: { // 2回目が押しっぱなし = ダブルタップ長押し(空ならタップ+タップ押し続け)
                uint16_t dhold = tdg_kc(t->pos, L_TDG_DHOLD);
                if (dhold == KC_NO) {
                    tdg_tap(tdg_kc(t->pos, L_TDG_TAP));
                    dhold = tdg_kc(t->pos, L_TDG_TAP);
                }
                tdg_hold(t, dhold);
                break;
            }
        }
    }
}

static uint8_t step_index(uint8_t idx, int8_t delta) {
    int8_t v = (int8_t)idx + delta;
    if (v < 0) v = 0;
    if (v > SLOT_COUNT - 1) v = SLOT_COUNT - 1;
    return (uint8_t)v;
}

/* ------------------------------------------------------------------
 * 記号ピッカー
 *   PICK1〜4 を押すと「1( 2) 3[ ...」のように候補を入力する。
 *   ホームポジションの8キー(左小指=1 … 左人差指=4、右人差指=5 … 右小指=8)を押すと、
 *   候補を全部消して選んだ記号だけを入力する。
 *   Esc / BackSpace は候補を全部消して終了。他のキーは候補を消してそのキーを通常入力。
 *   マウスクリックは候補を消さずに終了(カーソル位置が変わるため)。
 *   JIS配列のキーコードで記述。記号の入れ替えは下の表を書き換える。
 * ------------------------------------------------------------------ */
static uint16_t pick_sym_kc(uint8_t set, uint8_t i) {
    return sym_kc(cfg.pick_sym[set][i]);
}

static uint8_t pick_set      = PICK_NONE; // 候補表示中のセット
static uint8_t pick_len      = 0;         // 入力した候補の文字数(消すときの数)
static uint8_t pick_swallow  = 0;         // 選択に使ったキーのリリースを無視するためのビット

/* IMEの状態追跡: キーボード側から実際のIME状態は読めないので、
 * 最後に押した KC_LNG1(IME ON) / KC_LNG2(IME OFF) から推測する。
 * 不明(起動後まだ押していない)のときは、元に戻す動作はしない。 */
static int8_t  ime_known    = -1; // -1:不明 0:OFF 1:ON
static int8_t  pick_ime_prev = -1;      // 開く前のIME状態の推測(-1:不明)
static bool    pick_cur_full = false;   // 表示中のセットが全角セットか

static void ime_track(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) {
        return;
    }
    uint16_t kc = keycode;
    if (IS_QK_MOD_TAP(keycode)) {
        if (record->tap.count == 0) {
            return; // 長押し側(修飾キー)
        }
        kc = QK_MOD_TAP_GET_TAP_KEYCODE(keycode);
    } else if (IS_QK_LAYER_TAP(keycode)) {
        if (record->tap.count == 0) {
            return;
        }
        kc = QK_LAYER_TAP_GET_TAP_KEYCODE(keycode);
    }
    if (kc == KC_LNG1) {
        ime_known = 1;
    } else if (kc == KC_LNG2) {
        ime_known = 0;
    }
}

// 半角セットはIME OFF、全角セットはIME ON で候補を入力する
static void pick_ime_begin(bool full, bool first) {
#if PICK_IME_CONTROL
    if (first) {
        pick_ime_prev = ime_known;
    }
    tap_code_delay(full ? KC_LNG1 : KC_LNG2, PICK_TAP_DELAY_MS);
    ime_known = full ? 1 : 0;
    wait_ms(PICK_IME_WAIT_MS);
#endif
    pick_cur_full = full;
}

// 終了時: 全角セットは変換中の文字を確定(Enter)してから、IMEを元の状態に戻す
static void pick_ime_end(bool commit) {
#if PICK_IME_CONTROL
    if (commit && pick_cur_full) {
        tap_code_delay(KC_ENT, PICK_TAP_DELAY_MS);
    }
    if (pick_ime_prev == 1 && ime_known != 1) {
        wait_ms(PICK_IME_WAIT_MS);
        tap_code_delay(KC_LNG1, PICK_TAP_DELAY_MS);
        ime_known = 1;
    } else if (pick_ime_prev == 0 && ime_known != 0) {
        wait_ms(PICK_IME_WAIT_MS);
        tap_code_delay(KC_LNG2, PICK_TAP_DELAY_MS);
        ime_known = 0;
    }
    pick_ime_prev = -1;
#endif
    pick_cur_full = false;
}

static void pick_type(uint16_t kc) {
    tap_code16_delay(kc, PICK_TAP_DELAY_MS);
}

static void pick_erase(void) {
    for (uint8_t i = 0; i < pick_len; i++) {
        tap_code_delay(KC_BSPC, PICK_TAP_DELAY_MS);
    }
    pick_len = 0;
}

static void pick_open(uint8_t set) {
    pick_len   = 0;
    bool first = true;
    for (uint8_t i = 0; i < PICK_ITEMS; i++) {
        uint16_t kc = pick_sym_kc(set, i);
        if (kc == KC_NO) {
            continue; // 「なし」の枠は出さない(番号はそのまま)
        }
        if (!first && set < PICK_FULL_FROM) { // 全角セットは空白を入れない(IMEが変換を始めてしまうため)
            pick_type(KC_SPC);
            pick_len++;
        }
        first = false;
        pick_type(KC_1 + i); // 数字の目印(KC_1〜KC_8)
        pick_type(kc);
        pick_len += 2;
    }
    pick_set = set;
}

// 押したキーが、設定された選択キーのどれか(番号 0〜7)。該当なしは -1
static int8_t pick_sel_match(uint16_t keycode) {
    for (uint8_t i = 0; i < PICK_ITEMS; i++) {
        if (cfg.pick_sel[i] < SEL_COUNT && sel_table[cfg.pick_sel[i]] == keycode) {
            return (int8_t)i;
        }
    }
    return -1;
}
// 表示中のセットで、記号が入っている枠だけを選べる
static int8_t pick_sel_find(uint16_t keycode) {
    int8_t i = pick_sel_match(keycode);
    if (i >= 0 && pick_set != PICK_NONE && pick_sym_kc(pick_set, (uint8_t)i) != KC_NO) {
        return i;
    }
    return -1;
}

/* ------------------------------------------------------------------
 * 切替入力(CYC1〜8)
 *   キーを押すたびに、そのキーの候補リストの文字が1文字ずつ入れ替わる。
 *   例: CYC1 は ( → [ → { → < → ( …  (押すたびに直前の1文字をBackSpaceで消して次を入力)
 *   他のキーを押す / 一定時間たつ(Vialメニューで調整) / クリック で確定する。
 *   CYC_FULL_FROM 以降のキーは全角(IMEをONにして入力し、確定時にEnter)。
 * ------------------------------------------------------------------ */
static uint8_t  cyc_key   = CYC_NONE; // 切替中のキー番号
static uint8_t  cyc_idx   = 0;        // 現在の候補番号
static uint32_t cyc_timer = 0;

static uint16_t cyc_kc(uint8_t k, uint8_t i) {
    return sym_kc(cfg.cyc_sym[k][i]);
}
static uint8_t cyc_count(uint8_t k) {
    uint8_t n = 0;
    while (n < CYC_MAX && cyc_kc(k, n) != KC_NO) {
        n++;
    }
    return n;
}

static void cyc_finish(bool commit) {
    if (cyc_key == CYC_NONE) {
        return;
    }
    cyc_key = CYC_NONE;
    pick_ime_end(commit);
}

// 戻り値 true: このキーイベントは処理済み(通常処理しない)
static bool pick_handle(uint16_t keycode, keyrecord_t *record) {
    // 選択に使ったキーのリリースは無視する
    if (!record->event.pressed) {
        if (pick_swallow) {
            int8_t idx = pick_sel_match(keycode);
            if (idx >= 0 && (pick_swallow & (1u << idx))) {
                pick_swallow &= (uint8_t)~(1u << idx);
                return true;
            }
        }
        return false;
    }

    bool is_pick_key = (keycode >= PICK1 && keycode < PICK1 + PICK_SETS);
    if (pick_set == PICK_NONE) {
        if (!is_pick_key) {
            return false;
        }
        // 開く(Shift等を押していても記号が正しく出るよう、修飾を一時的に外す)
        uint8_t mods = get_mods();
        uint8_t weak = get_weak_mods();
        uint8_t osm  = get_oneshot_mods();
        clear_mods();
        clear_weak_mods();
        clear_oneshot_mods();
        cyc_finish(true); // 切替入力の途中なら確定してから開く
        pick_ime_begin((keycode - PICK1) >= PICK_FULL_FROM, true);
        pick_open(keycode - PICK1);
        set_mods(mods);
        set_weak_mods(weak);
        set_oneshot_mods(osm);
        return true;
    }

    // --- 候補表示中 ---
    uint8_t mods = get_mods();
    uint8_t weak = get_weak_mods();
    uint8_t osm  = get_oneshot_mods();

    {
        int8_t idx = pick_sel_find(keycode);
        if (idx >= 0) {
            uint8_t set = pick_set;
            clear_mods();
            clear_weak_mods();
            clear_oneshot_mods();
            pick_erase();
            pick_type(pick_sym_kc(set, (uint8_t)idx));
            pick_ime_end(true);
            set_mods(mods);
            set_weak_mods(weak);
            set_oneshot_mods(osm);
            pick_set = PICK_NONE;
            pick_swallow |= (uint8_t)(1u << idx);
            return true;
        }
    }

    // 修飾キー・一時レイヤー(MO)は邪魔しない
    if (IS_MODIFIER_KEYCODE(keycode) || IS_QK_MOMENTARY(keycode)) {
        return false;
    }
    // クリックはカーソル位置が変わるので、候補は消さずに終了
    if (IS_MOUSEKEY(keycode)) {
        pick_set = PICK_NONE;
        pick_ime_end(false);
        return false;
    }

    // ここから「候補を全部消して終了」
    clear_mods();
    clear_weak_mods();
    clear_oneshot_mods();
    pick_erase();
    set_mods(mods);
    set_weak_mods(weak);
    set_oneshot_mods(osm);
    uint8_t prev = pick_set;
    pick_set = PICK_NONE;
    if (keycode == KC_ESC || keycode == KC_BSPC) {
        pick_ime_end(false);
        return true; // 消すだけ
    }
    if (is_pick_key) {
        if ((uint8_t)(keycode - PICK1) != prev) {
            clear_mods();
            clear_weak_mods();
            clear_oneshot_mods();
            pick_ime_begin((keycode - PICK1) >= PICK_FULL_FROM, false);
            pick_open(keycode - PICK1); // 別のセットなら開き直す
            set_mods(mods);
            set_weak_mods(weak);
            set_oneshot_mods(osm);
            return true;
        }
        pick_ime_end(false);
        return true; // 同じキーなら閉じるだけ
    }
    pick_ime_end(false);
    return false; // 他のキーは通常どおり入力
}

static void cyc_mods_save_clear(uint8_t *m, uint8_t *w, uint8_t *o) {
    *m = get_mods();
    *w = get_weak_mods();
    *o = get_oneshot_mods();
    clear_mods();
    clear_weak_mods();
    clear_oneshot_mods();
}

static void cyc_mods_restore(uint8_t m, uint8_t w, uint8_t o) {
    set_mods(m);
    set_weak_mods(w);
    set_oneshot_mods(o);
}

static bool cyc_handle(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) {
        return false;
    }
    bool is_cyc = (keycode >= CYC1 && keycode < CYC1 + CYC_KEYS);
    uint8_t m, w, o;
    if (cyc_key == CYC_NONE) {
        if (!is_cyc) {
            return false;
        }
        uint8_t k = keycode - CYC1;
        if (cyc_count(k) == 0) {
            return true; // 候補が1つも登録されていないキー
        }
        cyc_mods_save_clear(&m, &w, &o);
        pick_ime_begin(k >= CYC_FULL_FROM, true);
        pick_type(cyc_kc(k, 0));
        cyc_mods_restore(m, w, o);
        cyc_key   = k;
        cyc_idx   = 0;
        cyc_timer = timer_read32();
        return true;
    }
    // --- 切替中 ---
    if (is_cyc && (uint8_t)(keycode - CYC1) == cyc_key) {
        uint8_t n = cyc_count(cyc_key);
        cyc_idx   = (cyc_idx + 1) % (n ? n : 1);
        cyc_mods_save_clear(&m, &w, &o);
        tap_code_delay(KC_BSPC, PICK_TAP_DELAY_MS);
        pick_type(cyc_kc(cyc_key, cyc_idx));
        cyc_mods_restore(m, w, o);
        cyc_timer = timer_read32();
        return true;
    }
    if (IS_MODIFIER_KEYCODE(keycode) || IS_QK_MOMENTARY(keycode)) {
        return false; // 修飾キー・一時レイヤーは邪魔しない
    }
    if (IS_MOUSEKEY(keycode)) {
        cyc_finish(false);
        return false;
    }
    if (keycode == KC_ESC || keycode == KC_BSPC) {
        cyc_mods_save_clear(&m, &w, &o);
        tap_code_delay(KC_BSPC, PICK_TAP_DELAY_MS); // 入力中の1文字を消して終了
        cyc_mods_restore(m, w, o);
        cyc_finish(false);
        return true;
    }
    cyc_finish(true); // 他のキー: 確定してからそのキーを処理
    if (is_cyc) {
        uint8_t k = keycode - CYC1;
        if (cyc_count(k) == 0) {
            return true; // 候補が1つも登録されていないキー
        }
        cyc_mods_save_clear(&m, &w, &o);
        pick_ime_begin(k >= CYC_FULL_FROM, true);
        pick_type(cyc_kc(k, 0));
        cyc_mods_restore(m, w, o);
        cyc_key   = k;
        cyc_idx   = 0;
        cyc_timer = timer_read32();
        return true;
    }
    return false;
}

/* ------------------------------------------------------------------
 * テンキーレイヤー(NUM_IME_LAYER)に入ったらIME OFF、出たら元の状態へ
 *   レイヤーの変化はフラグだけ立て、キー送信は housekeeping_task_user で行う
 *   (入ってすぐ出た場合などは、何も送らずに打ち消す)
 * ------------------------------------------------------------------ */
static bool    numime_in      = false; // 現在NUMレイヤーにいるか
static int8_t  numime_pending = 0;     // 0:なし 1:入った 2:出た
static int8_t  numime_prev    = -1;    // 入る前のIME状態の推測

layer_state_t layer_state_set_user(layer_state_t state) {
#if NUM_IME_CONTROL
    if (is_keyboard_master()) {
        bool now = layer_state_cmp(state, NUM_IME_LAYER);
        if (now != numime_in) {
            numime_in = now;
            if (numime_pending == 0) {
                numime_pending = now ? 1 : 2;
            } else {
                numime_pending = 0; // 処理前に逆の動きがあったので打ち消す
            }
        }
    }
#endif
    return state;
}

static void numime_task(void) {
#if NUM_IME_CONTROL
    if (numime_pending == 1) {
        numime_prev = ime_known;
        tap_code_delay(KC_LNG2, PICK_TAP_DELAY_MS);
        ime_known = 0;
    } else if (numime_pending == 2) {
        if (numime_prev == 1 && ime_known != 1) {
            tap_code_delay(KC_LNG1, PICK_TAP_DELAY_MS);
            ime_known = 1;
        }
        numime_prev = -1;
    }
    numime_pending = 0;
#endif
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    ime_track(keycode, record);

    // 記号ピッカー(候補表示中のキー処理)
    if (pick_handle(keycode, record) || cyc_handle(keycode, record)) {
        if (record->event.pressed) {
            auto_layer_disable();
            lockout_active = true;
            lockout_timer  = timer_read32();
        }
        return false;
    }

    // CPI / スクロール速度 / 反転 のキー
    if (keycode >= CPI_UP && keycode <= RINV_Y) {
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
                case LINV_X:
                    cfg.invert_flags ^= INV_LX;
                    break;
                case LINV_Y:
                    cfg.invert_flags ^= INV_LY;
                    break;
                case RINV_X:
                    cfg.invert_flags ^= INV_RX;
                    break;
                case RINV_Y:
                    cfg.invert_flags ^= INV_RY;
                    break;
            }
            cfg_save();
            if (auto_cur != AUTO_LAYER_NONE) {
                auto_timer = timer_read32();
            }
        }
        return false;
    }

    // TDG(通常の打鍵と同じ扱い: 自動レイヤー解除 + 一定時間は再ONを抑止)
    if (keycode == TDG) {
        keypos_t pos = record->event.key;
        if (pos.row >= MATRIX_ROWS || pos.col >= MATRIX_COLS) {
            return false; // コンボ等で合成されたイベントは対象外
        }
        if (record->event.pressed) {
            auto_layer_disable();
            lockout_active = true;
            lockout_timer  = timer_read32();
            tdg_press(pos);
        } else {
            tdg_release(pos);
        }
        return false;
    }

    if (!record->event.pressed) {
        return true;
    }
    tdg_flush_wait(); // 他のキーが押されたら、次のタップ待ちのTDGは確定する
    if (key_keeps_auto_layer(keycode, record)) {
        if (auto_cur != AUTO_LAYER_NONE) {
            auto_timer = timer_read32(); // クリック中などは維持
        }
        return true;
    }
    // 通常の打鍵: 自動レイヤー解除 + 一定時間は再ONを抑止
    auto_layer_disable();
    lockout_active = true;
    lockout_timer  = timer_read32();
    return true;
}

void housekeeping_task_user(void) {
    if (!is_keyboard_master()) {
        return;
    }
    if (auto_cur != AUTO_LAYER_NONE && timer_elapsed32(auto_timer) >= (uint32_t)cfg.auto_timeout * 10) {
        auto_layer_disable();
    }
    tdg_task();
    numime_task();
    if (cyc_key != CYC_NONE && cfg.cyc_timeout && timer_elapsed32(cyc_timer) >= (uint32_t)cfg.cyc_timeout * 10) {
        cyc_finish(true); // 時間切れ: 確定
    }
}

/* ------------------------------------------------------------------
 * 非線形加速(マウス動作のボール)
 *   速さ(カウント/秒)に応じて倍率を 1.0 → 最大倍率 に変化させる。
 *   t = (速さ - 低速閾値) / (高速閾値 - 低速閾値)  を 0〜1 に丸め、
 *   倍率 = 1 + (最大 - 1) * t^カーブ
 * ------------------------------------------------------------------ */
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
static void accel_apply(ball_t *b, int16_t *x, int16_t *y) {
    int16_t  ax = abs(*x), ay = abs(*y);
    uint32_t mag = (ax > ay) ? (ax + (ay >> 1)) : (ay + (ax >> 1)); // 簡易ノルム
    b->accel_win_sum += mag;

    uint32_t now = timer_read32();
    uint32_t dt  = TIMER_DIFF_32(now, b->accel_win_start);
    if (dt >= ACCEL_WINDOW_MS) {
        b->accel_gain_q10  = accel_calc_gain_q10((b->accel_win_sum * 1000) / dt);
        b->accel_win_sum   = 0;
        b->accel_win_start = now;
    }

    // 端数を持ち越しながら倍率を掛ける
    b->accel_frac_x += (int32_t)*x * b->accel_gain_q10;
    b->accel_frac_y += (int32_t)*y * b->accel_gain_q10;
    int32_t ox = b->accel_frac_x / 1024;
    int32_t oy = b->accel_frac_y / 1024;
    b->accel_frac_x -= ox * 1024;
    b->accel_frac_y -= oy * 1024;
    *x = ox;
    *y = oy;
}

/* ------------------------------------------------------------------
 * 軸の向き(回転 5度刻み + X/Y反転)
 *   回転角 θ: x' = x*cosθ + y*sinθ,  y' = -x*sinθ + y*cosθ  (θ=90 で x'=y, y'=-x = ROTATION_90 と同じ)
 *   回転 → 反転 の順に適用
 * ------------------------------------------------------------------ */
// sin(5度 * i) * 1024 (i = 0〜71)
static const int16_t sin_q10[72] = {
    0, 89, 178, 265, 350, 433, 512, 587, 658, 724, 784, 839,
    887, 928, 962, 989, 1008, 1020, 1024, 1020, 1008, 989, 962, 928,
    887, 839, 784, 724, 658, 587, 512, 433, 350, 265, 178, 89,
    0, -89, -178, -265, -350, -433, -512, -587, -658, -724, -784, -839,
    -887, -928, -962, -989, -1008, -1020, -1024, -1020, -1008, -989, -962, -928,
    -887, -839, -784, -724, -658, -587, -512, -433, -350, -265, -178, -89,
};


static void orient_apply(int16_t *x, int16_t *y, uint8_t rot, bool inv_x, bool inv_y, rot_frac_t *f) {
    if (rot != 0) {
        int32_t s = sin_q10[rot % 72];
        int32_t c = sin_q10[(rot + 18) % 72]; // cos θ = sin(θ + 90度)
        f->fx += (int32_t)*x * c + (int32_t)*y * s;
        f->fy += -(int32_t)*x * s + (int32_t)*y * c;
        int32_t ox = f->fx / 1024;
        int32_t oy = f->fy / 1024;
        f->fx -= ox * 1024;
        f->fy -= oy * 1024;
        *x = ox;
        *y = oy;
    }
    if (inv_x) {
        *x = -*x;
    }
    if (inv_y) {
        *y = -*y;
    }
}

/* ------------------------------------------------------------------
 * 直進補正(マウス動作のボール)
 *   直近の窓(straight_window ms)の移動を見て、軸に近い角度(straight_angle 度以内)なら
 *   副軸の動きを捨てて軸に揃える。ロックは straight_lock×10 ms だけで、経過したら
 *   直近の窓で再判定する(動きが止まる前でも別の向きに切り替わる)。
 *   判定: 副軸 / 主軸 < tan(角度)   (角度そのものは求めず、比で比較)
 * ------------------------------------------------------------------ */
// tan(度) * 1024 (0〜45度)
static const uint16_t tan_q10[46] = {
    0, 18, 36, 54, 72, 90, 108, 126, 144, 162, 181, 199,
    218, 236, 255, 274, 294, 313, 333, 353, 373, 393, 414, 435,
    456, 477, 499, 522, 544, 568, 591, 615, 640, 665, 691, 717,
    744, 772, 800, 829, 859, 890, 922, 955, 989, 1024,
};

#define STRAIGHT_MIN_COUNTS 4 // 窓の移動量がこれ未満なら判定しない
#define STRAIGHT_IDLE_MS 100  // この時間動きがなければロック解除

static uint8_t straight_judge(ball_t *b) {
    uint32_t x = b->st_last_x, y = b->st_last_y;
    if (x + y < STRAIGHT_MIN_COUNTS) {
        return 0;
    }
    uint32_t t = tan_q10[cfg.straight_angle];
    if (y * 1024 < x * t) {
        return 1;
    }
    if (x * 1024 < y * t) {
        return 2;
    }
    return 0;
}

static void straighten(ball_t *b, int16_t *x, int16_t *y) {
    if (cfg.straight_angle == 0) {
        b->st_lock = 0;
        return;
    }
    uint32_t now = timer_read32();
    // しばらく動いていなかったら、前回のロックと集計を捨てる(動き出しの判定より先に行う)
    if (TIMER_DIFF_32(now, b->st_last_motion) > STRAIGHT_IDLE_MS) {
        b->st_lock      = 0;
        b->st_win_x     = 0;
        b->st_win_y     = 0;
        b->st_last_x    = 0;
        b->st_last_y    = 0;
        b->st_win_start = now;
    }
    if (*x || *y) {
        b->st_last_motion = now;
        b->st_win_x += abs(*x);
        b->st_win_y += abs(*y);
    }
    // 窓の更新(ロック中も生の動きを集計し続ける)
    if (TIMER_DIFF_32(now, b->st_win_start) >= cfg.straight_window) {
        b->st_last_x    = b->st_win_x;
        b->st_last_y    = b->st_win_y;
        b->st_win_x     = 0;
        b->st_win_y     = 0;
        b->st_win_start = now;
        if (b->st_lock == 0) {
            b->st_lock       = straight_judge(b);
            b->st_lock_start = now;
        }
    }
    // ロック時間が過ぎたら直近の窓で再判定
    if (b->st_lock && TIMER_DIFF_32(now, b->st_lock_start) >= (uint32_t)cfg.straight_lock * 10) {
        b->st_lock       = straight_judge(b);
        b->st_lock_start = now;
    }
    if (b->st_lock == 1) {
        *y = 0;
    } else if (b->st_lock == 2) {
        *x = 0;
    }
}

/* ------------------------------------------------------------------
 * 最終出力の倍率(マウス動作のボール)。ウルトラワイドで横を速くしたい等
 * ------------------------------------------------------------------ */
static void output_scale(ball_t *b, int16_t *x, int16_t *y) {
    int32_t xp = (int32_t)cfg.x_scale * 2; // %
    int32_t yp = (int32_t)cfg.y_scale * 2;
    if (xp == 100 && yp == 100) {
        b->scale_frac_x = b->scale_frac_y = 0;
        return;
    }
    b->scale_frac_x += *x * xp;
    b->scale_frac_y += *y * yp;
    int32_t ox = b->scale_frac_x / 100;
    int32_t oy = b->scale_frac_y / 100;
    b->scale_frac_x -= ox * 100;
    b->scale_frac_y -= oy * 100;
    *x = ox;
    *y = oy;
}

/* ------------------------------------------------------------------
 * スクロールを縦横のみにする(斜めを検出しない)
 *   窓(SCRAXIS_WINDOW_MS)の動きが大きい軸にロックし、SCRAXIS_HOLD_MS ごとに再判定する。
 *   ロックが決まるまでは、1回ごとに大きい軸だけを通す。
 * ------------------------------------------------------------------ */
static uint8_t sa_pick(ball_t *b) {
    if (b->sa_last_x + b->sa_last_y == 0) {
        return 0;
    }
    return (b->sa_last_x >= b->sa_last_y) ? 1 : 2;
}

static void scroll_axis_only(ball_t *b, int16_t *x, int16_t *y) {
    uint32_t now = timer_read32();
    if (TIMER_DIFF_32(now, b->sa_last_motion) > SCRAXIS_IDLE_MS) {
        b->sa_lock      = 0;
        b->sa_win_x     = 0;
        b->sa_win_y     = 0;
        b->sa_last_x    = 0;
        b->sa_last_y    = 0;
        b->sa_win_start = now;
    }
    if (*x || *y) {
        b->sa_last_motion = now;
        b->sa_win_x += abs(*x);
        b->sa_win_y += abs(*y);
    }
    if (TIMER_DIFF_32(now, b->sa_win_start) >= SCRAXIS_WINDOW_MS) {
        b->sa_last_x    = b->sa_win_x;
        b->sa_last_y    = b->sa_win_y;
        b->sa_win_x     = 0;
        b->sa_win_y     = 0;
        b->sa_win_start = now;
        if (b->sa_lock == 0) {
            b->sa_lock       = sa_pick(b);
            b->sa_lock_start = now;
        }
    }
    if (b->sa_lock && TIMER_DIFF_32(now, b->sa_lock_start) >= SCRAXIS_HOLD_MS) {
        b->sa_lock       = sa_pick(b);
        b->sa_lock_start = now;
    }
    uint8_t axis = b->sa_lock;
    if (axis == 0) { // ロック前: 1回ごとに大きい軸だけ
        axis = (abs(*x) >= abs(*y)) ? 1 : 2;
    }
    if (axis == 1) {
        *y = 0;
    } else {
        *x = 0;
    }
}

/* ------------------------------------------------------------------
 * ジェスチャー(4方向のみ)
 *   動きを累積し、大きい軸の方向に 1打鍵ぶんの距離(ゆっくり/速いで別設定)進むたびに
 *   ジェスチャー表(レイヤー)の対応する位置のキーを1回打鍵する。ゆっくり動かし続ければ連続入力になる。
 *   速い動きは単発(1回打鍵したら、動きが止まるまで打鍵しない)。cfg.gesture_fast_repeat=1 で連続にもできる。
 *   速さが 「速い」しきい値以上なら 速い方向キー、未満なら ゆっくり方向キー(仮想キー。vk_gesture 参照)を使う。
 * ------------------------------------------------------------------ */
// 方向: 0:上 1:下 2:左 3:右
static void gesture_emit(bool fast, uint8_t dir) {
    uint16_t kc = vk_lookup_key(vk_gesture[fast ? 1 : 0][dir]);
    if (kc > KC_TRNS) {
        kc_tap(kc);
    }
}

static void gesture_apply(ball_t *b, int16_t x, int16_t y) {
    uint32_t now = timer_read32();
    if (TIMER_DIFF_32(now, b->g_last_motion) > GESTURE_IDLE_MS) {
        b->g_x = b->g_y = 0;
        b->g_fast_done  = false;
        b->g_spd        = 0;
        b->g_spd_sum    = 0;
        b->g_spd_start  = now;
    }
    if (x == 0 && y == 0) {
        return;
    }
    b->g_last_motion = now;
    // 速さ(カウント/秒)を窓ごとに更新
    b->g_spd_sum += abs(x) + abs(y);
    uint32_t dt = TIMER_DIFF_32(now, b->g_spd_start);
    if (dt >= GESTURE_SPEED_WINDOW_MS) {
        b->g_spd       = (b->g_spd_sum * 1000) / dt;
        b->g_spd_sum   = 0;
        b->g_spd_start = now;
        dt             = 0;
    }
    // 判定に使う速さ: 前の窓の速さと、今の窓の途中経過の大きい方
    //   (動き出しの一打目が「ゆっくり」と誤判定されないよう、途中経過も見る)
    uint32_t spd_now = (b->g_spd_sum * 1000) / (dt < GESTURE_SPEED_MIN_DT_MS ? GESTURE_SPEED_MIN_DT_MS : dt);
    uint32_t spd     = b->g_spd > spd_now ? b->g_spd : spd_now;
    // 速いジェスチャーで打鍵済みなら、動きが止まるまで何もしない(減速中の「ゆっくり」判定の誤爆も防ぐ)
    if (b->g_fast_done) {
        b->g_x = b->g_y = 0;
        return;
    }
    // 距離を累積(方向は、累積した動きの向きで4方向のどれか1つに決める)
    int32_t gx = b->g_x + x, gy = b->g_y + y;
    if (gx > 32000) gx = 32000;
    if (gx < -32000) gx = -32000;
    if (gy > 32000) gy = 32000;
    if (gy < -32000) gy = -32000;
    for (uint8_t n = 0; n < GESTURE_MAX_EMIT; n++) {
        bool     fast = spd >= (uint32_t)cfg.gesture_speed * 10;
        int32_t  step = fast ? cfg.gesture_step_fast : cfg.gesture_step_slow;
        // 方向: 動きの向きで4分割(上=0°として 315°〜45°=上 / 45°〜135°=右 / 135°〜225°=下 / 225°〜315°=左。
        //       ちょうど45°ちょうどは左右)。距離: 斜めでも損しないよう、ベクトルの長さ(近似)で測る
        int32_t  ax    = abs(gx), ay = abs(gy);
        bool     horiz = ax >= ay;
        int32_t  mag   = horiz ? ax + (ay * 3) / 8 : ay + (ax * 3) / 8;
        if (mag < step) {
            break;
        }
        if (horiz) {
            gesture_emit(fast, gx > 0 ? 3 : 2);
            gx += (gx > 0) ? -step : step;
            gy = 0;
        } else {
            gesture_emit(fast, gy > 0 ? 1 : 0);
            gy += (gy > 0) ? -step : step;
            gx = 0;
        }
        if (fast && !cfg.gesture_fast_repeat) {
            b->g_fast_done = true; // 単発: 動きが止まるまでこれ以上打鍵しない
            gx = gy = 0;
            break;
        }
    }
    b->g_x = gx;
    b->g_y = gy;
}

/* ------------------------------------------------------------------
 * 両ボール同時操作(常時有効)
 *   左右のボールが同時に動いたとき(それぞれの機能は止めて)次の操作として扱う:
 *     左右が離れる/寄る(左を左・右を右 / 左を右・右を左) → Ctrl+ホイールで拡大/縮小
 *     同時に上 または 同時に左 → Home      同時に下 または 同時に右 → End
 * ------------------------------------------------------------------ */
#define JOINT_WINDOW_MS 100 // この時間以内に両方動いたら「同時」
#define JOINT_MIN 10        // 同時と見なすのに必要な、各ボールの移動量
#define JOINT_IDLE_MS 200   // 両方がこの時間止まったら操作の終わり
#define JOINT_DECIDE 60     // 向きの判定に必要な移動量(合計)
#define JOINT_ZOOM_STEP 30  // ホイール1目盛りぶんの、左右の離れ具合
enum { JT_NONE = 0, JT_ZOOM, JT_DONE };

static struct {
    int32_t  a[2][2]; // [ボール][x,y] の累積
    uint32_t last[2];
    bool     active;
    uint8_t  type;
    int32_t  zacc;
    bool     ctrl;
} joint;

static void joint_reset(void) {
    memset(joint.a, 0, sizeof(joint.a));
    joint.active = false;
    joint.type   = JT_NONE;
    joint.zacc   = 0;
    if (joint.ctrl) {
        unregister_code(KC_LCTL);
        joint.ctrl = false;
    }
}

// x[2], y[2]: 各ボールの移動量。返り値: 同時操作中か。*wheel にホイール量を返す
static bool joint_update(const int16_t *x, const int16_t *y, int8_t *wheel) {
    uint32_t now = timer_read32();
    *wheel       = 0;
    for (uint8_t i = 0; i < 2; i++) {
        if (x[i] || y[i]) {
            if (!joint.active && TIMER_DIFF_32(now, joint.last[i]) > JOINT_WINDOW_MS) {
                joint.a[i][0] = joint.a[i][1] = 0;
            }
            joint.a[i][0] += x[i];
            joint.a[i][1] += y[i];
            joint.last[i] = now;
        }
    }
    if (!joint.active) {
        bool both = true;
        for (uint8_t i = 0; i < 2; i++) {
            both = both && TIMER_DIFF_32(now, joint.last[i]) <= JOINT_WINDOW_MS && (abs(joint.a[i][0]) + abs(joint.a[i][1])) >= JOINT_MIN;
        }
        if (!both) {
            return false;
        }
        joint.active = true;
        joint.type   = JT_NONE;
        joint.zacc   = 0;
    }
    if (TIMER_DIFF_32(now, joint.last[0]) > JOINT_IDLE_MS && TIMER_DIFF_32(now, joint.last[1]) > JOINT_IDLE_MS) {
        joint_reset();
        return false;
    }
    int32_t H = joint.a[0][0] + joint.a[1][0]; // 同じ向きの横
    int32_t V = joint.a[0][1] + joint.a[1][1]; // 同じ向きの縦(下が+)
    int32_t Z = joint.a[1][0] - joint.a[0][0]; // 離れる(+) / 寄る(-)
    int32_t aH = abs(H), aV = abs(V), aZ = abs(Z);
    if (joint.type == JT_NONE) {
        int32_t m = aH > aV ? aH : aV;
        if (aZ > m ? aZ >= JOINT_DECIDE : m >= JOINT_DECIDE) {
            if (aZ > m) {
                joint.type = JT_ZOOM;
                if (!joint.ctrl) {
                    register_code(KC_LCTL);
                    joint.ctrl = true;
                }
                joint.zacc = Z; // 判定までの分も使う
            } else {
                bool home = (aH >= aV) ? (H < 0) : (V < 0);
                kc_tap(home ? KC_HOME : KC_END);
                joint.type = JT_DONE;
            }
        }
    } else if (joint.type == JT_ZOOM) {
        joint.zacc += x[1] - x[0];
    }
    if (joint.type == JT_ZOOM) {
        int32_t st = joint.zacc / JOINT_ZOOM_STEP;
        joint.zacc -= st * JOINT_ZOOM_STEP;
        *wheel = clamp8((int16_t)st);
    }
    return true;
}

/* ------------------------------------------------------------------
 * トラックボール処理
 *   各ボールを、設定された動作(マウス / スクロール / ジェスチャー)で処理する
 * ------------------------------------------------------------------ */
report_mouse_t pointing_device_task_combined_user(report_mouse_t left_report, report_mouse_t right_report) {
    // ---- 左右入れ替え(センサー → 『左ボール/右ボール』の役割) ----
    if (cfg.invert_flags & INV_SWAP) {
        report_mouse_t tmp = left_report;
        left_report        = right_report;
        right_report       = tmp;
    }
    report_mouse_t out[2] = {left_report, right_report};

    if (lockout_active && timer_elapsed32(lockout_timer) >= AUTO_LAYER_LOCKOUT_MS) {
        lockout_active = false;
    }

    // ---- 軸の向き(回転・反転) ----
    int16_t xs[2], ys[2];
    for (uint8_t i = 0; i < 2; i++) {
        bool left = (i == 0);
        xs[i] = out[i].x;
        ys[i] = out[i].y;
        orient_apply(&xs[i], &ys[i], left ? cfg.rot_left : cfg.rot_right, cfg.invert_flags & (left ? INV_LX : INV_RX), cfg.invert_flags & (left ? INV_LY : INV_RY), &ball[i].rot);
    }
    // ---- 両ボール同時操作(拡大縮小 / Home / End) ----
    int8_t jwheel = 0;
    bool   joint_on = joint_update(xs, ys, &jwheel);
    if (joint_on) {
        for (uint8_t i = 0; i < 2; i++) {
            out[i].x = out[i].y = out[i].h = out[i].v = 0;
        }
        out[1].v = jwheel;
        return pointing_device_combine_reports(out[0], out[1]);
    }

    for (uint8_t i = 0; i < 2; i++) {
        ball_t *b = &ball[i];
        int16_t x = xs[i], y = ys[i];

        // ---- 自動レイヤー切替(ボールごとに番号を設定) ----
        bool trig = activity_update(&b->act, x, y);
        if (auto_cur != AUTO_LAYER_NONE && (x || y)) {
            auto_timer = timer_read32(); // 動いている間は維持
        }
        {
            uint16_t fkc = vk_lookup_fn(vk_fn[i]);
            if (!lockout_active && trig && IS_QK_MOMENTARY(fkc)) {
                uint8_t tl = QK_MOMENTARY_GET_LAYER(fkc);
                if (tl != 0 && tl < LAYER_TOTAL) {
                    auto_layer_enable(tl);
                }
            }
        }

        out[i].x = 0;
        out[i].y = 0;
        out[i].h = 0;
        out[i].v = 0;

        uint8_t mode = ball_function(i);
        if (mode != ball_cur_mode[i]) {
            ball_apply_cpi(i, mode); // 機能が変わったらCPIも切り替える
        }
        switch (mode) {
            case MODE_SCROLL: {
                if (cfg.invert_flags & SCR_AXIS_ONLY) {
                    scroll_axis_only(b, &x, &y);
                }
                uint8_t div = cfg.scroll_div[cfg.scroll_index];
                b->scroll_acc_h += (cfg.invert_flags & SCR_INV_H) ? -x : x;
                b->scroll_acc_v += (cfg.invert_flags & SCR_INV_V) ? y : -y;
                int16_t h = b->scroll_acc_h / div;
                int16_t v = b->scroll_acc_v / div;
                b->scroll_acc_h -= h * div;
                b->scroll_acc_v -= v * div;
                out[i].h = clamp8(h);
                out[i].v = clamp8(v);
                break;
            }
            case MODE_GESTURE:
                gesture_apply(b, x, y);
                break;
            case MODE_AUTO: // レイヤーが切り替わるまで動かさない
                break;
            default: { // MODE_MOUSE / MODE_PRECISE
                if (mode == MODE_PRECISE) {
                    // 精密: 縦横ロック + 分周
                    if (cfg.precision_lock) {
                        if (x || y) {
                            b->axis_timer = timer_read32();
                            if (b->axis_lock == 0) {
                                b->axis_sum_x += x;
                                b->axis_sum_y += y;
                                if (abs(b->axis_sum_x) + abs(b->axis_sum_y) >= PRECISION_AXIS_DECIDE) {
                                    b->axis_lock = (abs(b->axis_sum_x) >= abs(b->axis_sum_y)) ? 1 : 2;
                                    x            = b->axis_sum_x; // 判定までの移動量を引き継ぐ
                                    y            = b->axis_sum_y;
                                } else {
                                    x = 0;
                                    y = 0;
                                }
                            }
                        }
                        if (timer_elapsed32(b->axis_timer) > PRECISION_AXIS_RELEASE_MS) {
                            b->axis_lock  = 0;
                            b->axis_sum_x = 0;
                            b->axis_sum_y = 0;
                        }
                        if (b->axis_lock == 1) {
                            y = 0;
                        } else if (b->axis_lock == 2) {
                            x = 0;
                        }
                    }
                    b->prec_acc_x += x;
                    b->prec_acc_y += y;
                    x = b->prec_acc_x / cfg.precision_div;
                    y = b->prec_acc_y / cfg.precision_div;
                    b->prec_acc_x -= x * cfg.precision_div;
                    b->prec_acc_y -= y * cfg.precision_div;
                    b->accel_win_sum  = 0; // 精密中は加速状態を持ち越さない
                    b->accel_gain_q10 = 1024;
                    b->st_lock        = 0;
                } else {
                    b->prec_acc_x = b->prec_acc_y = 0;
                    b->axis_lock  = 0;
                    b->axis_sum_x = b->axis_sum_y = 0;
                    // 通常: 直進補正 → 非線形加速
                    straighten(b, &x, &y);
                    accel_apply(b, &x, &y);
                }
                // 最終出力の倍率(精密レイヤー中も有効)
                output_scale(b, &x, &y);
                out[i].x = clamp8(x);
                out[i].y = clamp8(y);
                break;
            }
        }
    }

    return pointing_device_combine_reports(out[0], out[1]);
}

void keyboard_post_init_user(void) {
    balls_init();
    cfg_load();
    if (is_keyboard_master()) {
        apply_cpi();
    }
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
    id_rot_left            = 12, // dropdown 0〜71 (×5度)
    id_rot_right           = 13, // dropdown 0〜71 (×5度)
    id_invert_lx           = 14, // toggle
    id_invert_ly           = 15, // toggle
    id_invert_rx           = 16, // toggle
    id_invert_ry           = 17, // toggle
    id_swap_lr             = 18, // toggle: センサーの左右入れ替え
    id_scroll_inv_h        = 19, // toggle: スクロール左右反転
    id_scroll_inv_v        = 20, // toggle: スクロール上下反転
    id_straight_angle      = 21, // range 8bit  (度, 0=無効)
    id_straight_window     = 22, // range 8bit  (ms)
    id_straight_lock       = 23, // range 16bit (ms)
    id_x_scale             = 24, // range 16bit (%)
    id_y_scale             = 25, // range 16bit (%)
    id_mode_left           = 26, // dropdown 0:マウス 1:スクロール 2:ジェスチャー
    id_mode_right          = 27, // dropdown
    id_scroll_axis_only    = 28, // toggle: スクロールを縦横のみ
    id_auto_timeout        = 29, // range 16bit (ms)
    id_auto_layer_left     = 30, // dropdown (255=なし)
    id_auto_layer_right    = 31, // dropdown (255=なし)
    id_gesture_layer_left  = 32, // dropdown
    id_gesture_layer_right = 33, // dropdown
    id_gesture_speed       = 34, // range 16bit (カウント/秒)
    id_gesture_step_slow   = 35, // range 8bit (カウント)
    id_gesture_step_fast   = 36, // range 8bit (カウント)
    id_tdg_term            = 37, // range 16bit (ms)
    id_gesture_fast_repeat = 38, // toggle: 速いジェスチャーも連続入力にする
    id_cyc_timeout         = 39, // range 16bit (ms) 0=自動確定しない
    // 40〜63: 予備
    id_pick_sym_base       = 64,  // 64〜119: 記号ピッカー ①〜⑦ の1〜8番の記号 (64 + セット*8 + 番号) dropdown
    id_cyc_sym_base        = 128, // 128〜191: 切替入力 CYC1〜8 の1〜8番目の記号 (128 + キー*8 + 番号) dropdown
    id_pick_sel_base       = 200, // 200〜207: 記号ピッカーの1〜8番の選択キー dropdown
};

static void set_flag(uint8_t mask, uint8_t on) {
    if (on) {
        cfg.invert_flags |= mask;
    } else {
        cfg.invert_flags &= ~mask;
    }
}

// 値のバイト並び
//   VIA custom UI for Vial : 値は常にリトルエンディアン32bit(4バイト)
//   VIA 本家               : 8bit、または ビッグエンディアン16bit
static bool tb_le = false;

static inline uint16_t rd16(const uint8_t *p) {
    return tb_le ? (uint16_t)(p[0] | (p[1] << 8)) : (uint16_t)((p[0] << 8) | p[1]);
}
static inline void wr16(uint8_t *p, uint16_t v) {
    if (tb_le) {
        p[0] = v & 0xFF;
        p[1] = v >> 8;
        p[2] = 0;
        p[3] = 0;
    } else {
        p[0] = v >> 8;
        p[1] = v & 0xFF;
    }
}
static inline void wr8(uint8_t *p, uint8_t v) {
    p[0] = v;
    if (tb_le) {
        p[1] = 0;
        p[2] = 0;
        p[3] = 0;
    }
}

// d = [value_id, value_data...]
static void tb_set_value(uint8_t *d) {
    uint8_t *v = &d[1];
    // 記号ピッカー / 切替入力 / 選択キー(記号表の番号)
    if (d[0] >= id_pick_sym_base && d[0] < id_pick_sym_base + PICK_SETS * PICK_ITEMS) {
        uint8_t n = d[0] - id_pick_sym_base;
        cfg.pick_sym[n / PICK_ITEMS][n % PICK_ITEMS] = v[0] < SYM_COUNT ? v[0] : 0;
        return;
    }
    if (d[0] >= id_cyc_sym_base && d[0] < id_cyc_sym_base + CYC_KEYS * CYC_MAX) {
        uint8_t n = d[0] - id_cyc_sym_base;
        cfg.cyc_sym[n / CYC_MAX][n % CYC_MAX] = v[0] < SYM_COUNT ? v[0] : 0;
        return;
    }
    if (d[0] >= id_pick_sel_base && d[0] < id_pick_sel_base + PICK_ITEMS) {
        cfg.pick_sel[d[0] - id_pick_sel_base] = v[0] < SEL_COUNT ? v[0] : 0;
        return;
    }
    switch (d[0]) {
        case id_cpi_index:
            cfg.cpi_index = v[0];
            cfg_sanitize();
            apply_cpi();
            break;
        case id_cpi_slot:
            if (v[0] < SLOT_COUNT) {
                uint16_t q = (rd16(&v[1]) + CPI_Q_STEP / 2) / CPI_Q_STEP;
                cfg.cpi_q[v[0]] = q > 127 ? 127 : (q < 16 ? 16 : q);
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
            cfg.accel_max_gain = rd16(v);
            cfg_sanitize();
            break;
        case id_accel_curve:
            cfg.accel_curve = v[0];
            cfg_sanitize();
            break;
        case id_accel_low_speed:
            cfg.accel_low_speed = rd16(v);
            cfg_sanitize();
            break;
        case id_accel_high_speed:
            cfg.accel_high_speed = rd16(v);
            cfg_sanitize();
            break;
        case id_precision_div:
            cfg.precision_div = v[0];
            cfg_sanitize();
            break;
        case id_precision_lock:
            cfg.precision_lock = v[0] ? 1 : 0;
            break;
        case id_rot_left:
            cfg.rot_left = v[0];
            cfg_sanitize();
            break;
        case id_rot_right:
            cfg.rot_right = v[0];
            cfg_sanitize();
            break;
        case id_invert_lx:
            set_flag(INV_LX, v[0]);
            break;
        case id_invert_ly:
            set_flag(INV_LY, v[0]);
            break;
        case id_invert_rx:
            set_flag(INV_RX, v[0]);
            break;
        case id_invert_ry:
            set_flag(INV_RY, v[0]);
            break;
        case id_swap_lr:
            set_flag(INV_SWAP, v[0]);
            apply_cpi();
            break;
        case id_scroll_inv_h:
            set_flag(SCR_INV_H, v[0]);
            break;
        case id_scroll_inv_v:
            set_flag(SCR_INV_V, v[0]);
            break;
        case id_scroll_axis_only:
            set_flag(SCR_AXIS_ONLY, v[0]);
            break;
        case id_mode_left:
            cfg.mode_left = v[0];
            cfg_sanitize();
            apply_cpi();
            break;
        case id_mode_right:
            cfg.mode_right = v[0];
            cfg_sanitize();
            apply_cpi();
            break;
        case id_auto_timeout: {
            uint16_t q = rd16(v) / 10;
            cfg.auto_timeout = q > 255 ? 255 : q;
            cfg_sanitize();
            break;
        }
        case id_auto_layer_left:
            cfg.auto_layer_left = v[0];
            cfg_sanitize();
            break;
        case id_auto_layer_right:
            cfg.auto_layer_right = v[0];
            cfg_sanitize();
            break;
        case id_gesture_layer_left:
            cfg.gesture_layer_left = v[0];
            cfg_sanitize();
            break;
        case id_gesture_layer_right:
            cfg.gesture_layer_right = v[0];
            cfg_sanitize();
            break;
        case id_gesture_speed: {
            uint16_t q = rd16(v) / 10;
            cfg.gesture_speed = q > 255 ? 255 : q;
            cfg_sanitize();
            break;
        }
        case id_gesture_step_slow:
            cfg.gesture_step_slow = v[0];
            cfg_sanitize();
            break;
        case id_gesture_step_fast:
            cfg.gesture_step_fast = v[0];
            cfg_sanitize();
            break;
        case id_gesture_fast_repeat:
            cfg.gesture_fast_repeat = v[0] ? 1 : 0;
            break;
        case id_cyc_timeout: {
            uint16_t q = rd16(v) / 10;
            cfg.cyc_timeout = q > 255 ? 255 : q;
            break;
        }
        case id_tdg_term: {
            uint16_t q = rd16(v) / 10;
            cfg.tdg_term = q > 255 ? 255 : q;
            cfg_sanitize();
            break;
        }
        case id_straight_angle:
            cfg.straight_angle = v[0];
            cfg_sanitize();
            break;
        case id_straight_window:
            cfg.straight_window = v[0];
            cfg_sanitize();
            break;
        case id_straight_lock: {
            uint16_t q = rd16(v) / 10;
            cfg.straight_lock = q > 200 ? 200 : q;
            cfg_sanitize();
            break;
        }
        case id_x_scale: {
            uint16_t q = rd16(v) / 2;
            cfg.x_scale = q > 200 ? 200 : q;
            cfg_sanitize();
            break;
        }
        case id_y_scale: {
            uint16_t q = rd16(v) / 2;
            cfg.y_scale = q > 200 ? 200 : q;
            cfg_sanitize();
            break;
        }
    }
}

static void tb_get_value(uint8_t *d) {
    uint8_t *v = &d[1];
    if (d[0] >= id_pick_sym_base && d[0] < id_pick_sym_base + PICK_SETS * PICK_ITEMS) {
        uint8_t n = d[0] - id_pick_sym_base;
        wr8(v, cfg.pick_sym[n / PICK_ITEMS][n % PICK_ITEMS]);
        return;
    }
    if (d[0] >= id_cyc_sym_base && d[0] < id_cyc_sym_base + CYC_KEYS * CYC_MAX) {
        uint8_t n = d[0] - id_cyc_sym_base;
        wr8(v, cfg.cyc_sym[n / CYC_MAX][n % CYC_MAX]);
        return;
    }
    if (d[0] >= id_pick_sel_base && d[0] < id_pick_sel_base + PICK_ITEMS) {
        wr8(v, cfg.pick_sel[d[0] - id_pick_sel_base]);
        return;
    }
    switch (d[0]) {
        case id_cpi_index:
            wr8(v, cfg.cpi_index);
            break;
        case id_cpi_slot:
            if (v[0] < SLOT_COUNT) {
                wr16(&v[1], (uint16_t)cfg.cpi_q[v[0]] * CPI_Q_STEP);
            }
            break;
        case id_scroll_index:
            wr8(v, cfg.scroll_index);
            break;
        case id_scroll_slot:
            if (v[0] < SLOT_COUNT) {
                wr8(&v[1], cfg.scroll_div[v[0]]);
            }
            break;
        case id_accel_enable:
            wr8(v, cfg.accel_enable);
            break;
        case id_accel_max_gain:
            wr16(v, cfg.accel_max_gain);
            break;
        case id_accel_curve:
            wr8(v, cfg.accel_curve);
            break;
        case id_accel_low_speed:
            wr16(v, cfg.accel_low_speed);
            break;
        case id_accel_high_speed:
            wr16(v, cfg.accel_high_speed);
            break;
        case id_precision_div:
            wr8(v, cfg.precision_div);
            break;
        case id_precision_lock:
            wr8(v, cfg.precision_lock);
            break;
        case id_rot_left:
            wr8(v, cfg.rot_left);
            break;
        case id_rot_right:
            wr8(v, cfg.rot_right);
            break;
        case id_invert_lx:
            wr8(v, (cfg.invert_flags & INV_LX) ? 1 : 0);
            break;
        case id_invert_ly:
            wr8(v, (cfg.invert_flags & INV_LY) ? 1 : 0);
            break;
        case id_invert_rx:
            wr8(v, (cfg.invert_flags & INV_RX) ? 1 : 0);
            break;
        case id_invert_ry:
            wr8(v, (cfg.invert_flags & INV_RY) ? 1 : 0);
            break;
        case id_swap_lr:
            wr8(v, (cfg.invert_flags & INV_SWAP) ? 1 : 0);
            break;
        case id_scroll_inv_h:
            wr8(v, (cfg.invert_flags & SCR_INV_H) ? 1 : 0);
            break;
        case id_scroll_inv_v:
            wr8(v, (cfg.invert_flags & SCR_INV_V) ? 1 : 0);
            break;
        case id_scroll_axis_only:
            wr8(v, (cfg.invert_flags & SCR_AXIS_ONLY) ? 1 : 0);
            break;
        case id_mode_left:
            wr8(v, cfg.mode_left);
            break;
        case id_mode_right:
            wr8(v, cfg.mode_right);
            break;
        case id_auto_timeout:
            wr16(v, (uint16_t)cfg.auto_timeout * 10);
            break;
        case id_auto_layer_left:
            wr8(v, cfg.auto_layer_left);
            break;
        case id_auto_layer_right:
            wr8(v, cfg.auto_layer_right);
            break;
        case id_gesture_layer_left:
            wr8(v, cfg.gesture_layer_left);
            break;
        case id_gesture_layer_right:
            wr8(v, cfg.gesture_layer_right);
            break;
        case id_gesture_speed:
            wr16(v, (uint16_t)cfg.gesture_speed * 10);
            break;
        case id_gesture_step_slow:
            wr8(v, cfg.gesture_step_slow);
            break;
        case id_gesture_step_fast:
            wr8(v, cfg.gesture_step_fast);
            break;
        case id_gesture_fast_repeat:
            wr8(v, cfg.gesture_fast_repeat);
            break;
        case id_cyc_timeout:
            wr16(v, (uint16_t)cfg.cyc_timeout * 10);
            break;
        case id_tdg_term:
            wr16(v, (uint16_t)cfg.tdg_term * 10);
            break;
        case id_straight_angle:
            wr8(v, cfg.straight_angle);
            break;
        case id_straight_window:
            wr8(v, cfg.straight_window);
            break;
        case id_straight_lock:
            wr16(v, (uint16_t)cfg.straight_lock * 10);
            break;
        case id_x_scale:
            wr16(v, (uint16_t)cfg.x_scale * 2);
            break;
        case id_y_scale:
            wr16(v, (uint16_t)cfg.y_scale * 2);
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
                // 「保存」は全項目ぶん連続で送られてくるので、内容が変わったときだけ書く
                if (memcmp(&cfg, &cfg_saved, sizeof(cfg)) != 0) {
                    cfg_save();
                }
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
        tb_le = true; // VIA custom UI for Vial はリトルエンディアン32bit
        tb_custom_command(&data[1], length - 1);
    } else if (*command_id == id_custom_set_value || *command_id == id_custom_get_value || *command_id == id_custom_save) {
        tb_le = false; // VIA 本家形式
        tb_custom_command(data, length);
    }
}
