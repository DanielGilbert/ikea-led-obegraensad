#pragma once

#include "PluginManager.h"

class SecondsClockPlugin : public Plugin
{
private:
    struct tm timeinfo;

    int previousMinutes;
    int previousHour;
    int previousSecond;

    std::vector<int> xSecondsLookupTable;
    std::vector<int> ySecondsLookupTable;

public:
    void setup() override;
    void loop() override;
    const char *getName() const override;
};
