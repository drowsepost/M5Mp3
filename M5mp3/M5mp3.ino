#include <FS.h>
#include <Wire.h>
#include <SD.h>
#include "M5Cardputer.h"
#include "Audio.h"  // https://github.com/schreibfaul1/ESP32-audioI2S   version 3.2.1
#include "font.h"
M5Canvas sprite(&M5Cardputer.Display);

// microSD card
#define SD_SCK 40
#define SD_MISO 39
#define SD_MOSI 14
#define SD_CS 12

// I2S
#define I2S_DOUT 42
#define I2S_BCLK 41
#define I2S_LRCK 43

#define APP_TITLE "M5 AMP"
#define FONT_FILE &fonts::lgfxJapanGothic_12

Audio audio;

unsigned short grays[18];
unsigned short gray;
unsigned short light;
int playingPoint = -1;
int n = 0;
int volume=5;
int bri=0;
int brightness[5]={50,100,150,200,250};
bool isPlaying = false;
bool stoped=true;
bool nextS=0;
bool volUp=0;
int g[14]={0};
int graphSpeed=0;
int textPos=60;
int sliderPos=0;

// Task handle for audio task
TaskHandle_t handleAudioTask = NULL;

#define MAX_FILES 128
#define SCAN_DIRECTORY "/music/"

// Array to store file names
String audioFiles[MAX_FILES];
String audioNames[MAX_FILES];

char playTitle[128] = {0};
int32_t playTitleWidth = 0;
uint32_t audioPlayTime = 0;
int fileCount = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("hi");

    // Initialize M5Cardputer and SD card
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Speaker.begin();

    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setBrightness(brightness[bri]);

    M5Cardputer.Display.setTextFont(0);
    M5Cardputer.Display.setTextColor(TFT_WHITE,TFT_BLACK);
    M5Cardputer.Display.drawString("Loading...", 8, 8);
    
    sprite.createSprite(240,135);

    if (!SD.begin()) {
        Serial.println(F("ERROR: SD Mount Failed!"));
    }
    listFiles(SD, SCAN_DIRECTORY, 1);

    // Initialize audio output
    audio.setPinout(I2S_BCLK, I2S_LRCK, I2S_DOUT);
    audio.setVolume(volume); // 0...21
    audio.setBufferSize(8*1024);
    audio.setAudioTaskCore(0);

    int co = 214;
    for (int i = 0; i < 18; i++) {
        grays[i] = M5Cardputer.Display.color565(co, co, co+40);
        co = co - 13;
    }

    // Create tasks and pin them to different cores
    xTaskCreatePinnedToCore(Task_TFT, "Task_TFT", 20480, NULL, 1, NULL, 1);            // Core 0
    xTaskCreatePinnedToCore(Task_Audio, "Task_Audio", 20480, NULL, 2, &handleAudioTask, 0); // Core 1
}

