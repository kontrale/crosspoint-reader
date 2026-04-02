#include "ReadingStatsStore.h"

#include <Arduino.h>
#include <HalStorage.h>
#include <Logging.h>

// Per-book stats.bin: version(1) + totalSeconds(4) + totalPages(4) + sessions(2) = 11 bytes
static constexpr uint8_t BOOK_STATS_VERSION = 1;
static constexpr size_t BOOK_STATS_SIZE = 11;

// Global reading_stats.bin: version(1) + totalSeconds(4) + totalPages(4) + totalSessions(4) = 13 bytes
static constexpr uint8_t GLOBAL_STATS_VERSION = 1;
static constexpr size_t GLOBAL_STATS_SIZE = 13;

static constexpr const char* GLOBAL_STATS_PATH = "/.crosspoint/reading_stats.bin";
static constexpr const char* CSV_EXPORT_PATH = "/.crosspoint/reading_stats.csv";

ReadingStatsStore& ReadingStatsStore::getInstance() {
  static ReadingStatsStore instance;
  return instance;
}

void ReadingStatsStore::beginSession(const std::string& cachePath) {
  _cachePath = cachePath;
  _sessionPages = 0;
  _sessionStartMs = millis();
  _sessionActive = true;
  loadBookStats();
  loadGlobalStats();
  LOG_DBG("STATS", "Session started: %s", cachePath.c_str());
}

void ReadingStatsStore::recordPageTurn() {
  if (_sessionActive) {
    _sessionPages++;
  }
}

void ReadingStatsStore::endSession() {
  if (!_sessionActive) return;
  _sessionActive = false;

  const uint32_t elapsed = getSessionSeconds();
  LOG_DBG("STATS", "Session ended: %us, %u pages", elapsed, _sessionPages);

  _bookStats.totalSecondsRead += elapsed;
  _bookStats.totalPagesRead += _sessionPages;
  _bookStats.sessionsCount++;
  saveBookStats();

  _globalStats.totalSecondsRead += elapsed;
  _globalStats.totalPagesRead += _sessionPages;
  _globalStats.totalSessions++;
  saveGlobalStats();
}

uint32_t ReadingStatsStore::getSessionSeconds() const {
  if (!_sessionActive) return 0;
  return static_cast<uint32_t>((millis() - _sessionStartMs) / 1000UL);
}

void ReadingStatsStore::loadBookStats() {
  _bookStats = {};
  if (_cachePath.empty()) return;

  FsFile f;
  const std::string path = _cachePath + "/stats.bin";
  if (Storage.openFileForRead("STATS", path, f)) {
    uint8_t data[BOOK_STATS_SIZE];
    const int n = f.read(data, sizeof(data));
    f.close();
    if (static_cast<size_t>(n) == BOOK_STATS_SIZE && data[0] == BOOK_STATS_VERSION) {
      _bookStats.totalSecondsRead = static_cast<uint32_t>(data[1]) | (static_cast<uint32_t>(data[2]) << 8) |
                                    (static_cast<uint32_t>(data[3]) << 16) | (static_cast<uint32_t>(data[4]) << 24);
      _bookStats.totalPagesRead   = static_cast<uint32_t>(data[5]) | (static_cast<uint32_t>(data[6]) << 8) |
                                    (static_cast<uint32_t>(data[7]) << 16) | (static_cast<uint32_t>(data[8]) << 24);
      _bookStats.sessionsCount    = static_cast<uint16_t>(data[9]) | (static_cast<uint16_t>(data[10]) << 8);
    }
  }
}

void ReadingStatsStore::saveBookStats() const {
  if (_cachePath.empty()) return;
  FsFile f;
  const std::string path = _cachePath + "/stats.bin";
  if (Storage.openFileForWrite("STATS", path, f)) {
    uint8_t data[BOOK_STATS_SIZE];
    data[0] = BOOK_STATS_VERSION;
    data[1] = _bookStats.totalSecondsRead & 0xFF;
    data[2] = (_bookStats.totalSecondsRead >> 8) & 0xFF;
    data[3] = (_bookStats.totalSecondsRead >> 16) & 0xFF;
    data[4] = (_bookStats.totalSecondsRead >> 24) & 0xFF;
    data[5] = _bookStats.totalPagesRead & 0xFF;
    data[6] = (_bookStats.totalPagesRead >> 8) & 0xFF;
    data[7] = (_bookStats.totalPagesRead >> 16) & 0xFF;
    data[8] = (_bookStats.totalPagesRead >> 24) & 0xFF;
    data[9]  = _bookStats.sessionsCount & 0xFF;
    data[10] = (_bookStats.sessionsCount >> 8) & 0xFF;
    f.write(data, sizeof(data));
    f.close();
  }
}

