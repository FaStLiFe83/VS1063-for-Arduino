/**
 * VS1063 Arduino Library - Полнофункциональная библиотека для работы с аудиокодеком VS1063
 * Версия 1.0
 * 
 * Поддерживает все основные функции из даташита:
 * - Воспроизведение: MP3, Ogg Vorbis, AAC, WMA, FLAC, WAV
 * - Запись: MP3, Ogg Vorbis, PCM, ADPCM, G.711, G.722
 * - Полнодуплексные кодеки
 * - DSP-эффекты: эквалайзер, басы/высокие, пространственная обработка
 * - Управление скоростью воспроизведения
 * - Работа с плагинами
 * - Продвинутые функции: clock adjustment, sample rate control
 */

#ifndef VS1063_h
#define VS1063_h

#include <Arduino.h>
#include <SPI.h>

class VS1063 {
public:
  // Конструктор (пины управления)
  VS1063(uint8_t xcsPin, uint8_t xdcsPin, uint8_t dreqPin, uint8_t resetPin);
  
  // ===== Базовые функции =====
  void begin();                           // Инициализация кодекса
  void softReset();                       // Программный сброс
  void hardReset();                       // Аппаратный сброс
  bool isReady();                         // Проверка готовности кодекса
  
  // ===== Воспроизведение =====
  void startPlayback();                   // Начать воспроизведение
  void stopPlayback();                    // Остановить воспроизведение
  void pausePlayback(bool pause);         // Пауза/продолжение
  void setVolume(uint8_t left, uint8_t right); // Громкость (0=max, 255=min)
  uint16_t getDecodeTime();               // Время воспроизведения в секундах
  
  // ===== Запись =====
  enum RecordFormat {
    REC_MP3,      // MP3 encoding
    REC_OGG,      // Ogg Vorbis encoding
    REC_PCM,      // 16-bit PCM
    REC_ADPCM,    // IMA ADPCM
    REC_G711_ULAW, // G.711 μ-law
    REC_G711_ALAW, // G.711 A-law
    REC_G722      // G.722 ADPCM
  };
  
  void startRecording(RecordFormat format, uint32_t sampleRate = 44100); // Начать запись
  void stopRecording();                     // Остановить запись
  bool isRecording();                       // Проверка состояния записи
  uint16_t getRecordingData(uint8_t* buf, uint16_t len); // Чтение записанных данных
  
  // ===== Эффекты и обработка =====
  void setBass(uint8_t bassBoost, uint8_t freqLimit); // Усиление басов (0-15, 0=off)
  void setTreble(uint8_t trebleBoost, uint8_t freqLimit); // Усиление ВЧ (0-15, 0=off)
  void setEqualizer5Band(int8_t* gains);   // 5-полосный эквалайзер (-15..+15 dB)
  void setEarSpeaker(bool enable);         // Пространственная обработка EarSpeaker
  void setSpeed(uint16_t percent);         // Изменение скорости (50-200%)
  void setADMixer(uint8_t gain);           // Микшер АЦП (0-7)
  void setPCMMixer(uint8_t volume);        // Микшер PCM (0-255)
  
  // ===== Работа с данными =====
  void writeData(uint8_t* data, uint16_t len); // Запись аудиоданных
  uint16_t readData(uint8_t* buffer, uint16_t len); // Чтение аудиоданных
  
  // ===== Плагины и пользовательский код =====
  void loadPlugin(const uint16_t* plugin, uint16_t len); // Загрузка плагина
  void activatePlugin(uint16_t startAddr);               // Активация плагина
  void deactivatePlugin();                               // Деактивация плагина
  void loadApplication(const uint8_t* image, uint32_t len); // Загрузка приложения
  
  // ===== Продвинутые функции =====
  void setClockFrequency(uint32_t freq);   // Установка частоты (12-13 или 24-26 МГц)
  void setSampleRate(uint32_t rate);       // Установка частоты дискретизации
  void setGPIO(uint8_t pin, bool state);   // Управление GPIO (0-11)
  bool getGPIO(uint8_t pin);               // Чтение состояния GPIO
  
  // ===== Отладочные функции =====
  uint16_t readRegister(uint8_t reg);      // Чтение регистра SCI
  void writeRegister(uint8_t reg, uint16_t value); // Запись регистра SCI
  uint32_t getChipID();                    // Получение уникального ID чипа

private:
  // Пины управления
  uint8_t _xcs, _xdcs, _dreq, _reset;
  
  // Вспомогательные функции
  void writeSCI(uint8_t reg, uint16_t value);
  uint16_t readSCI(uint8_t reg);
  void writeSDI(uint8_t data);
  void waitForDreq();
  void writeRAM(uint16_t addr, uint16_t data);
  uint16_t readRAM(uint16_t addr);
  
  // Состояние
  bool _isRecording;
  bool _isPlaying;
};

#endif
