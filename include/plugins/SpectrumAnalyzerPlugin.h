#pragma once

#include "PluginManager.h"
//#include <Wire.h>

class SpectrumAnalyzerPlugin : public Plugin
{
private:

public:
    /*esp_err_t i2s_install();
    esp_err_t i2s_setpin();*/
    void setup() override;
    void loop() override;
    const char *getName() const override;
    void displayBand(int band, int dsize);
};
