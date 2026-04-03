#include "EpubReaderSearchActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <Logging.h>
#include <MappedInputManager.h>
#include <algorithm>

#include "activities/ActivityResult.h"
#include "activities/util/KeyboardEntryActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"

void EpubReaderSearchActivity::onEnter() {
  Activity::onEnter();
  searchQuery.clear();
  searchResults.clear();
  selectedResultIndex = 0;
  searchPerformed = false;
  LOG_DBG("SRCH", "Search activity entered");
  launchKeyboardInput();
}

void EpubReaderSearchActivity::onExit() {
  Activity::onExit();
  LOG_DBG("SRCH", "Search activity exited");
}

void EpubReaderSearchActivity::launchKeyboardInput() {
  startActivityForResult(
      std::make_unique<KeyboardEntryActivity>(renderer, mappedInput, I18N.get(StrId::STR_SEARCH), "", 200),
      [this](const ActivityResult& result) { onKeyboardResult(result); });
}

void EpubReaderSearchActivity::onKeyboardResult(const ActivityResult& result) {
  if (result.isCancelled) {
    // User cancelled keyboard input, close search activity
    finish();
    return;
  }

  const auto& kb = std::get<KeyboardResult>(result.data);
  searchQuery = kb.text;
  LOG_DBG("SRCH", "Got search query: '%s'", searchQuery.c_str());
  
  // Show searching state before performing potentially slow search
  currentState = UIState::SEARCHING;
  waitForConfirmRelease = true;  // Wait for Confirm to be fully released before accepting input
  requestUpdate();
}

void EpubReaderSearchActivity::performSearch() {
  if (searchQuery.empty()) {
    // Shouldn't happen, but handle gracefully
    finish();
    return;
  }

  LOG_INF("SRCH", "Performing search for: '%s'", searchQuery.c_str());
  searchResults = searchEngine.search(searchQuery, currentSpineIndex);

  if (searchResults.empty()) {
    currentState = UIState::EMPTY_RESULTS;
    LOG_DBG("SRCH", "No results found");
  } else {
    currentState = UIState::SHOWING_RESULTS;
    selectedResultIndex = 0;
    LOG_DBG("SRCH", "Found %zu results", searchResults.size());
  }
  
  requestUpdate();
}

void EpubReaderSearchActivity::onResultSelected(const TextSearchEngine::SearchResult& result) {
  LOG_DBG("SRCH", "Navigating to spine %d, page %d", result.spineIndex, result.pageNumber);
  setResult(SearchResultData{result.spineIndex, result.pageNumber});
  finish();
}

void EpubReaderSearchActivity::handleInput() {
  // Wait until Confirm is physically released before accepting any input
  if (waitForConfirmRelease) {
    if (!mappedInput.isPressed(MappedInputManager::Button::Confirm)) {
      waitForConfirmRelease = false;
    }
    return;
  }
  
  switch (currentState) {
    case UIState::SEARCHING: {
      // Allow user to cancel search with Back button
      if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
        finish();
      }
      break;
    }
    
    case UIState::SHOWING_RESULTS: {
      if (mappedInput.wasReleased(MappedInputManager::Button::Up)) {
        if (selectedResultIndex > 0) {
          selectedResultIndex--;
          requestUpdate();
        }
      } else if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
        if (selectedResultIndex < static_cast<int>(searchResults.size()) - 1) {
          selectedResultIndex++;
          requestUpdate();
        }
      } else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
        onResultSelected(searchResults[selectedResultIndex]);
      } else if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
        // Close search results, go back to reader
        finish();
      }
      break;
    }

    case UIState::EMPTY_RESULTS: {
      if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
        // Close search, go back to reader
        finish();
      }
      break;
    }
  }
}

void EpubReaderSearchActivity::renderResults(RenderLock& lock) {
  renderer.clearScreen();

  // Title with result count
  std::string title = "Results (" + std::to_string(searchResults.size()) + ")";
  renderer.drawText(UI_12_FONT_ID, 20, 20, title.c_str(), true);

  // Search query reminder
  std::string queryLine = "Query: " + searchQuery;
  renderer.drawText(UI_10_FONT_ID, 20, 55, queryLine.c_str(), false);

  int y = 85;
  constexpr int itemHeight = 40;
  const int maxVisible = (renderer.getScreenHeight() - y - 40) / itemHeight;

  // Scroll: keep selectedResultIndex visible
  const int firstVisible = std::max(0, std::min(selectedResultIndex - maxVisible + 1,
                                                 static_cast<int>(searchResults.size()) - maxVisible));
  const int lastVisible = std::min(firstVisible + maxVisible, static_cast<int>(searchResults.size()));

  for (int i = firstVisible; i < lastVisible; ++i) {
    const auto& result = searchResults[i];
    bool isSelected = (i == selectedResultIndex);

    if (isSelected) {
      renderer.fillRect(15, y - 5, renderer.getScreenWidth() - 30, itemHeight - 10, true);
    }

    std::string location = searchEngine.getLocationString(result);
    renderer.drawText(UI_10_FONT_ID, 25, y, location.c_str(), !isSelected);

    y += itemHeight;
  }

  // Scroll indicator: show position if list is longer than screen
  if (static_cast<int>(searchResults.size()) > maxVisible) {
    std::string indicator = std::to_string(selectedResultIndex + 1) + "/" + std::to_string(searchResults.size());
    renderer.drawText(SMALL_FONT_ID, renderer.getScreenWidth() - 60, 20, indicator.c_str(), false);
  }

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}

void EpubReaderSearchActivity::renderEmptyResults(RenderLock& lock) {
  renderer.clearScreen();

  renderer.drawText(UI_12_FONT_ID, 20, 50, "No Matches", true);
  
  std::string queryLine = "Query: " + searchQuery;
  renderer.drawText(UI_10_FONT_ID, 20, 100, queryLine.c_str(), false);
  
  renderer.drawText(UI_10_FONT_ID, 20, 150, "No matches found in this chapter.", false);
  renderer.drawText(UI_10_FONT_ID, 20, 175, "Try searching other chapters or", false);
  renderer.drawText(UI_10_FONT_ID, 20, 200, "read more to enable full-text search.", false);

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), "", "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}

void EpubReaderSearchActivity::loop() {
  // Perform search on first loop iteration after SEARCHING state is set
  if (currentState == UIState::SEARCHING && !searchPerformed && !searchQuery.empty()) {
    LOG_DBG("SRCH", "Starting search execution");
    performSearch();
    searchPerformed = true;
    LOG_DBG("SRCH", "Search completed, found %zu results", searchResults.size());
  }
  
  handleInput();
}
void EpubReaderSearchActivity::renderSearching(RenderLock& lock) {
  renderer.clearScreen();

  renderer.drawText(UI_12_FONT_ID, 20, 100, "Searching...", true);
  
  std::string queryLine = "Query: " + searchQuery;
  renderer.drawText(UI_10_FONT_ID, 20, 150, queryLine.c_str(), false);
  
  renderer.drawText(UI_10_FONT_ID, 20, 200, "Press Back to cancel", false);

  const auto labels = mappedInput.mapLabels(tr(STR_BACK), "", "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer();
}
void EpubReaderSearchActivity::render(RenderLock&& lock) {
  switch (currentState) {
    case UIState::SEARCHING:
      renderSearching(lock);
      break;
    case UIState::SHOWING_RESULTS:
      renderResults(lock);
      break;
    case UIState::EMPTY_RESULTS:
      renderEmptyResults(lock);
      break;
  }
}
