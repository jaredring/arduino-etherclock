/* EtherClock!
 *
 * An Ethernet Powered, NTP Synced, LCD Clock
 *
 * This is just a learning exercise
 *
 */

 // Libs from standard Arduino IDE
 #include <Wire.h>
 #include <SPI.h>

// Third Party Libs
#include <Ethernet2.h>          // https://github.com/adafruit/Ethernet2
#include <LiquidTWI2.h>         // https://github.com/lincomatic/LiquidTWI2
#include <DS3232RTC.h>          // https://github.com/JChristensen/DS3232RTC
#include <Time.h>               // https://github.com/PaulStoffregen/Time

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

void setup() {

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
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("IP Address:");
    lcd.setCursor(0, 1);
    lcd.print(Ethernet.localIP());
    delay(5000);

}

// Maintain our DHCP Lease
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

void loop() {
    delay(10);

    dhcp_maintain();
}


