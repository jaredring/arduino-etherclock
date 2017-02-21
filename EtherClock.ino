/* EtherClock!
 *
 * An Ethernet Powered, NTP Synced, LCD Clock
 *
 * This is just a learning exercise
 *
 * It Displays the time. and date, and temperature
 * and adjust the backlight colour of the LCD depending onw hat the temp is doing
 * Rising - Red
 * Steady - Blue
 * Falling - Green
 *
 */

 // Libs from standard Arduino IDE
 #include <Wire.h>
 #include <SPI.h>

// Third Party Libs
#include <Ethernet2.h>          // https://github.com/adafruit/Ethernet2
#include <EthernetUdp2.h>       // https://github.com/adafruit/Ethernet2
#include <LiquidTWI2.h>         // https://github.com/lincomatic/LiquidTWI2
#include <DS3232RTC.h>          // https://github.com/JChristensen/DS3232RTC
#include <Time.h>               // https://github.com/PaulStoffregen/Time

#include "constants.h"
#include "utility.h"

void setup() {
    // Reset Ethernet
    reset_ethernet(ETHER_SS, ETHER_RST);

    // Open serial communications and wait for port to open:
    Serial.begin(9600);
    // this check is only needed on the Leonardo:
    while (!Serial && millis()<5000) {
        ; // wait for serial port to connect. Needed for native USB port only
    }
    Serial.println("We have a serial console!");

    // Define LCD backup type and initialise
    lcd.setMCPType(LCD_BACKPACK_TYPE);
    lcd.begin(16, 2);
    lcd.clear();
    lcd.setCursor(0, 0);

    // Define our RGB PWM Pins
    // NB: On the RGB LCD we have it is common Annode!
    // This effectively turns all them on
    pinMode(LCDRED, OUTPUT);
    pinMode(LCDGRN, OUTPUT);
    pinMode(LCDBLU, OUTPUT);
    digitalWrite(LCDRED, 0);
    digitalWrite(LCDGRN, 0);
    digitalWrite(LCDBLU, 0);

    // Fire up the RTC - This I believe tells the RTC to sync every 5mins
    setSyncProvider(RTC.get);
    if (timeStatus() != timeSet) {
        Serial.println("Time has not been set on the RTC");
        lcd.print("RTC is NOT Set");
        delay(5000);
    } else {
        Serial.println("Time is set in RTC");
        lcd.print("RTC is set");
        delay(5000);
    }

    lcd.clear();
    lcd.setCursor(0, 0);

    // Fire up Ethernet and DHCP
    lcd.print("Attempting DHCP");
    lcd.setCursor(0, 1);
    lcd.print("Wait for DHCP...");

    while (Ethernet.begin(mac) != 1) {
        Serial.println("No response from DHCP. Sleeping.");
        delay(5000);
    }

    LCD_clearLine(lcd, 1);
    lcd.print(Ethernet.localIP());
    Serial.print("Got IP: ");
    Serial.println(Ethernet.localIP());
    delay(5000);
    lcd.clear();

    ntpUdp.begin(ntpLocalPort);
}

void sendNtpRequest(char* ntphost) {

    // Initialise the Packet
    memset(ntpPacketBuffer, 0, NTP_PACKET_SIZE);

    ntpPacketBuffer[0] = 0b11100011;    // LI, Version, Mode
    ntpPacketBuffer[1] = 0;             // Stratum
    ntpPacketBuffer[2] = 6;             // Polling Interval (Really? What is this measured in, is it realistic?)
    ntpPacketBuffer[3] = 0xEC;          // Peer Clock Precision (WHAT IS IT)
    // 8 bytes of zero
    ntpPacketBuffer[12] = 49;           // I justs love random magic numbers!
    ntpPacketBuffer[13] = 0x4E;
    ntpPacketBuffer[14] = 49;
    ntpPacketBuffer[15] = 52;

    // ntpUdp.beginPacket(ntphost, 123);
    IPAddress tranquil(192, 168, 0, 249);
    ntpUdp.beginPacket(tranquil, 123);
    ntpUdp.write(ntpPacketBuffer, NTP_PACKET_SIZE);
    ntpUdp.endPacket();
    Serial.println("Sent an NTP Request");
}

// This returns seconds since epoch, in UTC
time_t processNtpResponse() {

    ntpUdp.read(ntpPacketBuffer, NTP_PACKET_SIZE);
    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(ntpPacketBuffer[40], ntpPacketBuffer[41]);
    unsigned long lowWord = word(ntpPacketBuffer[42], ntpPacketBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    // now convert NTP time into everyday time:
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    unsigned long localTime = epoch + TZ_OFFSET_SECS;
    // print Unix time:

    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(epoch % 60); // print the second

    // print the hour, minute and second:
    Serial.print("The AEST time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((localTime  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if ( ((localTime % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((localTime  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ( (localTime % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(localTime % 60); // print the second


    return epoch;
}

// This takes seconds since epoch in UTC
// Adds in our TZ adjustment and sets
void updateTime(time_t t) {

    t += TZ_OFFSET_SECS;
    RTC.set(t);
    setTime(t);

}

void loop() {
    // A second has elapsed - Update Clock
    if ((millis() % 1000) == 0) {
        LCD_clearLine(lcd, 0);
        lcd.print("Time: ");
        lcd.print(hour());
        lcd.print(":");
        lcd.print(minute());
        lcd.print(":");
        lcd.print(second());
        if (show_date and second() >= 30) {
            // Update to show the Temperature
            LCD_clearLine(lcd, 1);
            float tempC = RTC.temperature() /4.0;
            lcd.print("Temp: ");
            lcd.print(tempC);
            lcd.print("C");
            String message;
            if (tempC == last_temp) {
                // Steady - BLUE
                message = "Temp Steady: ";
                digitalWrite(LCDRED, 255);
                digitalWrite(LCDGRN, 255);
                digitalWrite(LCDBLU, 0);
            } else if (tempC > last_temp) {
                // Rising - RED
                message = "Temp Rising: ";
                digitalWrite(LCDRED, 0);
                digitalWrite(LCDGRN, 255);
                digitalWrite(LCDBLU, 255);
            } else {
                // Falling - GREEN
                message = "Temp Falling: ";
                digitalWrite(LCDRED, 255);
                digitalWrite(LCDGRN, 0);
                digitalWrite(LCDBLU, 255);
            }
            Serial.println(message + last_temp + "C, Now: " + tempC + "C");
            last_temp = tempC;
            show_date = !show_date;
        } else if (not show_date and second() < 30) {
            // Update to show the Date
            LCD_clearLine(lcd, 1);
            lcd.print("Date: ");
            lcd.print(day());
            lcd.print("/");
            lcd.print(month());
            lcd.print("/");
            lcd.print(year());
            show_date = !show_date;
        }

        // See if we have an NTP response, but only when we expect one
        // I like short circuit eval
        if (ntpResponseRequired and ntpUdp.parsePacket()) {
			Serial.println("NTP Response is waiting to be processed");
            time_t t = processNtpResponse();
            updateTime(t);
            ntpResponseRequired = false;
        }
        secsSinceLastNtpReq++;
    }

    if (secsSinceLastNtpReq > secsBetweenNtpReqs) {
        Serial.println("NTP Update Required");
        sendNtpRequest(ntpServer);
		secsSinceLastNtpReq = 0;
        ntpResponseRequired = true;
    }

    dhcp_maintain();
}
