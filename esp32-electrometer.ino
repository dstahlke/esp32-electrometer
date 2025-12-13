#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebSrv.h>
#include <DNSServer.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

DNSServer dnsServer;
AsyncWebServer server(80);
AsyncWebSocket webSocket("/ws");
AsyncWebSocketClient* wsClient;

static constexpr int log_num_pixels = 4;
static constexpr int num_pixels = 1 << log_num_pixels;
static constexpr int sensitivity = 7;

Adafruit_NeoPixel pixels(num_pixels, A2, NEO_GRB + NEO_KHZ800);

auto &SerialLog = Serial1;

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_CONNECT) {
        SerialLog.printf("ws[%s][%u] connect\n", server->url(), client->id());
        wsClient = client;
    } else if (type == WS_EVT_DISCONNECT) {
        SerialLog.printf("ws[%s][%u] disconnect\n", server->url(), client->id());
        wsClient = nullptr;
    } else if (type == WS_EVT_ERROR) {
        SerialLog.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
    }
}

void setup()
{
    SerialLog.begin(115200);
    SerialLog.setDebugOutput(true);
    SerialLog.println("\nbegin");

    if (!LittleFS.begin()) {
        SerialLog.println("error mounting LittleFS");
    }

    #if 1
    WiFi.softAP("electrometer");
    SerialLog.print("IP address: ");
    SerialLog.println(WiFi.softAPIP());
    dnsServer.start(53, "*", WiFi.softAPIP());
    #else
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        SerialLog.printf("wifi failed\n");
        return;
    }
    SerialLog.print("IP address: ");
    SerialLog.println(WiFi.localIP());
    #endif

    webSocket.onEvent(onEvent);
    server.addHandler(&webSocket);

    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });

    server.begin();

    pinMode(A3, INPUT_PULLUP);

    Wire.begin();
    Wire.setClock(1000000);

    Wire.beginTransmission(0x40);
    Wire.write(uint8_t(0x40)); // WREG CONFIG
    Wire.write(uint8_t(0b01100011));
    Wire.endTransmission();

    Wire.beginTransmission(0x40);
    Wire.write(uint8_t(0x08)); // START
    Wire.endTransmission();

    pixels.begin();
}

static uint8_t adc_read_config()
{
    Wire.beginTransmission(0x40);
    Wire.write(uint8_t(0x20)); // RREG CONFIG
    Wire.endTransmission();
    Wire.requestFrom(uint8_t(0x40), uint8_t(1));
    return Wire.read();
}

static uint8_t adc_read_status()
{
    Wire.beginTransmission(0x40);
    Wire.write(uint8_t(0x24)); // RREG STATUS
    Wire.endTransmission();
    Wire.requestFrom(uint8_t(0x40), uint8_t(1));
    return Wire.read();
}

static uint32_t adc_read_data()
{
    Wire.beginTransmission(0x40);
    Wire.write(uint8_t(0x10)); // RDATA
    Wire.endTransmission();
    Wire.requestFrom(uint8_t(0x40), uint8_t(3));
    uint32_t adc{};
    adc = (adc << 8) + uint8_t(Wire.read());
    adc = (adc << 8) + uint8_t(Wire.read());
    adc = (adc << 8) + uint8_t(Wire.read());
    return adc;
}

void loop()
{
    dnsServer.processNextRequest();

    static uint32_t delta = 0;
    static bool sign = 0;

    bool const adcReady = !digitalRead(A3);
    if(adcReady) {
        if((0)) {
            SerialLog.printf("reg=%02x,%02x", adc_read_config(), adc_read_status());
        }
        uint32_t const adc = adc_read_data();

        static uint32_t prev_adc = 0;
        if(adc > prev_adc) {
            delta = adc - prev_adc;
            sign = false;
        } else {
            delta = prev_adc - adc;
            sign = true;
        }
        prev_adc = adc;

        SerialLog.printf("adc=%06x delta=%c%06x heap=%d\r", adc, sign ? '-' : '+', delta, ESP.getFreeHeap());
        if(wsClient != nullptr && wsClient->canSend()) {
            wsClient->printf("%d", adc);
        }
    }

    // The easy way would be to just use `pixelidx = (adc / something) % numPixels`.
    // We instead do something more complicated to allow the LED ring to have
    // update rate faster than the ADC sample rate.  When the ADC value has
    // risen or fallen, we continually roll the LED in that direction until the
    // next sample comes in.

    static uint32_t prev_millis = 0;
    uint32_t const now = millis();
    uint32_t const delta_millis = (now - prev_millis);
    prev_millis = now;

    static uint32_t led_accum = 0;
    // In C/C++, signed integer overflow is undefined behavior.  Unsigned
    // integer overflow is okay.  We use overflow wraparound as a cheap modulo
    // operation.
    led_accum += (sign ? static_cast<uint32_t>(-delta) : delta) * delta_millis * (1<<sensitivity);
    int const pixelidx = led_accum >> (32 - log_num_pixels);
    pixels.clear();
    pixels.setPixelColor(pixelidx, pixels.Color(0, 150, 0));
    pixels.show();
}
