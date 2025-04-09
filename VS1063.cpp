#include "VS1063.h"
#include <SPI.h>

VS1063::VS1063(uint8_t xcsPin, uint8_t xdcsPin, uint8_t dreqPin, uint8_t resetPin) :
  _xcs(xcsPin), _xdcs(xdcsPin), _dreq(dreqPin), _reset(resetPin),
  _isRecording(false), _isPlaying(false) {}

void VS1063::begin() {
  // Инициализация пинов
  pinMode(_xcs, OUTPUT);
  pinMode(_xdcs, OUTPUT);
  pinMode(_dreq, INPUT);
  pinMode(_reset, OUTPUT);
  
  digitalWrite(_xcs, HIGH);
  digitalWrite(_xdcs, HIGH);
  digitalWrite(_reset, LOW); // Держим в сбросе
  
  // Инициализация SPI
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  
  // Сброс и инициализация
  hardReset();
  
  // Настройка по умолчанию
  setClockFrequency(12288000); // 12.288 МГц
  setVolume(40, 40); // Средняя громкость
}

void VS1063::softReset() {
  writeSCI(SCI_MODE, readSCI(SCI_MODE) | 0x0004); // Установка бита сброса
  delay(10);
  writeSCI(SCI_MODE, readSCI(SCI_MODE) & ~0x0004); // Сброс бита
  delay(100);
  _isPlaying = false;
  _isRecording = false;
}

void VS1063::hardReset() {
  digitalWrite(_reset, LOW);
  delay(10);
  digitalWrite(_reset, HIGH);
  delay(100);
  _isPlaying = false;
  _isRecording = false;
}

bool VS1063::isReady() {
  return digitalRead(_dreq) == HIGH;
}

// ===== Функции воспроизведения =====
void VS1063::startPlayback() {
  writeSCI(SCI_MODE, readSCI(SCI_MODE) | 0x0800); // Установка SM_PLAY
  _isPlaying = true;
}

void VS1063::stopPlayback() {
  writeSCI(SCI_MODE, readSCI(SCI_MODE) & ~0x0800); // Сброс SM_PLAY
  _isPlaying = false;
}

void VS1063::pausePlayback(bool pause) {
  if (pause) {
    writeSCI(SCI_MODE, readSCI(SCI_MODE) | 0x0008); // Установка SM_PAUSE
  } else {
    writeSCI(SCI_MODE, readSCI(SCI_MODE) & ~0x0008); // Сброс SM_PAUSE
  }
}

void VS1063::setVolume(uint8_t left, uint8_t right) {
  writeSCI(SCI_VOL, ((uint16_t)left << 8) | right);
}

uint16_t VS1063::getDecodeTime() {
  return readSCI(SCI_DECODE_TIME);
}

// ===== Функции записи =====
void VS1063::startRecording(RecordFormat format, uint32_t sampleRate) {
  uint16_t mode = readSCI(SCI_MODE);
  mode &= ~0x5400; // Сброс SM_ADPCM, SM_LINE1, SM_LINE2
  mode |= 0x4000;  // Установка SM_RECORD
  
  switch (format) {
    case REC_MP3: mode |= 0x1000; break;
    case REC_OGG: mode |= 0x1000; break;
    case REC_ADPCM: mode |= 0x0400; break;
    case REC_G711_ULAW: mode |= 0x1400; break;
    case REC_G711_ALAW: mode |= 0x2400; break;
    case REC_G722: mode |= 0x3400; break;
    case REC_PCM: break; // Без дополнительных битов
  }
  
  writeSCI(SCI_MODE, mode);
  writeSCI(SCI_AUDATA, (sampleRate >> 16) & 0xFF);
  writeSCI(SCI_AUDATA, sampleRate & 0xFFFF);
  
  _isRecording = true;
}

void VS1063::stopRecording() {
  writeSCI(SCI_MODE, readSCI(SCI_MODE) & ~0x4000); // Сброс SM_RECORD
  _isRecording = false;
}

bool VS1063::isRecording() {
  return _isRecording;
}

