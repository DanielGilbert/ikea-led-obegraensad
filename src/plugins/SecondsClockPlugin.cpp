#include "plugins/SecondsClockPlugin.h"

void SecondsClockPlugin::setup()
{
  // loading screen
  Screen.setPixel(4, 7, 1);
  Screen.setPixel(5, 7, 1);
  Screen.setPixel(7, 7, 1);
  Screen.setPixel(8, 7, 1);
  Screen.setPixel(10, 7, 1);
  Screen.setPixel(11, 7, 1);

  previousMinutes = -1;
  previousHour = -1;

  xSecondsLookupTable = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10, 9, 8, 07, 06, 05, 04, 03, 02, 01, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00 };
  ySecondsLookupTable = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 00, 00, 00, 00, 00, 00, 01, 02, 03, 04, 05, 06, 07, 8, 9, 10, 11, 12, 13, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 14, 13, 12, 11, 10, 9, 8, 07, 06, 05, 04, 03, 02, 01 };
}

void SecondsClockPlugin::loop()
{
  
  if (getLocalTime(&timeinfo))
  {
    if (previousHour != timeinfo.tm_hour || previousMinutes != timeinfo.tm_min)
    {
      Screen.clear();
      Screen.drawNumbers(3, 2, {(timeinfo.tm_hour - timeinfo.tm_hour % 10) / 10, timeinfo.tm_hour % 10});
      Screen.drawNumbers(3, 8, {(timeinfo.tm_min - timeinfo.tm_min % 10) / 10, timeinfo.tm_min % 10});
    }

    previousMinutes = timeinfo.tm_min;
    previousHour = timeinfo.tm_hour;

    if (previousSecond != timeinfo.tm_sec)
    {
      // clear second lane
      Screen.clearRect(0, 0, 16, 1);
      Screen.clearRect(0, 15, 16, 1);
      Screen.clearRect(0, 0, 1, 16);
      Screen.clearRect(15, 0, 1, 16);
      // alternating second pixel
      
      for (int i = 0; i <= timeinfo.tm_sec; i++)
      {
          Screen.setPixel(xSecondsLookupTable[i], ySecondsLookupTable[i], 1);
      }
  

      previousSecond = timeinfo.tm_sec;
    }

    previousMinutes = timeinfo.tm_min;
    previousHour = timeinfo.tm_hour;
  }
}

const char *SecondsClockPlugin::getName() const
{
  return "Seconds Clock";
}
