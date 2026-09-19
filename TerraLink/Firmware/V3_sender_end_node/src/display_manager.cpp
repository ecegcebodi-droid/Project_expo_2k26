#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "config.h"
#include "terralink_globals.h"
#include "terralink_modules.h"

static void drawHoldingScreen(const char *title);
static void drawGpsScreen();
static void drawSendingScreen();
static void drawWaitingScreen();

void initializeDisplay() {
    Wire.begin(OLED_SDA, OLED_SCL);
    oled.begin();
    oled.setPowerSave(0);
    oled.clearBuffer();
    oled.setFont(u8g2_font_6x10_tf);
    oled.drawStr(31, 30, "TERRALINK");
    oled.drawStr(35, 46, "STARTING");
    oled.sendBuffer();
    Serial.println("OLED initialized.");
}

void drawHeader() {
    oled.setFont(u8g2_font_6x10_tf);
    oled.drawStr(0, 9, "TERRALINK");
    oled.setCursor(103, 9); oled.print("N"); oled.print(NODE_ID);
    oled.drawLine(0, 13, 127, 13);
}
void drawNakedCheck(int x, int y) { oled.drawLine(x-5,y,x,y+5); oled.drawLine(x,y+5,x+9,y-6); }
void drawNakedCross(int x, int y) { oled.drawLine(x-5,y-5,x+5,y+5); oled.drawLine(x+5,y-5,x-5,y+5); }
void drawGPSIcon(int x, int y, bool fixed) {
    oled.drawCircle(x,y,5); oled.drawLine(x,y+5,x,y+9);
    if (fixed) drawNakedCheck(x+7,y); else { oled.drawLine(x-3,y-3,x+3,y+3); oled.drawLine(x+3,y-3,x-3,y+3); }
}
void drawWarningIcon(int x, int y) {
    oled.drawLine(x,y-7,x-7,y+6); oled.drawLine(x-7,y+6,x+7,y+6); oled.drawLine(x+7,y+6,x,y-7);
    oled.drawLine(x,y-3,x,y+2); oled.drawPixel(x,y+4);
}
void drawRadioWaves(int x, int y, int phase) {
    oled.drawDisc(x,y,2); if (phase>=1) oled.drawCircle(x,y,6); if (phase>=2) oled.drawCircle(x,y,10); if (phase>=3) oled.drawCircle(x,y,14);
}
void drawProgressBar(int x,int y,int width,int height,int percent) {
    percent=constrain(percent,0,100); oled.drawFrame(x,y,width,height);
    const int fillWidth=((width-2)*percent)/100; if(fillWidth>0) oled.drawBox(x+1,y+1,fillWidth,height-2);
}

void drawRescueMessageScreen() {
    oled.clearBuffer(); drawHeader(); oled.setFont(u8g2_font_6x10_tf); oled.drawStr(28,24,"RESCUE MESSAGE"); oled.drawLine(0,27,127,27);
    oled.setFont(u8g2_font_5x8_tf); oled.drawStr(2,37,"FROM:"); oled.setCursor(30,37); oled.print(currentRescueMessageSource);
    oled.setCursor(82,37); oled.print("P"); oled.print(receivedMessagePriority);
    String message(receivedRescueMessage); int line=0; String currentLine;
    for(unsigned int i=0;i<message.length();++i){ currentLine += message[i]; if(currentLine.length()>=21 || i==message.length()-1){ oled.drawStr(2,47+(line*8),currentLine.c_str()); currentLine=""; line++; if(line>=2) break; }}
    oled.setCursor(2,62); oled.print("ID:"); oled.print(receivedMessageID); oled.print(" ACK"); oled.sendBuffer();
}

void updateOLED() {
    if (millis()-lastUIUpdate < UI_UPDATE_MS) return;
    lastUIUpdate=millis();
    if (currentState==STATE_RESCUE_MESSAGE) { drawRescueMessageScreen(); return; }
    oled.clearBuffer(); drawHeader();
    switch(currentState) {
        case STATE_AWAKENING: {
            oled.setFont(u8g2_font_6x10_tf); oled.drawStr(36,30,"STARTING"); oled.drawStr(30,46,"PLEASE WAIT"); int phase=(millis()/250)%4;
            for(int i=0;i<4;i++) { if(i==phase) oled.drawDisc(49+i*10,57,2); else oled.drawCircle(49+i*10,57,2); } break;
        }
        case STATE_WAIT_RELEASE:
            oled.setFont(u8g2_font_6x10_tf); oled.drawStr(28,29,"BUTTON HELD"); oled.drawStr(18,43,"RELEASE FIRST"); oled.setFont(u8g2_font_5x8_tf); oled.drawStr(24,58,"THEN HOLD 2s"); break;
        case STATE_READY:
            oled.setFont(u8g2_font_6x10_tf); oled.drawStr(48,27,"READY"); oled.setFont(u8g2_font_5x8_tf); oled.drawStr(30,48,"EMERGENCY SOS"); oled.drawFrame(27,51,74,12); oled.drawStr(36,60,"HOLD 2 SEC"); break;
        case STATE_HOLDING_SOS: drawHoldingScreen("SOS HOLDING"); break;
        case STATE_SOS_ACTIVATED: drawGpsScreen(); break;
        case STATE_SENDING: drawSendingScreen(); break;
        case STATE_WAITING_ACK: drawWaitingScreen(); break;
        case STATE_SOS_SUCCESS:
            oled.setFont(u8g2_font_6x10_tf); drawNakedCheck(60,29); oled.drawStr(26,45,"HELP RECEIVED"); oled.setFont(u8g2_font_5x8_tf); oled.drawStr(29,59,"RESCUE ALERTED"); break;
        case STATE_SOS_FAILURE:
            oled.setFont(u8g2_font_6x10_tf); oled.drawStr(38,28,"SOS SENT"); oled.drawLine(43,37,51,37); oled.drawLine(57,37,65,37); oled.drawLine(71,37,79,37); oled.drawStr(46,46,"NO REPLY"); oled.setFont(u8g2_font_5x8_tf); oled.drawStr(22,59,"HOLD 2s TO RESEND"); break;
        case STATE_POST_SOS:
            oled.setFont(u8g2_font_5x8_tf); oled.drawStr(30,24,"HELP MODE ACTIVE"); { int pulse=(millis()/400)%2; if(pulse) oled.drawDisc(22,36,3); else oled.drawCircle(22,36,3); } oled.drawStr(32,39,"LINK ACTIVE"); oled.drawFrame(20,45,88,13); oled.drawStr(29,54,"HOLD 2s RESEND"); break;
        case STATE_HOLDING_RESEND: drawHoldingScreen("RESEND SOS"); break;
        default: break;
    }
    oled.sendBuffer();
}

