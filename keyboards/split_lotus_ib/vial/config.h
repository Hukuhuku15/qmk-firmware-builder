#pragma once

#define EE_HANDS
#define VIAL_KEYBOARD_UID {0xf5, 0xcd, 0x33, 0x0b, 0xb0, 0x86, 0x1b, 0x5f}

// Vial 動的エントリ数(タップダンス / コンボ)
#define VIAL_TAP_DANCE_ENTRIES 20
#define VIAL_COMBO_ENTRIES 20

// レイヤー数(0〜3:キーマップ 4:マウス 6:精密 7:ジェスチャー表 8〜11:TDG動作表)
#define DYNAMIC_KEYMAP_LAYER_COUNT 12

// TT(layer) のレイヤー切替に必要なタップ回数
#define TAPPING_TOGGLE 3

// トラックボール設定(動作・CPI/スクロール段階・加速・ジェスチャー等)の保存領域
#define EECONFIG_USER_DATA_SIZE 192

#define PICO_FLASH_SIZE_BYTES (1 * 1024 * 1024)
#define SPLIT_POINTING_ENABLE
#define POINTING_DEVICE_COMBINED

// 軸の回転・反転は keymap.c / Vialメニューで実行時に設定する
// (POINTING_DEVICE_ROTATION_* / INVERT_* は使わない)
