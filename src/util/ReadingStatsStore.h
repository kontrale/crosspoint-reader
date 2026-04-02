#pragma once

#include <cstdint>
#include <string>

/**
 * ReadingStatsStore tracks reading time and page turns per book and globally.
 *
 * Per-book stats are persisted to {cachePath}/stats.bin.
 * Global stats are persisted to /.crosspoint/reading_stats.bin.
 *
 * Lifecycle:
 *   beginSession(cachePath)  — call when a book opens (loads saved stats)
 *   recordPageTurn()         — call on each forward page turn
 *   endSession()             — call when the book closes (saves updated stats)
 */
class ReadingStatsStore {
 public:
  static ReadingStatsStore& getInstance();
  ReadingStatsStore(const ReadingStatsStore&) = delete;
  ReadingStatsStore& operator=(const ReadingStatsStore&) = delete;

  struct BookStats {
    uint32_t totalSecondsRead = 0;
    uint32_t totalPagesRead = 0;
    uint16_t sessionsCount = 0;
  };

  struct GlobalStats {
    uint32_t totalSecondsRead = 0;
    uint32_t totalPagesRead = 0;
    uint32_t totalSessions = 0;
  };

  void beginSession(const std::string& cachePath);
  void recordPageTurn();
  void endSession();

  // Live elapsed seconds for the current session (0 if no session active)
  uint32_t getSessionSeconds() const;
  uint32_t getSessionPages() const { return _sessionPages; }
  const BookStats& getBookStats() const { return _bookStats; }
  const GlobalStats& getGlobalStats() const { return _globalStats; }

  // Write per-book and global totals to /.crosspoint/reading_stats.csv
  void exportCsv(const std::string& bookTitle) const;

 private:
  ReadingStatsStore() = default;

  void loadBookStats();
  void saveBookStats() const;
  void loadGlobalStats();
  void saveGlobalStats() const;

  std::string _cachePath;
  BookStats _bookStats;
  GlobalStats _globalStats;

  unsigned long _sessionStartMs = 0;
  uint32_t _sessionPages = 0;
  bool _sessionActive = false;
};
