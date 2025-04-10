/*

  GPS module

  Copyright (C) 2018 by Xose Pérez <xose dot perez at gmail dot com>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <TinyGPS++.h>
#include <TimerOne.h>

#define NO_CONNECTION_PRINT_COUNT 200000
#define GPS_POLL_INTERVAL 3000

//bool isDataFresh = true;
//uint32_t latitude;
//uint32_t longitude;
//uint32_t altitude;
uint32_t randomNumber;
//uint8_t hdop;
//uint8_t sats;

// unsigned long noConnectionCounter = 0;
//long lastGPSCheck = 0;
//long gpsCheckDeltaCounter = 0;
//volatile bool allowGPSPoll = false;

//TinyGPSPlus gps;
//HardwareSerial _serial_gps(GPS_SERIAL_NUM);

//void gps_time(char *buffer, uint8_t size)
//{
//    snprintf(buffer, size, "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
//}


void test_loop()
{
    randomNumber = random(0,10);
    DEBUG_PORT.println(randomNumber);
}


bool buildPacketTest()
{
    int i = 0;
    DEBUG_PORT.println("randomnumber is txBuffer.");

    txBuffer[i++] = randomNumber;//latitude >> 16;
   // txBuffer[i++] = latitude >> 8;
   // txBuffer[i++] = latitude;
   // txBuffer[i++] = longitude >> 16;
   // txBuffer[i++] = longitude >> 8;
   // txBuffer[i++] = longitude;
   // txBuffer[i++] = altitude >> 16;
   // txBuffer[i++] = altitude >> 8;
   // txBuffer[i++] = altitude;
   // txBuffer[i++] = sats;
    txBuffer[i++] = (char)'t';

   // isDataFresh = false;
    return true;
}