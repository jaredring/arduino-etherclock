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

// Ethernet
#define ETHER_SS    10U         // Ethernet Slave Select D10
#define ETHER_RST   11U         // Etheret Reset D11

// NTP Details
unsigned int secsSinceLastNtpReq = 32000;       // Force an NTP update on power on
const unsigned int secsBetweenNtpReqs = 21600;   // Four Hours
unsigned int ntpLocalPort = 8123;
char ntpServer[] = "tranquil.home.thanhandjared.net";
const int NTP_PACKET_SIZE = 48;
byte ntpPacketBuffer[NTP_PACKET_SIZE];
bool ntpResponseRequired = false;
EthernetUDP ntpUdp;

// Setup the Pins
// Reserved Pins:
// Leonardo uses D2 for SDA and D3 for CLK
// DFRobot W5500 also steals D7, D10 and D11 for Ethernet

#define LCDRED 5
#define LCDGRN 6
#define LCDBLU 9

// Older Style LTI_TYPE_MCP23008
// New RGB Style LTI_TYPE_MCP23017
#define LCD_BACKPACK_TYPE LTI_TYPE_MCP23008
// I2C Address of Backback
LiquidTWI2 lcd(0);

// Set a MAC Address
// NB: This is an example MAC Address
// TODO: Derive a unique MAC address from a temperature sensor
// Ref: Jon Oxer
byte mac[] = {
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02
};

const unsigned long TZ_OFFSET_SECS = 36000UL;  // +10 from UTC

// Do we show the date or the temp
bool show_date = true;

float last_temp = 0.0;  // Are we rising, falling or steady

void LCD_clearLine(int line) {
    lcd.setCursor(0, line);
    for (int i=0;i<17;i++) {
        lcd.print(" ");
    }
    lcd.setCursor(0, line);
}


void setup() {
    // Reset Ethernet
    pinMode(ETHER_SS, OUTPUT);
    pinMode(ETHER_RST, OUTPUT);
    digitalWrite(ETHER_SS, LOW);
    digitalWrite(ETHER_RST, HIGH);  // Reset Ethernet
    delay(200);
    digitalWrite(ETHER_RST, LOW);   // Push Reset
    delay(200);
    digitalWrite(ETHER_RST, HIGH);  // Release Reset
    delay(200);
    // Ether Reset Complete


    // Open serial communications and wait for port to open:
    Serial.begin(9600);
    // this check is only needed on the Leonardo:
    while (!Serial and millis()<5000) {
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
    LCD_clearLine(1);
    lcd.print(Ethernet.localIP());
    Serial.print("Got IP: ");
    Serial.println(Ethernet.localIP());
    delay(5000);
    lcd.clear();

    ntpUdp.begin(ntpLocalPort);

}

// Maintainbe our DHCP Lease
// Based on examples from the Ethernet2 Library
void dhcp_maintain() {
    switch (Ethernet.maintain()) {
        case 1:
            // Renew FAIL
            Serial.println("DHCP Renew Failed");
            // Turn off NTP Fetching
            break;

        case 2:
            // Renew Success
            Serial.println("DHCP Renew Success");
            break;

        case 3:
            // Rebind Failed (how is this diff to Renew fail?)
            Serial.println("DHCP Rebind Failed");
            // Assume turn off NTP Fetching
            break;

        case 4:
            // Rebind Success
            Serial.println("DHCP Rebind Success");
            break;

        default:
            // Shrug
            break;

    }
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

//    ntpUdp.beginPacket(ntphost, 123);
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
        LCD_clearLine(0);
        lcd.print("Time: ");
        lcd.print(hour());
        lcd.print(":");
        lcd.print(minute());
        lcd.print(":");
        lcd.print(second());
        if (show_date and second() >= 30) {
            // Update to show the Temperature
            LCD_clearLine(1);
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
            LCD_clearLine(1);
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