void loop() {
    M5Cardputer.update();
    // Check for key press events
    if (M5Cardputer.Keyboard.isChange()) {
        if (M5Cardputer.Keyboard.isKeyPressed('a')) {
            if(playingPoint < 0) {
                playingPoint = n;
                nextS=1;
            }
            isPlaying = !isPlaying;
            stoped=!stoped;
        } // Toggle the playback state

        if (M5Cardputer.Keyboard.isKeyPressed('v')) {
            isPlaying=false;
            volUp=true;
            volume=volume+5;
            if(volume>20) volume=5;
        } 

        if (M5Cardputer.Keyboard.isKeyPressed('/')) {
            isPlaying=false;
            volUp=true;
            volume=volume+1;
            if(volume>20) volume=20;
        } 

        if (M5Cardputer.Keyboard.isKeyPressed(',')) {
            isPlaying=false;
            volUp=true;
            volume=volume-1;
            if(volume<1) volume=1;
        } 

        if (M5Cardputer.Keyboard.isKeyPressed('l')) {
            bri++;
            if(bri==5) bri=0;
            M5Cardputer.Display.setBrightness(brightness[bri]);
        }

        if (M5Cardputer.Keyboard.isKeyPressed('p')) {
            isPlaying=false;
            playingPoint--;
            if(playingPoint<0) playingPoint=fileCount-1;
            nextS=1;
        } 
        
        if (M5Cardputer.Keyboard.isKeyPressed('n')) {
            isPlaying=false;
            playingPoint++;
            if(playingPoint>=fileCount) playingPoint=0;
            nextS=1;
        } 

        if (M5Cardputer.Keyboard.isKeyPressed(';')) {
            n--;
            if(n<0) n=fileCount-1;
        } 

        if (M5Cardputer.Keyboard.isKeyPressed('.')) {
            n++;
            if(n>=fileCount) n=0;
        } 

        if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
            stoped=false;
            isPlaying=false;
            playingPoint = n;
            nextS=1;
        } 

        if (M5Cardputer.Keyboard.isKeyPressed('b')) {
            isPlaying=false;
            playingPoint = random(0,fileCount);
            nextS=1;
        } 
    }
    vTaskDelay(20 / portTICK_PERIOD_MS);
}

String formatTime(uint32_t seconds) {
    if(seconds < 0) return "00:00";
    uint32_t minutes = seconds / 60;
    uint32_t secs = seconds % 60;

    char buffer[6]; // "MM:SS" + EOL
    snprintf(buffer, sizeof(buffer), "%02lu:%02lu", minutes, secs);
    return String(buffer);
}

