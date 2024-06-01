#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <FastLED.h>
FASTLED_USING_NAMESPACE

#define LED_PIN     D5
#define NUM_LEDS    40
int BRIGHTNESS = 250;
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

int UPDATES_PER_SECOND=100;

CRGBPalette16 currentPalette;
TBlendType    currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;
bool gReverseDirection = false;
CRGBPalette16 gPal;
String mycolor="acacac";
// Replace with your network credentials
const char* ssid = "RGB LED";
const char* password = "1234567890";

const char* PARAM_INPUT_1 = "output";
const char* PARAM_INPUT_2 = "state";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
IPAddress local_IP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>RGB LED CONTROL</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="icon" type="image/png" href="favicon.png">
    <style>
        h1,h3{color:#fff}html{font-family:Arial,Helvetica,sans-serif;display:inline-block;text-align:center}h1{font-size:1.8rem}
        .card-title,.reading,label{font-size:1.2rem}label{color:#051a36}.topnav{overflow:hidden;background-color:#1a59ac}.card,.slider:before{background-color:#fff}
        body{margin:0}.content{padding:50px}.card-grid{max-width:800px;margin:0 auto;display:grid;grid-gap:2rem;grid-template-columns:repeat(auto-fit,minmax(200px,1fr))}.card{box-shadow:2px 2px 12px 1px rgba(140,140,140,.5)}
        .card-title{font-weight:700;color:#075cac}.reading{color:#1282a2}select{font-size:1.1rem;color:#075cac;padding:5px;border-color:#075cac}
        .item{border-bottom:1px solid #051a36;padding:20px}.switch{position:relative;display:inline-block;width:80px;height:40px}.switch input{display:none}
        .slider{position:absolute;top:0;left:0;right:0;bottom:0;background-color:#b30000;border-radius:6px}.slider:before{position:absolute;content:"";height:32px;width:32px;left:8px;bottom:4px;-webkit-transition:.4s;transition:.4s;border-radius:3px}
        input:checked+.slider{background-color:#0c9494}input:checked+.slider:before{-webkit-transform:translateX(32px);-ms-transform:translateX(32px);transform:translateX(32px)}
    </style>
</head>
<body>
<body>
    <div class="topnav">
        <h1>RGB LED CONTROL</h1>
    </div>
    <div class="content">
        <div class="card-grid">
            <div class="card">
                <p class="card-title">Ma`lumotlar</p>
                <label class="switch"><input type="checkbox" onchange="selectMode(this)"> <span
                        class="slider"></span></label>
                <form name="led" method="post" id="myform">
                    <div id="multi">
                        <div class="item">
                            <label>Effekt nomi:</label>
                            <select name="effect" id="effect" onchange="changeRange(0)">
                                <option   value="1">Ranglar patnisi</option>
                                <option value="2">Olov</option>
                                <option value="3">Kamalak</option>
                                <option value="4">Halqa</option>
                                <option value="5">Effekt 1</option>
                                <option value="6">Effekt 2</option>
                            </select>
                        </div>
                        <div class="item">
                            <label>Tezlik:</label>
                            <input type="range" min="1" max="500" step="1" value="" id="speed" onchange="changeRange(1)"> <span id="birghtness_val"></span>
                        </div>
                    </div>
                    <div id="one">
                        <div class="item">
                            <label>Rang:</label>
                            <input type="color" onchange="changeRange(2)" value="">
                        </div>
                    </div>
                    <div class="item">
                        <label>Yorug`lik:</label>
                        <input type="range" min="0" max="255" step="1" value="" id="birghtness" onchange="changeRange(3)"> <span id="birghtness_val"></span>
                    </div>

                </form>
            </div>
        </div>
    </div>
    <footer>
        <div class="topnav">
            <h3>Created by: JASCO</h3>
        </div>
    </footer>

    <script>
        function selectMode(element) {
            var multi = document.getElementById("multi");
            var one = document.getElementById("one");
            if (element.checked) {
                multi.style.display = "block";
                one.style.display = "none";
            } else {
                multi.style.display = "none";
                one.style.display = "block";
            }
        }
        function changeRange(x) {
            var el=document.getElementById('myform').elements[x].value;
            var xhr = new XMLHttpRequest();
            switch(x) {
                case 0: {  xhr.open("GET", "/update?effect=" + el , true); } break;
                case 1: { xhr.open("GET", "/update?speed=" + el, true); } break;
                case 2: { xhr.open("GET", "/update?color=" + el.replace('#',''), true); } break;
                case 3: { xhr.open("GET", "/update?brightness=" + el, true); } break;
            };
            xhr.send();

        }
    </script>
</body>

</html>)rawliteral";
void rainbow();
void rainbowWithGlitter();
void confetti();
void sinelon();
void juggle();
void bpm();
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
byte Mode;

uint32_t colorCodeToHex(String colorCode) {
  uint8_t red = strtol(colorCode.substring(0, 2).c_str(), NULL, 16);
  uint8_t green = strtol(colorCode.substring(2, 4).c_str(), NULL, 16);
  uint8_t blue = strtol(colorCode.substring(4, 6).c_str(), NULL, 16);
  return ((uint32_t)red << 16) | ((uint32_t)green << 8) | blue;
}

void setup(){
  Serial.begin(115200);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );
  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;
  gPal = HeatColors_p;
  gPal = CRGBPalette16( CRGB::Black, CRGB::Red, CRGB::Yellow, CRGB::White);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  delay(100);
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // Send a GET request to <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage1;
    String inputMessage2;
    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
    if (request->hasParam("effect")) {
      inputMessage1 = request->getParam("effect")->value();
        Mode=inputMessage1.toInt();
    }
    else if (request->hasParam("speed")) {
      inputMessage1 = request->getParam("speed")->value();
        UPDATES_PER_SECOND=inputMessage1.toInt();
    }
    else if (request->hasParam("color")) {
      Mode=7;
      mycolor = request->getParam("color")->value();
    }
    else if (request->hasParam("brightness")) {
      inputMessage1 = request->getParam("brightness")->value();
      BRIGHTNESS=inputMessage1.toInt();
      FastLED.setBrightness(  BRIGHTNESS );
    }
    else {
      inputMessage1 = "No message sent";
    }
    Serial.print("GPIO: ");
    Serial.print(inputMessage1);
    Serial.print(" - Set to: ");
    Serial.println(inputMessage2);
    request->send(200, "text/plain", "OK");
  });
  

  // Start server
  server.begin();
}

void loop() {
  switch(Mode) {
    case 1: { showColorPalette(); } break;
    case 2: { showFire(); } break;
    case 3: { showReel(); } break;
    case 4: { showOneColor(); } break;
    case 5: { showEffect1(); } break;
    case 6: { showEffect2(); } break;
    case 7: {  showOneColor(); } break; 
    default: {
      showReel();
      }
    }
};

void showColorPalette() {
  ChangePalettePeriodically();   
    static uint8_t startIndex = 0;
    startIndex = startIndex + 1; /* motion speed */
    FillLEDsFromPaletteColors( startIndex);
    FastLED.show();
    FastLED.delay(2000 / UPDATES_PER_SECOND);
  };
void FillLEDsFromPaletteColors( uint8_t colorIndex)
{
    uint8_t brightness = 255;
    
    for( int i = 0; i < NUM_LEDS; ++i) {
        leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
        colorIndex += 3;
    }
}

void ChangePalettePeriodically()
{
    uint8_t secondHand = (millis() / 1000) % 60;
    static uint8_t lastSecond = 99;
    
    if( lastSecond != secondHand) {
        lastSecond = secondHand;
        if( secondHand ==  0)  { currentPalette = RainbowColors_p;         currentBlending = LINEARBLEND; }
        if( secondHand == 10)  { currentPalette = RainbowStripeColors_p;   currentBlending = NOBLEND;  }
        if( secondHand == 15)  { currentPalette = RainbowStripeColors_p;   currentBlending = LINEARBLEND; }
        if( secondHand == 20)  { SetupPurpleAndGreenPalette();             currentBlending = LINEARBLEND; }
        if( secondHand == 25)  { SetupTotallyRandomPalette();              currentBlending = LINEARBLEND; }
        if( secondHand == 30)  { SetupBlackAndWhiteStripedPalette();       currentBlending = NOBLEND; }
        if( secondHand == 35)  { SetupBlackAndWhiteStripedPalette();       currentBlending = LINEARBLEND; }
        if( secondHand == 40)  { currentPalette = CloudColors_p;           currentBlending = LINEARBLEND; }
        if( secondHand == 45)  { currentPalette = PartyColors_p;           currentBlending = LINEARBLEND; }
        if( secondHand == 50)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = NOBLEND;  }
        if( secondHand == 55)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = LINEARBLEND; }
    }
}

