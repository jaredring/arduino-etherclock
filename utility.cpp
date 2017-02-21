/* utility.ino */

#include "utility.h"
#include <LiquidTWI2.h>
#include <Ethernet2.h>          // https://github.com/adafruit/Ethernet2
#include <EthernetUdp2.h>       // https://github.com/adafruit/Ethernet2

void LCD_clearLine(LiquidTWI2 lcdref, int line) {
    lcdref.setCursor(0, line);
    char row[16];
    memset(row, ' ', 16);
    lcdref.print(row);
    lcdref.setCursor(0, line);
}

// Maintainbe our DHCP Lease
// Based on examples from the Ethernet2 Library
void dhcp_maintain() {
    switch (Ethernet.maintain()) {
        case DHCP_RENEW_FAIL:
            Serial.println("DHCP Renew Failed");
            // Turn off NTP Fetching
            break;
        case DHCP_RENEW_SUCCESS:
            Serial.println("DHCP Renew Success");
            break;
        case DHCP_REBIND_FAIL:
            // how is this diff to Renew fail?
            Serial.println("DHCP Rebind Failed");
            // Assume turn off NTP Fetching
            break;
        case DHCP_REBIND_SUCCESS:
            Serial.println("DHCP Rebind Success");
            break;
        default:
            break;
    }
}