void draw()
{
    gray=grays[15];
    light=grays[11];
    sprite.fillRect(0,0,240,135,gray); // bg

    // FILE LIST
    sprite.fillRect(4,8,130,122,BLACK);

    // SLIDER
    sprite.fillRect(129,8,5,122,0x0841); // slider bg
    sliderPos=map(n,0,fileCount,8,110);
    sprite.fillRect(129,sliderPos,5,20,grays[2]);
    sprite.fillRect(131,sliderPos+4,1,12,grays[16]);

    // TITLE BAR
    sprite.fillRect(4,2,50,2,ORANGE);
    sprite.fillRect(84,2,50,2,ORANGE);

    // Window Border
    sprite.drawFastVLine(3,9,120,light);
    sprite.drawFastVLine(134,9,120,light);
    sprite.drawFastHLine(3,129,130,light);
    
    sprite.drawFastHLine(0,0,240,light);
    sprite.drawFastHLine(0,134,240,light);

    // CENTER BAR
    sprite.fillRect(139,0,3,135,BLACK);

    // PLAYER FACE
    sprite.fillRect(190,2,45,2,ORANGE);
    sprite.fillRect(190,6,45,3,grays[4]);
    sprite.fillRect(148,14,86,42,BLACK);
    sprite.fillRect(148,59,86,16,BLACK);
    
    sprite.drawFastVLine(143,0,135,light);
    
    sprite.drawFastVLine(238,0,135,light);
    sprite.drawFastVLine(138,0,135,light);
    sprite.drawFastVLine(148,14,42,light);
    sprite.drawFastHLine(148,14,86,light);

    //buttons
    for(int i=0;i<4;i++)
        sprite.fillRoundRect(148+(i*22),94,18,18,2,grays[4]);

    //button icons
    sprite.fillRect(220,104,8,2,grays[13]);
    sprite.fillRect(220,108,8,2,grays[13]);
    sprite.fillTriangle(228,102,228,106,231,105,grays[13]);
    sprite.fillTriangle(220,106,220,110,217,109,grays[13]);

    if(!stoped) {
        sprite.fillRect(152,104,3,6,grays[13]);
        sprite.fillRect(157,104,3,6,grays[13]);
    } else {
        sprite.fillTriangle(156,102,156,110,160,106,grays[13]);
    }
    
    //volume bar
    int volumepos = (int)(((float)((volume - 1) * 5) / 100.0) * 55.0);
    sprite.fillRoundRect(172,82,60,3,2,YELLOW);
    sprite.fillRoundRect(172+volumepos,80,10,8,2,grays[2]);
    sprite.fillRoundRect(174+volumepos,82,6,4,2,grays[10]);
    
    // brightness
    sprite.fillRoundRect(172,124,30,3,2,MAGENTA);
    sprite.fillRoundRect(172+(bri*5),122,10,8,2,grays[2]);
    sprite.fillRoundRect(174+(bri*5),124,6,4,2,grays[10]);

    // GRAPH
    for(int i=0;i<14;i++){ 
        if(!stoped)  
            g[i]=random(1,5);
        for(int j=0;j<g[i];j++)
            sprite.fillRect(172+(i*4),50-j*3,3,2,grays[4]);
    }
    
    // FILE LIST
    sprite.setTextFont(FONT_FILE);
    sprite.setTextDatum(0);
    sprite.setClipRect(4,8,125,122);

    if(n<5) {
        for(int i=0;i<10;i++) {
            sprite.setTextColor(((i==n)? WHITE : TFT_GREEN), ((i==playingPoint)? 0x0862 : BLACK));

            if(i<fileCount)
                sprite.drawString(audioNames[i], 5, 8+(i*12));
        }
    } else {
        int yos=0;
        for(int i=n-5;i<n-5+10;i++) {
            sprite.setTextColor(((i==n)? WHITE : TFT_GREEN), ((i==playingPoint)? 0x0862 : BLACK));

            if(i<fileCount)
                sprite.drawString(audioNames[i], 5, 8+(yos*12));
            yos++;
        }
    }
    
    sprite.clearClipRect();
    
    // UI TEXTS
    sprite.setTextFont(0);
    sprite.setTextDatum(0);

    sprite.setTextColor(grays[2],gray);
    sprite.drawString("LIST", 58, 0);
    sprite.setTextColor(grays[4],gray);
    sprite.drawString("VOL", 148, 80);
    sprite.drawString("LIG", 148, 122);
    
    // MAIN FACE
    sprite.setTextColor(grays[1],gray);
    sprite.drawString(APP_TITLE, 148, 3);
    sprite.setTextColor(grays[8],BLACK);
    
    if(isPlaying && !stoped) {
        sprite.drawString("P", 152,18);
        sprite.drawString("L", 152,27);
        sprite.drawString("A", 152,36);
        sprite.drawString("Y", 152,45);
        sprite.fillTriangle(162,18,162,26,168,22,TFT_GREEN);
        sprite.fillRect(162,30,6,6,TFT_MAROON);
    } else {
        sprite.drawString("S", 152,18);
        sprite.drawString("T", 152,27);
        sprite.drawString("O", 152,36);
        sprite.drawString("P", 152,45);
        sprite.fillTriangle(162,18,162,26,168,22,TFT_DARKGREEN);
        sprite.fillRect(162,30,6,6,RED);
    }

    // TIME COUNT
    sprite.setTextColor(TFT_GREEN,BLACK); 
    sprite.setFreeFont(&DSEG7_Classic_Mini_Regular_16);
    sprite.drawString(formatTime(audioPlayTime), 172, 18);

    // BATTERY
    sprite.drawRect(206,119,28,12,TFT_GREEN);
    sprite.fillRect(234,122,3,6,TFT_GREEN);

    sprite.setTextFont(0);
    int percent=0;
    if(analogRead(10)>2390)
        percent=100;
    else if(analogRead(10)<1400)
        percent=1;
    else
        percent=map(analogRead(10),1400,2390,1,100);
    
    sprite.setTextDatum(3);
    sprite.drawString(String(percent)+"%", 220, 121);

    // BTN FACE
    sprite.setTextColor(BLACK,grays[4]);
    sprite.drawString("B",220,96); 
    sprite.drawString("N",198,96); 
    sprite.drawString("P",176,96); 
    sprite.drawString("A",154,96); 

    sprite.setTextColor(BLACK,grays[5]);
    sprite.drawString(">>",202,103); 
    sprite.drawString("<<",180,103); 
    
    // MUSIC TITLE
    sprite.setClipRect(148,59,86,16);
    sprite.setTextFont(FONT_FILE);
    sprite.setTextDatum(0);
    sprite.fillSprite(BLACK);
    sprite.setTextColor(TFT_GREEN,BLACK);
    sprite.drawString(playTitle,148 + textPos,62);
    textPos=textPos-2;
    if(textPos < -playTitleWidth) textPos=90;
    sprite.clearClipRect();
    
    sprite.pushSprite(0,0);
}

