/* constants.h */

// Ethernet
#define ETHER_SS    10U         // Ethernet Slave Select D10
#define ETHER_RST   11U         // Etheret Reset D11

// Setup the Pins
// Reserved Pins:
// Leonardo uses D2 for SDA and D3 for CLK
// DFRobot W5500 also steals D7, D10 and D11 for Ethernet
#define LCDRED 5
#define LCDGRN 6
#define LCDBLU 9

// NTP Details
unsigned int secsSinceLastNtpReq = 32000;       // Force an NTP update on power on
const unsigned int secsBetweenNtpReqs = 21600;   // Four Hours
unsigned int ntpLocalPort = 8123;
char ntpServer[] = "tranquil.home.thanhandjared.net";
const int NTP_PACKET_SIZE = 48;
byte ntpPacketBuffer[NTP_PACKET_SIZE];
bool ntpResponseRequired = false;
EthernetUDP ntpUdp;


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