void ReadingStatsStore::loadGlobalStats() {
  _globalStats = {};
  FsFile f;
  if (Storage.openFileForRead("STATS", GLOBAL_STATS_PATH, f)) {
    uint8_t data[GLOBAL_STATS_SIZE];
    const int n = f.read(data, sizeof(data));
    f.close();
    if (static_cast<size_t>(n) == GLOBAL_STATS_SIZE && data[0] == GLOBAL_STATS_VERSION) {
      _globalStats.totalSecondsRead = static_cast<uint32_t>(data[1]) | (static_cast<uint32_t>(data[2]) << 8) |
                                      (static_cast<uint32_t>(data[3]) << 16) | (static_cast<uint32_t>(data[4]) << 24);
      _globalStats.totalPagesRead   = static_cast<uint32_t>(data[5]) | (static_cast<uint32_t>(data[6]) << 8) |
                                      (static_cast<uint32_t>(data[7]) << 16) | (static_cast<uint32_t>(data[8]) << 24);
      _globalStats.totalSessions    = static_cast<uint32_t>(data[9]) | (static_cast<uint32_t>(data[10]) << 8) |
                                      (static_cast<uint32_t>(data[11]) << 16) | (static_cast<uint32_t>(data[12]) << 24);
    }
  }
}

void ReadingStatsStore::saveGlobalStats() const {
  FsFile f;
  if (Storage.openFileForWrite("STATS", GLOBAL_STATS_PATH, f)) {
    uint8_t data[GLOBAL_STATS_SIZE];
    data[0] = GLOBAL_STATS_VERSION;
    data[1] = _globalStats.totalSecondsRead & 0xFF;
    data[2] = (_globalStats.totalSecondsRead >> 8) & 0xFF;
    data[3] = (_globalStats.totalSecondsRead >> 16) & 0xFF;
    data[4] = (_globalStats.totalSecondsRead >> 24) & 0xFF;
    data[5] = _globalStats.totalPagesRead & 0xFF;
    data[6] = (_globalStats.totalPagesRead >> 8) & 0xFF;
    data[7] = (_globalStats.totalPagesRead >> 16) & 0xFF;
    data[8] = (_globalStats.totalPagesRead >> 24) & 0xFF;
    data[9]  = _globalStats.totalSessions & 0xFF;
    data[10] = (_globalStats.totalSessions >> 8) & 0xFF;
    data[11] = (_globalStats.totalSessions >> 16) & 0xFF;
    data[12] = (_globalStats.totalSessions >> 24) & 0xFF;
    f.write(data, sizeof(data));
    f.close();
  }
}

void ReadingStatsStore::exportCsv(const std::string& bookTitle) const {
  FsFile f;
  if (!Storage.openFileForWrite("STATS", CSV_EXPORT_PATH, f)) return;

  const char* header = "scope,title,total_seconds,total_pages,sessions\r\n";
  f.write(reinterpret_cast<const uint8_t*>(header), strlen(header));

  char row[256];
  snprintf(row, sizeof(row), "book,\"%s\",%lu,%lu,%u\r\n",
           bookTitle.c_str(),
           static_cast<unsigned long>(_bookStats.totalSecondsRead),
           static_cast<unsigned long>(_bookStats.totalPagesRead),
           _bookStats.sessionsCount);
  f.write(reinterpret_cast<const uint8_t*>(row), strlen(row));

  snprintf(row, sizeof(row), "global,\"All Books\",%lu,%lu,%lu\r\n",
           static_cast<unsigned long>(_globalStats.totalSecondsRead),
           static_cast<unsigned long>(_globalStats.totalPagesRead),
           static_cast<unsigned long>(_globalStats.totalSessions));
  f.write(reinterpret_cast<const uint8_t*>(row), strlen(row));

  f.close();
  LOG_INF("STATS", "Exported CSV to %s", CSV_EXPORT_PATH);
}