static void drawHoldingScreen(const char *title) {
    oled.setFont(u8g2_font_6x10_tf); oled.drawStr(36,27,title); unsigned long held=millis()-touchStartTime;
    int percent=map(constrain((long)held,0L,(long)SOS_LONG_PRESS_MS),0,SOS_LONG_PRESS_MS,0,100);
    float remaining=((float)SOS_LONG_PRESS_MS-(float)held)/1000.0f; if(remaining<0) remaining=0;
    oled.setFont(u8g2_font_7x13B_tf); oled.setCursor(50,43); oled.print(remaining,1); oled.print("s"); drawProgressBar(10,50,108,9,percent);
}

static void drawGpsScreen() {
    oled.setFont(u8g2_font_5x8_tf);
    if(gpsFixAvailable){ drawGPSIcon(13,24,true); oled.drawStr(24,27,"GPS LOCATION OK"); oled.drawStr(0,39,"LAT:"); oled.setCursor(22,39); oled.print((double)currentLatitude,6); oled.drawStr(0,50,"LON:"); oled.setCursor(22,50); oled.print((double)currentLongitude,6); oled.drawStr(28,61,"PREPARING SOS..."); }
    else { drawWarningIcon(12,25); oled.setFont(u8g2_font_6x10_tf); oled.drawStr(25,27,"GPS SIGNAL WEAK"); oled.setFont(u8g2_font_5x8_tf); oled.drawStr(38,39,"MOVE TO OPEN"); oled.drawStr(51,49,"SPACE"); oled.drawStr(22,60,"SOS WILL STILL SEND"); }
    if(!gpsFixAvailable){ unsigned long elapsed=millis()-gpsWarningStart; int percent=map(constrain((long)elapsed,0L,(long)GPS_MESSAGE_TIME_MS),0,GPS_MESSAGE_TIME_MS,0,100); drawProgressBar(0,61,20,3,percent); }
}

static void drawSendingScreen() {
    oled.setFont(u8g2_font_6x10_tf); oled.drawStr(38,25,"SENDING SOS"); oled.setFont(u8g2_font_5x8_tf); oled.drawStr(0,37,"LAT:"); oled.setCursor(22,37); if(gpsFixAvailable) oled.print((double)currentLatitude,6); else oled.print("0.000000"); oled.drawStr(0,47,"LON:"); oled.setCursor(22,47); if(gpsFixAvailable) oled.print((double)currentLongitude,6); else oled.print("0.000000"); oled.drawStr(0,58,gpsFixAvailable?"GPS: FIX":"GPS: NO FIX"); int phase=(millis()/180)%4; oled.drawDisc(87,54,2); if(phase>=1) oled.drawCircle(87,54,5); if(phase>=2) oled.drawCircle(87,54,9); if(phase>=3) oled.drawCircle(87,54,13); oled.drawStr(106,58,"TX");
}

static void drawWaitingScreen() {
    oled.setFont(u8g2_font_5x8_tf); oled.drawStr(25,22,"WAITING FOR RESCUE"); oled.drawStr(43,34,"HELP LINK"); unsigned long elapsed=millis()-stateStartTime; long remaining=((long)ACK_WAIT_TIMEOUT_MS-(long)elapsed)/1000; if(remaining<0) remaining=0; oled.setFont(u8g2_font_7x13B_tf); oled.setCursor(42,49); oled.print(remaining); oled.print("s"); int phase=(millis()/220)%4; drawRadioWaves(88,43,phase); int percent=map(constrain((long)elapsed,0L,(long)ACK_WAIT_TIMEOUT_MS),0,ACK_WAIT_TIMEOUT_MS,100,0); drawProgressBar(10,56,108,6,percent);
}