void updateTitleString(const char *str) {
    strncpy(playTitle, str, sizeof(playTitle) - 1);
    playTitle[sizeof(playTitle) - 1] = '\0';
    playTitleWidth = sprite.textWidth(playTitle, FONT_FILE);
}

void playSong(const int n) {
    audio.stopSong();
    vTaskDelay(10 / portTICK_PERIOD_MS);
    audio.connecttoFS(SD,audioFiles[n].c_str(), 0);
    updateTitleString(audioNames[n].c_str());
    playingPoint = n;
    textPos=90;
}

void Task_TFT(void *pvParameters) {
    while (1) {
        if(graphSpeed==0) {
            draw();  // Adjust the delay for responsiveness
        }
        graphSpeed++;
        if(graphSpeed==4) graphSpeed=0;
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}

void Task_Audio(void *pvParameters) {
    while (1) {
      
        if(volUp) {
            audio.setVolume(volume); 
            isPlaying=1;
            volUp=0;
        }

        if(nextS) {
            playSong(playingPoint);
            isPlaying=1;
            nextS=0;
        }

        if (isPlaying) {
            while (isPlaying) {
                if(!stoped) {
                    audio.loop();  // This keeps the audio playback running
                    audioPlayTime = audio.getAudioCurrentTime();
                }
                vTaskDelay(10 / portTICK_PERIOD_MS);  // Add a small delay to prevent task hogging
            }
        } else {
            isPlaying=true;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void listFiles(fs::FS &fs, const char *dirname, uint8_t levels) {
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root) {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        root.close();
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file && fileCount < MAX_FILES) {
        if (file.isDirectory()) {
            Serial.print("DIR : ");
            Serial.println(file.path());
            if (levels) {
                listFiles(fs, file.path(), levels - 1);
            }
        } else {
            String filename = String(file.name());
            String filepath = String(file.path());
            Serial.print("FILE: ");
            Serial.println(filepath);
            if ((filepath != "") && filename.lastIndexOf(".mp3") > 0 && filepath.length()<256) {    
                audioFiles[fileCount] = filepath;
                audioNames[fileCount] = filename;
                fileCount++;
            }
        }
        file = root.openNextFile();
    }

    file.close();
    root.close();
}

void audio_id3data(const char *info) {
    const char *longPrefix  = "Title/Songname/Content description:";
    const char *shortPrefix = "Title:";
    const char *title = nullptr;

    if (strncmp(info, longPrefix, strlen(longPrefix)) == 0) {
        title = info + strlen(longPrefix);
    } else if (strncmp(info, shortPrefix, strlen(shortPrefix)) == 0) {
        title = info + strlen(shortPrefix);
    }

    if (title) {
        while (*title == ' ') title++; // Delete Space
        updateTitleString(title);
        Serial.printf("Title Updated: %s\n", playTitle);
        textPos=90;
    }
}

void audio_eof_mp3(const char *info) {
    Serial.print("eof_mp3     ");
    Serial.println(info);
    playingPoint++;
    if(playingPoint>=fileCount) playingPoint=0;
    playSong(playingPoint);
}
