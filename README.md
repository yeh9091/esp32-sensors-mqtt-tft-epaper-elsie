# ESP32 Sensors, MQTT, TFT & E-Paper

這是一個以 **ESP32 + Arduino** 為核心的物聯網學習與整合專案，從基礎 GPIO、
LED、按鍵與蜂鳴器，逐步延伸到 DHT11 溫濕度、光線、PM2.5、OLED、TFT、
MQTT、ThingSpeak、Google Sheets、LINE 通知，以及 2.9 吋電子紙顯示。

Repository: `esp32-sensors-mqtt-tft-epaper-elsie`

## 專案展示網站

- GitHub Pages：<https://yeh9091.github.io/esp32-sensors-mqtt-tft-epaper-elsie/>
- 網站原始檔：[`index.html`](index.html)、[`site.css`](site.css)
- 成果照片、架構圖與報告：[`showcase-assets/`](showcase-assets/)

## 專案內容

專案根目錄中的範例依學習與整合階段編號：

- `01_hello` ～ `05_night_3_led`：ESP32 基礎輸出與 LED 練習
- `06_DHT` ～ `08_OLED`：DHT11、I2C 與 OLED 顯示
- `11_DHT_OLED_GYRBLED_ALERT` ～ `18_SONIC_OLED`：感測器、RGB LED、
  蜂鳴器、超音波與 OLED 整合
- `19_PM25` ～ `21_Weather_OLED`：空氣品質與氣象 API 顯示
- `22_dht_light_oled` ～ `25_dht_line`：溫濕度、光線、ThingSpeak、
  Google Sheets 與 LINE 通知
- `26_mqtt` ～ `27_mqtt_ctrl`：MQTT 感測資料與控制
- `28_ili9225` ～ `35_ili_mqtt_ctrl_page`：ILI9225 TFT 與網頁控制整合
- `36_epaper` ～ `38_mqtt_ctrl_page_epaper`：電子紙顯示與 MQTT / 網頁控制
- `libraries/`：本專案使用的 Arduino 函式庫
- `epaper_resources/`：電子紙模組的參考資料

## 硬體與功能

依不同範例使用下列元件：

- ESP32 開發板
- DHT11 溫濕度感測器
- 光敏電阻 / 類比光線感測器
- PM2.5 與氣象資料 API
- SSD1306 / U8g2 OLED
- ILI9225 TFT LCD
- 2.9 吋電子紙模組
- LED、RGB LED、蜂鳴器、超音波感測器

## 開始使用

1. 安裝 Arduino IDE 或相容的 ESP32 Arduino 開發環境。
2. 在 Arduino IDE 安裝 ESP32 board package。
3. 將 `libraries/` 下需要的函式庫複製或加入 Arduino libraries 目錄。
4. 開啟對應資料夾中的 `.ino` 檔案；Arduino sketch 資料夾名稱需與主 `.ino`
   檔案名稱相符。
5. 選擇正確的 ESP32 開發板與序列埠後編譯、上傳。

## 網路與第三方服務設定

需要 Wi-Fi、MQTT、ThingSpeak、政府開放資料、Google Apps Script 或 LINE
的範例，請先在本機設定自己的參數：

- Wi-Fi SSID / password
- ThingSpeak write API key
- MQTT broker、topic 與必要的帳號資訊
- 政府資料 API authorization / API key
- Google Apps Script / Sheet ID
- LINE Channel access token 與 user ID

為了避免公開 repository 洩漏憑證，原始碼中的敏感值已替換成 placeholder。
請勿把真實密碼、token 或 API key commit 到 Git；建議在本機使用未追蹤的
設定檔或透過編譯旗標注入設定。

## MQTT 範例

`26_mqtt`、`27_mqtt_ctrl` 與後續整合範例示範：

- ESP32 連線 Wi-Fi
- 發布溫度、濕度與光線資料
- 接收控制訊息
- 將感測結果呈現於 OLED / TFT / 電子紙
- 搭配網頁控制頁進行遠端操作

請依程式中的 `MQTT_HOST`、`MQTT_PORT`、`MQTT_TOPIC` 與控制 topic 說明，
改成自己的 broker 設定。

## 注意事項

- 本 repository 主要是課程與實作範例，不保證所有範例可直接在所有 ESP32
  板型與函式庫版本上編譯。
- 編譯產物與暫存報告渲染檔不納入版本控制，以避免 repository 過大。
- `showcase-assets/` 內的精選成果照片、架構圖與兩份成果報告會公開展示；
  請確認文件內容不含不宜公開的個人資料或機密資訊。
- 使用第三方函式庫時，請遵守各函式庫原本的授權條款。

## License

本專案未另行指定授權。除本專案程式碼外，`libraries/` 中的第三方函式庫
仍以各自的 LICENSE / 著作權聲明為準。