uint16_t VS1063::getRecordingData(uint8_t* buf, uint16_t len) {
  if (!_isRecording || !isReady()) return 0;
  
  uint16_t i;
  digitalWrite(_xdcs, LOW);
  for (i = 0; i < len && isReady(); i++) {
    buf[i] = SPI.transfer(0x00);
  }
  digitalWrite(_xdcs, HIGH);
  return i;
}

// ===== Эффекты и обработка =====
void VS1063::setBass(uint8_t bassBoost, uint8_t freqLimit) {
  uint16_t bass = (bassBoost << 8) | (freqLimit << 4);
  writeSCI(SCI_BASS, bass);
}

void VS1063::setTreble(uint8_t trebleBoost, uint8_t freqLimit) {
  uint16_t treble = (trebleBoost << 8) | (freqLimit << 4);
  writeSCI(SCI_BASS, readSCI(SCI_BASS) & 0x00FF | treble);
}

void VS1063::setEqualizer5Band(int8_t* gains) {
  // Запись коэффициентов в память кодекса
  writeRAM(0x1E40, (uint16_t)gains[0] + 15); // 80 Hz
  writeRAM(0x1E41, (uint16_t)gains[1] + 15); // 500 Hz
  writeRAM(0x1E42, (uint16_t)gains[2] + 15); // 2.5 kHz
  writeRAM(0x1E43, (uint16_t)gains[3] + 15); // 8 kHz
  writeRAM(0x1E44, (uint16_t)gains[4] + 15); // 14 kHz
  
  // Активация эквалайзера
  writeSCI(SCI_MODE, readSCI(SCI_MODE) | 0x0020);
}

void VS1063::setEarSpeaker(bool enable) {
  if (enable) {
    writeRAM(0x1E09, 0x0001); // Включение EarSpeaker
  } else {
    writeRAM(0x1E09, 0x0000); // Выключение
  }
}

void VS1063::setSpeed(uint16_t percent) {
  uint16_t speed = map(percent, 50, 200, 0, 65535);
  writeRAM(0x1E07, speed); // Установка скорости
  writeRAM(0x1E08, 0x0001); // Включение SpeedShifter
}

void VS1063::setADMixer(uint8_t gain) {
  writeRAM(0x1E05, gain & 0x07); // Установка усиления (0-7)
  writeRAM(0x1E06, 0x0001);      // Включение ADMixer
}

void VS1063::setPCMMixer(uint8_t volume) {
  writeRAM(0x1E03, volume);      // Установка громкости (0-255)
  writeRAM(0x1E04, 0x0001);      // Включение PCMMixer
}

// ===== Работа с данными =====
void VS1063::writeData(uint8_t* data, uint16_t len) {
  if (!isReady()) return;
  
  digitalWrite(_xdcs, LOW);
  for (uint16_t i = 0; i < len; i++) {
    SPI.transfer(data[i]);
  }
  digitalWrite(_xdcs, HIGH);
}

uint16_t VS1063::readData(uint8_t* buffer, uint16_t len) {
  if (!isReady()) return 0;
  
  uint16_t i;
  digitalWrite(_xdcs, LOW);
  for (i = 0; i < len && isReady(); i++) {
    buffer[i] = SPI.transfer(0x00);
  }
  digitalWrite(_xdcs, HIGH);
  return i;
}

// ===== Плагины и пользовательский код =====
void VS1063::loadPlugin(const uint16_t* plugin, uint16_t len) {
  uint16_t i = 0;
  while (i < len) {
    uint16_t addr = plugin[i++];
    uint16_t n = plugin[i++];
    
    if (n & 0x8000) { // RLE-кодирование
      n &= 0x7FFF;
      uint16_t val = plugin[i++];
      writeSCI(SCI_WRAMADDR, addr);
      while (n--) {
        writeSCI(SCI_WRAM, val);
      }
    } else { // Обычные данные
      writeSCI(SCI_WRAMADDR, addr);
      while (n--) {
        writeSCI(SCI_WRAM, plugin[i++]);
      }
    }
  }
}

