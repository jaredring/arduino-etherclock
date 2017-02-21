/* utility.h */

#include <LiquidTWI2.h>

// dhcp statuses
const int DHCP_RENEW_FAIL = 1;
const int DHCP_RENEW_SUCCESS = 2;
const int DHCP_REBIND_FAIL = 3;
const int DHCP_REBIND_SUCCESS = 4;

void LCD_clearLine(LiquidTWI2 lcdref, int line);
void dhcp_maintain();
