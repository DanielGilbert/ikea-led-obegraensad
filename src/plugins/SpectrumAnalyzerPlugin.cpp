#include <Arduino.h>
#include "arduinoFFT.h"
#include <driver/i2s_std.h>
#include "plugins/SpectrumAnalyzerPlugin.h"

// you shouldn't need to change these settings
#define SAMPLE_BUFFER_SIZE 1024
#define SAMPLE_RATE 44100
// most microphones will probably default to left channel but you may need to tie the L/R pin low
#define I2S_MIC_CHANNEL I2S_CHANNEL_FMT_ONLY_LEFT
// either wire your microphone to the same pins or change these to match your wiring
#define I2S_MIC_SERIAL_CLOCK GPIO_NUM_22
#define I2S_MIC_LEFT_RIGHT_CLOCK GPIO_NUM_23
#define I2S_MIC_SERIAL_DATA GPIO_NUM_21

i2s_chan_handle_t m_i2s_rx_handle;

int32_t raw_samples[SAMPLE_BUFFER_SIZE];
unsigned long newTime, oldTime;
double vReal[SAMPLE_BUFFER_SIZE];
double vImg[SAMPLE_BUFFER_SIZE];

unsigned int sampling_period_us;
unsigned long microseconds;

byte peak[] = {0,0,0,0,0,0,0,0};

ArduinoFFT<double> FFT(vReal, vImg, (uint16_t)SAMPLE_BUFFER_SIZE, (double)SAMPLE_RATE);

void SpectrumAnalyzerPlugin::setup()
{
  Screen.setPixel(4, 7, 1);
  Screen.setPixel(5, 7, 1);
  Screen.setPixel(7, 7, 1);
  Screen.setPixel(8, 7, 1);
  Screen.setPixel(10, 7, 1);
  Screen.setPixel(11, 7, 1);

  sampling_period_us = round(240000000 * (1.0 / 44100));

  newTime = micros();

     // -------- I2S configuration -------------------------------------------------------------------------------------------
  //i2s_chan_config_t m_i2s_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  i2s_chan_config_t m_i2s_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
 
  ESP_ERROR_CHECK(i2s_new_channel(&m_i2s_chan_cfg, NULL, &m_i2s_rx_handle));

  i2s_std_config_t m_i2s_std_cfg = {
    .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
    .slot_cfg = {
        .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
        .slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT,
        .slot_mode = I2S_SLOT_MODE_MONO,
        .slot_mask = I2S_STD_SLOT_LEFT,
        .ws_width = I2S_SLOT_BIT_WIDTH_32BIT,
        .ws_pol = false,
        .bit_shift = true,  // o false, no lo tengo claro
        .msb_right = false,
    },
    .gpio_cfg = {
      .mclk = I2S_GPIO_UNUSED,
      .bclk = I2S_MIC_SERIAL_CLOCK,
      .ws = I2S_MIC_LEFT_RIGHT_CLOCK,
      .dout = I2S_GPIO_UNUSED,
      .din = I2S_MIC_SERIAL_DATA,
      .invert_flags = {
        .mclk_inv = false,
        .bclk_inv = false,
        .ws_inv = false,
      },
    },
  };

  //m_i2s_std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;

  // The 'mclk_multiple' should be the multiple of 3 while using 24-bit data width
  //m_i2s_std_cfg.clk_cfg.mclk_multiple  = I2S_MCLK_MULTIPLE_384;  
  
  // Initialize the channel
  ESP_ERROR_CHECK(i2s_channel_init_std_mode(m_i2s_rx_handle, &m_i2s_std_cfg));

  // Before reading data, start the RX channel first
  ESP_ERROR_CHECK(i2s_channel_enable(m_i2s_rx_handle));

}
/*
unsigned int reverse(register unsigned int x)
{
    x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
    x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));
    x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));
    x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));
    return((x >> 16) | (x << 16));
}*/

bool sampleNew = false;