void VS1063::activatePlugin(uint16_t startAddr) {
  writeSCI(SCI_AIADDR, startAddr);
}

void VS1063::deactivatePlugin() {
  writeSCI(SCI_AIADDR, 0);
}

void VS1063::loadApplication(const uint8_t* image, uint32_t len) {
  // Проверка заголовка
  if (image[0] != 'P' || image[1] != '&' || image[2] != 'H') return;
  
  uint32_t pos = 3;
  while (pos < len) {
    uint8_t type = image[pos++];
    if (type > 3) break; // Неверный тип
    
    uint16_t recLen = (image[pos] << 8) | image[pos+1];
    pos += 2;
    uint16_t addr = (image[pos] << 8) | image[pos+1];
    pos += 2;
    
    if (type == 3) { // Execute record
      writeSCI(SCI_AIADDR, addr);
      break;
    }
    
    writeSCI(SCI_WRAMADDR, addr);
    for (uint16_t i = 0; i < recLen/2; i++) {
      uint16_t data = (image[pos] << 8) | image[pos+1];
      pos += 2;
      writeSCI(SCI_WRAM, data);
    }
  }
}

// ===== Продвинутые функции =====
void VS1063::setClockFrequency(uint32_t freq) {
  uint16_t clockf;
  if (freq <= 13000000) { // 12-13 МГц
    clockf = 0x8000 | ((freq / 4000) & 0x7FFF);
  } else { // 24-26 МГц
    clockf = 0x9000 | ((freq / 8000) & 0x7FFF);
  }
  writeSCI(SCI_CLOCKF, clockf);
}

void VS1063::setSampleRate(uint32_t rate) {
  writeSCI(SCI_AUDATA, rate);
}

void VS1063::setGPIO(uint8_t pin, bool state) {
  if (pin > 11) return;
  uint16_t gpio = readSCI(SCI_HDAT0);
  if (state) {
    gpio |= (1 << pin);
  } else {
    gpio &= ~(1 << pin);
  }
  writeSCI(SCI_HDAT0, gpio);
}

bool VS1063::getGPIO(uint8_t pin) {
  if (pin > 11) return false;
  return (readSCI(SCI_HDAT1) & (1 << pin)) != 0;
}

// ===== Отладочные функции =====
uint16_t VS1063::readRegister(uint8_t reg) {
  return readSCI(reg);
}

void VS1063::writeRegister(uint8_t reg, uint16_t value) {
  writeSCI(reg, value);
}

uint32_t VS1063::getChipID() {
  return ((uint32_t)readSCI(SCI_HDAT0) << 16) | readSCI(SCI_HDAT1);
}

// ===== Приватные функции =====
void VS1063::writeSCI(uint8_t reg, uint16_t value) {
  waitForDreq();
  digitalWrite(_xcs, LOW);
  SPI.transfer(0x02); // Write command
  SPI.transfer(reg);
  SPI.transfer((value >> 8) & 0xFF);
  SPI.transfer(value & 0xFF);
  digitalWrite(_xcs, HIGH);
}

uint16_t VS1063::readSCI(uint8_t reg) {
  waitForDreq();
  digitalWrite(_xcs, LOW);
  SPI.transfer(0x03); // Read command
  SPI.transfer(reg);
  uint16_t val = (SPI.transfer(0xFF) << 8);
  val |= SPI.transfer(0xFF);
  digitalWrite(_xcs, HIGH);
  return val;
}

void VS1063::writeSDI(uint8_t data) {
  waitForDreq();
  digitalWrite(_xdcs, LOW);
  SPI.transfer(data);
  digitalWrite(_xdcs, HIGH);
}

void VS1063::waitForDreq() {
  while (digitalRead(_dreq) == LOW) {
    delayMicroseconds(10);
  }
}

void VS1063::writeRAM(uint16_t addr, uint16_t data) {
  writeSCI(SCI_WRAMADDR, addr);
  writeSCI(SCI_WRAM, data);
}

uint16_t VS1063::readRAM(uint16_t addr) {
  writeSCI(SCI_WRAMADDR, addr);
  return readSCI(SCI_WRAM);
}