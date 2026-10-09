#ifdef VIA_ENABLE
#include "via.h"

// 関数のプロトタイプ宣言（コンパイルエラー対策）
void via_custom_value_command_kb(uint8_t *data, uint8_t length);

/* ==================================================================
 * 1. 既存のHID受信関数の中にカスタムUIの処理を統合
 * ================================================================== */
void raw_hid_receive_kb(uint8_t *data, uint8_t length) {
    uint8_t command_id = data[0];

    // --- ここにカスタムUI用の処理を挟み込む ---
    // 0x07 (Set) または 0x08 (Get) コマンドが来たらカスタム値を処理する
    if (command_id == 0x07 || command_id == 0x08) {
        via_custom_value_command_kb(data, length);
        host_raw_hid_send(data, length); // 古いQMK環境向けの送信関数
        return;
    }
    // ------------------------------------------

    // もし既存の raw_hid_receive_kb の中に、元々書いてあった処理（トラックボールのデバッグ用など）が
    // あれば、この下にそのまま残しておいてください。
}

/* ==================================================================
 * 2. WebUIとキーボード側構造体（cfg）の値を正しく仲介・格納する処理
 * ================================================================== */
void via_custom_value_command_kb(uint8_t *data, uint8_t length) {
    uint8_t command_id = data[0]; // 0x07: Set / 0x08: Get
    uint8_t value_id   = data[2]; // vial.json で設定した各項目の "id"
    uint8_t value_data = data[4]; // UI側から送られてくる1バイトの「値」

    // 0x07 (Set): UI側のスライダーやトグルを動かした時、キーボードの変数に保存する
    if (command_id == 0x07) {
        switch (value_id) {
            case 1: cfg.cpi_index     = value_data; apply_cpi(); break; // DPI変更時は即センサーに反映
            case 2: cfg.scroll_index  = value_data; break;
            case 3: cfg.accel_enable  = value_data; break;
            case 4: cfg.accel_curve   = value_data; break;
            case 5: cfg.precision_div = value_data; break;
            case 6: cfg.precision_lock= value_data; break;
            default: break;
        }
        cfg_sanitize(); // 値を安全な範囲に丸める
        cfg_save();     // EEPROMへ即座に永続保存
    } 
    // 0x08 (Get): ツール画面を開いた時、現在のキーボードの状態を画面に送り返す
    else if (command_id == 0x08) {
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
#endif
