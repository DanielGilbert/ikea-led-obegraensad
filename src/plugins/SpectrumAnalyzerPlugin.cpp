#include <Arduino.h>
#include "arduinoFFT.h"
#include <driver/i2s_std.h>
#include "plugins/SpectrumAnalyzerPlugin.h"

// you shouldn't need to change these settings
#define SAMPLE_BUFFER_SIZE 1024
#define SAMPLE_RATE 8000
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

ArduinoFFT<double> FFT(vReal, vImg, (uint16_t)SAMPLE_BUFFER_SIZE, (double)8000);

void SpectrumAnalyzerPlugin::setup()
{
  Screen.setPixel(4, 7, 1);
  Screen.setPixel(5, 7, 1);
  Screen.setPixel(7, 7, 1);
  Screen.setPixel(8, 7, 1);
  Screen.setPixel(10, 7, 1);
  Screen.setPixel(11, 7, 1);

  sampling_period_us = round(1000000 * (1.0 / 8000));

     // -------- I2S configuration -------------------------------------------------------------------------------------------
  //i2s_chan_config_t m_i2s_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  i2s_chan_config_t m_i2s_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
 
  ESP_ERROR_CHECK(i2s_new_channel(&m_i2s_chan_cfg, NULL, &m_i2s_rx_handle));

  i2s_std_config_t m_i2s_std_cfg = {
    .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
    .slot_cfg = {
        .data_bit_width = I2S_DATA_BIT_WIDTH_24BIT,
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

void SpectrumAnalyzerPlugin::loop()
{

  Screen.clear();
  // put your main code here, to run repeatedly:
  size_t bytes_read = 0;
  i2s_channel_read(m_i2s_rx_handle, &raw_samples, sizeof(int32_t) * SAMPLE_BUFFER_SIZE, &bytes_read, 1000);
  int samples_read = bytes_read / sizeof(int32_t);
  Screen.setPixel(9, 7, 0);
  Screen.setPixel(10, 7, 0);
  Screen.setPixel(11, 7, 0);

  // dump the samples out to the serial channel.
  for (int i = 0; i < samples_read; i++)
  {
    newTime = micros();

      vReal[i] = raw_samples[i];
      vImg[i] = 0;
      Serial.printf(">sample:%ld\n", raw_samples[i]);
      while ((micros() - newTime) < sampling_period_us) { /* do nothing to wait */ }
    

  }

  FFT.windowing(vReal, SAMPLE_BUFFER_SIZE, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(vReal, vImg, SAMPLE_BUFFER_SIZE, FFT_FORWARD);
  FFT.complexToMagnitude(vReal, vImg, SAMPLE_BUFFER_SIZE);

  for (int i = 2; i < (SAMPLE_BUFFER_SIZE/2); i++){ // Don't use sample 0 and only the first SAMPLES/2 are usable.
    // Each array element represents a frequency and its value, is the amplitude. Note the frequencies are not discrete.
    if (vReal[i] > 150000) { // Add a crude noise filter, 10 x amplitude or more
      if (i<=2 )             displayBand(0,(int)vReal[i]); // 125Hz
      if (i >2   && i<=4 )   displayBand(1,(int)vReal[i]); // 250Hz
      if (i >4   && i<=7 )   displayBand(2,(int)vReal[i]); // 500Hz
      if (i >7   && i<=15 )  displayBand(3,(int)vReal[i]); // 1000Hz
      if (i >15  && i<=40 )  displayBand(4,(int)vReal[i]); // 2000Hz
      if (i >40  && i<=70 )  displayBand(5,(int)vReal[i]); // 4000Hz
      if (i >70  && i<=288 ) displayBand(6,(int)vReal[i]); // 8000Hz
      if (i >288           ) displayBand(7,(int)vReal[i]); // 16000Hz
       Serial.printf(">vReal:%ld\n", vReal[i]);
    }
    //for (byte band = 0; band <= 7; band++) display.drawHorizontalLine(1+16*band,64-peak[band],14);
  }
}

void SpectrumAnalyzerPlugin::displayBand(int band, int dsize){
  int dmax = 13;
  dsize /= 1500; //amplitude
  if (dsize > dmax) dsize = dmax;
  for (int s = 0; s <= dsize; s=s+2)
  {
    Screen.drawLine(band, 16, band, 16 - s, 1);
    if (band == 1){
      Serial.printf(">vBand:%ld\n", dsize);
    }

    //display.drawHorizontalLine(1+16*band,64-s, 14);
  }
  if (dsize > peak[band]) {peak[band] = dsize;}
}

const char *SpectrumAnalyzerPlugin::getName() const
{
  return "Spectrum Analyzer";
}
