#pragma once

#include <string>

#include "activities/Activity.h"

/**
 * ReadingStatsActivity displays reading statistics for the current book
 * and globally across all books.
 *
 * Stats are read live from ReadingStatsStore (which must have an active session).
 * Confirm exports a CSV to the SD card.
 */
class ReadingStatsActivity final : public Activity {
 public:
  explicit ReadingStatsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, const std::string& bookTitle,
                                const std::string& cachePath)
      : Activity("ReadingStats", renderer, mappedInput), bookTitle(bookTitle), cachePath(cachePath) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  std::string bookTitle;
  std::string cachePath;
  bool exportConfirmed = false;

  static std::string formatDuration(uint32_t seconds);
};