void SpectrumAnalyzerPlugin::loop()
{
  Screen.clear();
  // put your main code here, to run repeatedly:
  size_t bytes_read = 0;
  i2s_channel_read(m_i2s_rx_handle, &raw_samples, sizeof(int32_t) * SAMPLE_BUFFER_SIZE, &bytes_read, 1000);
  int samples_read = bytes_read / sizeof(int32_t);

  // dump the samples out to the serial channel.
  //newTime = micros();

  if ((micros() - newTime) > 16000ul){
    sampleNew = true;
  }

  if (sampleNew){
   // Serial.printf(">newTime:%ld\n", micros() - newTime);
    int16_t buffer16[SAMPLE_BUFFER_SIZE] = {0};
    for (int i = 0; i < samples_read; i++)
    {

        int32_t temp = raw_samples[i] >> 11;
        buffer16[i] = (temp > INT16_MAX) ? INT16_MAX : (temp < -INT16_MAX) ? -INT16_MAX : (int16_t)temp;
        vReal[i] = buffer16[i] * 1.0;
        vImg[i] = 0;

        //Serial.printf(">sample:%ld\n", buffer16[i]) ;



    }

            FFT.windowing(vReal, SAMPLE_BUFFER_SIZE, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
        FFT.compute(vReal, vImg, SAMPLE_BUFFER_SIZE, FFT_FORWARD);
        FFT.complexToMagnitude(vReal, vImg, SAMPLE_BUFFER_SIZE);
  }


  //while ((micros() - newTime) < sampling_period_us) { Serial.printf(">delta:%ld", micros() - newTime); Serial.printf(">period:%ld", sampling_period_us); }
  /*for (int i = 0; i < samples_read; i++)
  {
      vReal[i] = (((int32_t*)(raw_samples + i * 4))[0]>>8) * 1.0;
      vImg[i] = 0;
      Serial.printf(">sample:%ld\n", (((int32_t*)(raw_samples + i * 4))[0]>>8)) ;
  }*/

  /*int16_t buffer16[SAMPLE_BUFFER_SIZE] = {0};
  for (int i=0; i<samples_read; i++) {
      uint8_t mid = raw_samples[i * 4 + 2];
      uint8_t msb = raw_samples[i * 4 + 3];
      uint16_t raw = (((uint32_t)msb) << 8) + ((uint32_t)mid);
      memcpy(&buffer16[i], &raw, sizeof(raw)); // Copy so sign bits aren't interfered with somehow.
      Serial.printf(">sample:%ld\n", buffer16[i]) ;
  }*/



  int bandValues[16] = {0};

  for (int i = 2; i < (SAMPLE_BUFFER_SIZE/2); i++){ // Don't use sample 0 and only the first SAMPLES/2 are usable.
    // Each array element represents a frequency and its value, is the amplitude. Note the frequencies are not discrete.
    //if (vReal[i] > 2200) { // Add a crude noise filter, 10 x amplitude or more
      /*if (i<=6 )             displayBand(0,(int)vReal[i]); // 125Hz
      if (i >6   && i<=9 )   displayBand(1,(int)vReal[i]); // 250Hz
      if (i >13   && i<=18 )   displayBand(2,(int)vReal[i]); // 500Hz
      if (i >18   && i<=24 )  displayBand(3,(int)vReal[i]); // 1000Hz
      if (i >24  && i<=34 )  displayBand(4,(int)vReal[i]); // 2000Hz
      if (i >34  && i<=47 )  displayBand(5,(int)vReal[i]); // 4000Hz
      if (i >47  && i<=65 ) displayBand(6,(int)vReal[i]); // 8000Hz
      if (i >65  && i<=90 ) displayBand(7,(int)vReal[i]); // 16000Hz
      if (i >90  && i<=125 )  displayBand(8,(int)vReal[i]); // 125Hz
      if (i >125   && i<=173 )   displayBand(9,(int)vReal[i]); // 250Hz
      if (i >173   && i<=239 )   displayBand(10,(int)vReal[i]); // 500Hz
      if (i >239   && i<=331 )  displayBand(11,(int)vReal[i]); // 1000Hz
      if (i >331  && i<=459 )  displayBand(12,(int)vReal[i]); // 2000Hz
      if (i >459  && i<=636 )  displayBand(13,(int)vReal[i]); // 4000Hz
      if (i >636  && i<=881 ) displayBand(14,(int)vReal[i]); // 8000Hz
      if (i >881           ) displayBand(15,(int)vReal[i]); // 16000Hz*/

//16 bands, 6kHz top band
      if (i<=2 )           bandValues[0]  += (int)vReal[i];
      if (i>2   && i<=3  ) bandValues[1]  += (int)vReal[i];
      if (i>3   && i<=6  ) bandValues[2]  += (int)vReal[i];
      if (i>6   && i<=8  ) bandValues[3]  += (int)vReal[i];
      if (i>8   && i<=11  ) bandValues[4]  += (int)vReal[i];
      if (i>11   && i<=15  ) bandValues[5]  += (int)vReal[i];
      if (i>15   && i<=21  ) bandValues[6]  += (int)vReal[i];
      if (i>21   && i<=28  ) bandValues[7]  += (int)vReal[i];
      if (i>28   && i<=38  ) bandValues[8]  += (int)vReal[i];
      if (i>38   && i<=52  ) bandValues[9]  += (int)vReal[i];
      if (i>52   && i<=71  ) bandValues[10]  += (int)vReal[i];
      if (i>71   && i<=96  ) bandValues[11]  += (int)vReal[i];
      if (i>96   && i<=131  ) bandValues[12]  += (int)vReal[i];
      if (i>131   && i<=178  ) bandValues[13]  += (int)vReal[i];
      if (i>178   && i<=242  ) bandValues[14]  += (int)vReal[i];
      if (i>242   && i<=326  ) bandValues[15]  += (int)vReal[i];

    //}
    //for (byte band = 0; band <= 7; band++) display.drawHorizontalLine(1+16*band,64-peak[band],14);
  }

  for(int i = 0; i < 16; i++){
    displayBand(i, bandValues[i]);
  }

  if (sampleNew){
    sampleNew = false;
    newTime = micros();

  }

}

void SpectrumAnalyzerPlugin::displayBand(int band, int dsize){
  int dmax = 16;
  if (band == 0){
      dsize /= (22050+11025); //amplitude
  }
  else{
      dsize /= (11025+11025); //amplitude
  }

  if (dsize > dmax) dsize = dmax;
  for (int s = 0; s <= dsize; s += 1)
  {
    Screen.drawLine(band, 16, band, 16 - s, 1);
    //display.drawHorizontalLine(1+16*band,64-s, 14);
  }
  if (dsize > peak[band]) {peak[band] = dsize;}
}

const char *SpectrumAnalyzerPlugin::getName() const
{
  return "Spectrum Analyzer";
}
