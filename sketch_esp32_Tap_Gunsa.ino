/*
  Name               : TapTap Go with ESP32
  Author             : Emmanuel Galihwara
  DevStart Date      : 23 February 2025
  Version            : 0000
  -------------------------------------------------------------------------------------------------------------------------------------
*/

//Library Import
#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

//Pin Setup
#define SS_PIN 5
#define RST_PIN 0
#define BUZZER_PIN 27  // Pin untuk buzzer
#define DISCO_LED 25   // Pin untuk LED Disco
#define RED_LED 26     // Pin untuk RGB Merah
#define GREEN_LED 33   // Pin untuk RGB Hijau
#define BLUE_LED 32    // Pin untuk RGB Biru

MFRC522 rfid(SS_PIN, RST_PIN);

// WiFi & API credentials
const char* ssid = "Warastory";  // Nama WiFi
const char* password = "Oregon88"; // Password WiFi
const char* googleScriptURL = "https://script.google.com/macros/s/AKfycbw8IG52YDKQq7cvi-OAHRN1kHN5WPMN1DgpRLzTc7iat3XlkkSqJOM5VITRXGvWHL2m/exec";
const char* apiURL = "https://gkigunsa.id/neo/api/rest.php";
const char* apiKey = "GBj6OQ4rZKhkqChKkSv4v1mEujmb8ZpP0BoGKB2v8RbAEeTS3V";
const char* apiUsername = "cardreader";
const char* apiPassword = "M3l3nt!ngT1ngg!";

//Function Declaration
void setRGB(int red, int green, int blue);
void beep(int duration, int count);
void sendToGoogleSheet(String message, String rfidTag);
void sendToRukovoditel(String rfidTag);

void setup() {
    Serial.begin(115200);
    SPI.begin();
    rfid.PCD_Init();
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(DISCO_LED, OUTPUT);
    pinMode(RED_LED, OUTPUT);
    pinMode(GREEN_LED, OUTPUT);
    pinMode(BLUE_LED, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);  // Matikan buzzer saat awal
    digitalWrite(DISCO_LED, HIGH);  // LED Disco menyala saat proses koneksi WiFi
    setRGB(255, 255, 255);  // LED RGB Merah (WiFi belum terhubung)


    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi...");

    while (WiFi.status() != WL_CONNECTED) {
        digitalWrite(BUZZER_PIN, HIGH); // Buzzer berbunyi terus jika WiFi gagal
        digitalWrite(DISCO_LED, HIGH);  // LED Disco menyala saat proses gagal
        setRGB(255, 255, 255);  // LED RGB Merah (WiFi belum terhubung)
        delay(500);
        Serial.print(".");
    }

    digitalWrite(DISCO_LED, LOW);  // Matikan LED Disco setelah koneksi berhasil
    setRGB(0, 0, 255); // LED RGB Biru(WiFi terhubung)
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("\nWiFi connected.");
    Serial.println("Tap kartu RFID atau ketik pesan di Serial Monitor lalu tekan ENTER untuk mengirim.");
}

void loop() {
    // Cek koneksi WiFi
    if (WiFi.status() != WL_CONNECTED) {
        digitalWrite(DISCO_LED, HIGH);
        setRGB(255, 0, 0); // LED Merah jika WiFi terputus
        digitalWrite(BUZZER_PIN, HIGH); // Buzzer berbunyi terus jika WiFi mati
    } else {
        digitalWrite(DISCO_LED, LOW);
        digitalWrite(BUZZER_PIN, LOW);
    }

    // Cek apakah ada kartu RFID ditap
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        String rfidTag = "";
        for (byte i = 0; i < rfid.uid.size; i++) {
            rfidTag += String(rfid.uid.uidByte[i], HEX);
        }
        Serial.println("RFID Detected: " + rfidTag);

        setRGB(0, 0, 255); // LED RGB Biru (Kartu terdeteksi)
        beep(100, 1);
        delay(1000);
        setRGB(0, 0, 255); // LED RGB kembali standby

        // Kirim ID kartu ke Google Sheet dan API
        sendToGoogleSheet(rfidTag, rfidTag);
        sendToRukovoditel(rfidTag);

        // Hentikan komunikasi dengan kartu
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
        delay(2000);
    }
    delay(100);
}

void sendToGoogleSheet(String message, String rfidTag){
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(googleScriptURL);
        http.addHeader("Content-Type", "application/json");

        String jsonPayload = "{\"message\":\"" + message + "\"}";
        int httpResponseCode = http.POST(jsonPayload);

        if (httpResponseCode > 0) {
            Serial.println("Pesan berhasil disimpan di Google Sheet!");
            setRGB(0, 255, 0); // LED Hijau (Data sukses disimpan)
            beep(200, 2); // Nada sukses
            setRGB(0, 0, 255); // LED RGB kembali standby
        } else {
            Serial.println("Gagal menyimpan pesan! Error code: " + String(httpResponseCode));
            setRGB(255, 0, 0); // LED Merah (Data gagal disimpan)
            beep(500, 3); // Nada error panjang & berulang
            setRGB(0, 0, 255); // LED RGB kembali mati
        }
        
        http.end();
    } else {
        Serial.println("WiFi Disconnected!");
    }
}

void sendToRukovoditel(String rfidTag) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(apiURL);
        http.addHeader("Content-Type", "application/json");

        // Format JSON dengan username & password
        String jsonPayload = "{";
        jsonPayload += "\"key\":\"" + String(apiKey) + "\",";
        jsonPayload += "\"username\":\"" + String(apiUsername) + "\",";
        jsonPayload += "\"password\":\"" + String(apiPassword) + "\",";
        jsonPayload += "\"action\":\"insert\",";
        jsonPayload += "\"entity_id\":\"85\",";
        jsonPayload += "\"items\":[{";
        jsonPayload += "\"field_964\":\"" + rfidTag + "\"";
        jsonPayload += "}]}";

        Serial.println("Mengirim data ke server...");
        Serial.println("JSON Payload: " + jsonPayload);

        int httpResponseCode = http.POST(jsonPayload);

        Serial.print("Response Code: ");
        Serial.println(httpResponseCode);

        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.println("Server Response: " + response);
            setRGB(0, 255, 0); // LED Hijau (Data sukses disimpan)
            beep(200, 2); // Nada sukses
            setRGB(0, 0, 255); // LED RGB kembali standby
        } else {
            Serial.println("Gagal mengirim pesan.");
            setRGB(255, 0, 0); // LED Merah (Data gagal disimpan)
            beep(500, 3); // Nada error panjang & berulang
            setRGB(0, 0, 255); // LED RGB kembali mati
        }

        http.end();
    } else {
        Serial.println("WiFi Disconnected!");
    }
}

// Fungsi untuk membunyikan buzzer dengan durasi & jumlah beep
void beep(int duration, int count) {
    for (int i = 0; i < count; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(duration);
        digitalWrite(BUZZER_PIN, LOW);
        delay(100);
    }
}

// Fungsi pengaturan nyala lampu
void setRGB(int red, int green, int blue) {
    analogWrite(RED_LED, red);
    analogWrite(GREEN_LED, green);
    analogWrite(BLUE_LED, blue);
}