void SetupTotallyRandomPalette()
{
    for( int i = 0; i < 16; ++i) {
        currentPalette[i] = CHSV( random8(), 255, random8());
    }
}

void SetupBlackAndWhiteStripedPalette()
{
    // 'black out' all 16 palette entries...
    fill_solid( currentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    currentPalette[0] = CRGB::White;
    currentPalette[4] = CRGB::White;
    currentPalette[8] = CRGB::White;
    currentPalette[12] = CRGB::White;
    
}

void SetupPurpleAndGreenPalette()
{
    CRGB purple = CHSV( HUE_PURPLE, 255, 255);
    CRGB green  = CHSV( HUE_GREEN, 255, 255);
    CRGB black  = CRGB::Black;
    
    currentPalette = CRGBPalette16(
                                   green,  green,  black,  black,
                                   purple, purple, black,  black,
                                   green,  green,  black,  black,
                                   purple, purple, black,  black );
}
const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
{
    CRGB::Red,
    CRGB::Gray, // 'white' is too bright compared to red and blue
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Red,
    CRGB::Gray,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Black,
    CRGB::Black
};

void showFire() {
    random16_add_entropy( random());
  //   static uint8_t hue = 0;
  //   hue++;
//     CRGB darkcolor  = CHSV(hue,255,192); // pure hue, three-quarters brightness
//     CRGB lightcolor = CHSV(hue,128,255); // half 'whitened', full brightness
  gPal = CRGBPalette16( CRGB::Black, CRGB::Blue, CRGB::Red, CRGB::White);


  Fire2012WithPalette();
  
  FastLED.show(); // display this frame
  FastLED.delay(200);

  };
#define COOLING  55
#define SPARKING 120

void Fire2012WithPalette()
{
  static uint8_t heat[NUM_LEDS];
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }
  
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }
    for( int j = 0; j < NUM_LEDS; j++) {
      uint8_t colorindex = scale8( heat[j], 240);
      CRGB color = ColorFromPalette( gPal, colorindex);
      int pixelnumber;
      if( gReverseDirection ) {
        pixelnumber = (NUM_LEDS-1) - j;
      } else {
        pixelnumber = j;
      }
      leds[pixelnumber] = color;
    }
}

void showReel() {
  gPatterns[gCurrentPatternNumber]();
  FastLED.show();  
  FastLED.delay(2000/UPDATES_PER_SECOND); 
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } 
  EVERY_N_SECONDS( 20 ) { nextPattern(); } 
  }

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

void rainbow() 
{
  
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter() 
{
  rainbow();
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 10);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = beatsin16( 13, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( gHue, 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  uint8_t dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
};

void showOneColor() {
  for (byte i=0; i<NUM_LEDS; i++) {
    leds[i] = colorCodeToHex(mycolor);
    }
    FastLED.show();  
  FastLED.delay(1000/200); 
  };

void fadeall() { for(int i = 0; i < NUM_LEDS; i++) { leds[i].nscale8(250); } };
void showEffect1() {
    static uint8_t hue = 0;
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue++, 255, 255);
    FastLED.show(); 
    // leds[i] = CRGB::Black;
    fadeall();
    delay(1000/UPDATES_PER_SECOND);
  }; 
  }
void showEffect2() {
    static uint8_t hue = 0;
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(hue++, 255, 255);
    FastLED.show(); 
    fadeall();
  } 
  delay(2000/UPDATES_PER_SECOND);
  }
