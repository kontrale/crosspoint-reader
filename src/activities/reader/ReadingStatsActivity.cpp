#include "ReadingStatsActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <cstdio>

#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "util/ReadingStatsStore.h"

void ReadingStatsActivity::onEnter() {
  Activity::onEnter();
  exportConfirmed = false;
  requestUpdate();
}

void ReadingStatsActivity::onExit() { Activity::onExit(); }

void ReadingStatsActivity::loop() {
  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    finish();
    return;
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    ReadingStatsStore::getInstance().exportCsv(bookTitle);
    exportConfirmed = true;
    requestUpdate();
    return;
  }

  // Refresh the session timer display every ~5 seconds
  static unsigned long lastRefresh = 0;
  if (millis() - lastRefresh >= 5000) {
    lastRefresh = millis();
    requestUpdate();
  }
}

void ReadingStatsActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();

  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight},
                 tr(STR_READING_STATS), nullptr);

  const ReadingStatsStore& store = ReadingStatsStore::getInstance();
  const auto& book = store.getBookStats();
  const auto& global = store.getGlobalStats();

  int y = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing + 10;
  const int lineHeight = renderer.getLineHeight(UI_10_FONT_ID) + 6;
  const int sectionGap = 20;
  const int x = 20;

  // --- Current session ---
  renderer.drawText(UI_12_FONT_ID, x, y, tr(STR_STATS_SESSION), true);
  y += renderer.getLineHeight(UI_12_FONT_ID) + 4;

  {
    char buf[64];
    const uint32_t sessionSec = store.getSessionSeconds();
    snprintf(buf, sizeof(buf), "%s  |  %lu pages", formatDuration(sessionSec).c_str(),
             static_cast<unsigned long>(store.getSessionPages()));
    renderer.drawText(UI_10_FONT_ID, x + 10, y, buf, true);
  }
  y += lineHeight + sectionGap;

  // --- This book ---
  renderer.drawLine(x, y, pageWidth - x, y, 1, true);
  y += 8;
  renderer.drawText(UI_12_FONT_ID, x, y, tr(STR_STATS_THIS_BOOK), true);
  y += renderer.getLineHeight(UI_12_FONT_ID) + 4;

  {
    char buf[80];
    snprintf(buf, sizeof(buf), "%s  |  %lu pages  |  %u sessions",
             formatDuration(book.totalSecondsRead).c_str(),
             static_cast<unsigned long>(book.totalPagesRead), book.sessionsCount);
    renderer.drawText(UI_10_FONT_ID, x + 10, y, buf, true);
  }
  y += lineHeight + sectionGap;

  // --- All books ---
  renderer.drawLine(x, y, pageWidth - x, y, 1, true);
  y += 8;
  renderer.drawText(UI_12_FONT_ID, x, y, tr(STR_STATS_ALL_BOOKS), true);
  y += renderer.getLineHeight(UI_12_FONT_ID) + 4;

  {
    char buf[80];
    snprintf(buf, sizeof(buf), "%s  |  %lu pages  |  %lu sessions",
             formatDuration(global.totalSecondsRead).c_str(),
             static_cast<unsigned long>(global.totalPagesRead),
             static_cast<unsigned long>(global.totalSessions));
    renderer.drawText(UI_10_FONT_ID, x + 10, y, buf, true);
  }
  y += lineHeight + sectionGap;

  // --- Export confirmation ---
  if (exportConfirmed) {
    renderer.drawLine(x, y, pageWidth - x, y, 1, true);
    y += 8;
    renderer.drawText(UI_10_FONT_ID, x, y, tr(STR_STATS_EXPORTED), true);
  }

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_STATS_EXPORT_CSV), "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}

std::string ReadingStatsActivity::formatDuration(uint32_t seconds) {
  char buf[32];
  if (seconds < 60) {
    snprintf(buf, sizeof(buf), "%us", static_cast<unsigned>(seconds));
  } else if (seconds < 3600) {
    snprintf(buf, sizeof(buf), "%um", static_cast<unsigned>(seconds / 60));
  } else {
    const unsigned h = seconds / 3600;
    const unsigned m = (seconds % 3600) / 60;
    snprintf(buf, sizeof(buf), "%uh %um", h, m);
  }
  return buf;
}
