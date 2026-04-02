#include "EpubReaderSearchActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <Logging.h>
#include <MappedInputManager.h>
#include <algorithm>

#include "activities/ActivityResult.h"
#include "CrossPointSettings.h"
#include "components/UITheme.h"
#include "fontIds.h"

void EpubReaderSearchActivity::onEnter() {
  Activity::onEnter();
  currentState = UIState::QUERY_INPUT;
  searchQuery.clear();
  searchResults.clear();
  selectedResultIndex = 0;
  LOG_DBG("SRCH", "Search activity entered");
}

void EpubReaderSearchActivity::onExit() {
  Activity::onExit();
  LOG_DBG("SRCH", "Search activity exited");
}

void EpubReaderSearchActivity::performSearch() {
  if (searchQuery.empty()) {
    currentState = UIState::QUERY_INPUT;
    return;
  }

  LOG_INF("SRCH", "Performing search for: '%s'", searchQuery.c_str());
  searchResults = searchEngine.search(searchQuery);

  if (searchResults.empty()) {
    currentState = UIState::EMPTY_RESULTS;
    LOG_DBG("SRCH", "No results found");
  } else {
    currentState = UIState::SHOWING_RESULTS;
    selectedResultIndex = 0;
    LOG_DBG("SRCH", "Found %zu results", searchResults.size());
  }
}

void EpubReaderSearchActivity::selectResult() {
  if (currentState == UIState::SHOWING_RESULTS && selectedResultIndex < searchResults.size()) {
    const auto& result = searchResults[selectedResultIndex];
    LOG_DBG("SRCH", "Navigating to spine %d, page %d", result.spineIndex, result.pageNumber);
    setResult(SearchResultData{result.spineIndex, result.pageNumber});
    finish();
  }
}

void EpubReaderSearchActivity::handleInput() {
  using ButtonPress = MappedInputManager::ButtonPress;

  auto buttons = mappedInput.getAndClearPresses();
  if (buttons.empty()) {
    return;
  }

  for (const auto& button : buttons) {
    switch (currentState) {
      case UIState::QUERY_INPUT: {
        if (button == ButtonPress::LEFT || button == ButtonPress::BACK) {
          // Close search without performing
          finish();
        } else if (button == ButtonPress::RIGHT || button == ButtonPress::CONFIRM) {
          // Perform search with current query
          performSearch();
        } else if (button == ButtonPress::UP) {
          // Delete last character from query
          if (!searchQuery.empty()) {
            searchQuery.pop_back();
            requestUpdate();
          }
        }
        // DOWN button could be used for character cycling in a full text input
        // For now, search is triggered with RIGHT/CONFIRM
        break;
      }

      case UIState::SHOWING_RESULTS: {
        if (button == ButtonPress::UP) {
          if (selectedResultIndex > 0) {
            selectedResultIndex--;
            requestUpdate();
          }
        } else if (button == ButtonPress::DOWN) {
          if (selectedResultIndex < static_cast<int>(searchResults.size()) - 1) {
            selectedResultIndex++;
            requestUpdate();
          }
        } else if (button == ButtonPress::CONFIRM || button == ButtonPress::RIGHT) {
          selectResult();
        } else if (button == ButtonPress::BACK || button == ButtonPress::LEFT) {
          // Return to query input
          currentState = UIState::QUERY_INPUT;
          requestUpdate();
        }
        break;
      }

      case UIState::EMPTY_RESULTS: {
        if (button == ButtonPress::BACK || button == ButtonPress::LEFT) {
          // Return to query input
          currentState = UIState::QUERY_INPUT;
          requestUpdate();
        }
        break;
      }
    }
  }
}

void EpubReaderSearchActivity::renderQueryInput(RenderLock& lock) {
  auto page = Page::create(renderer.getDisplayWidth(), renderer.getDisplayHeight());

  // Title
  int y = 20;
  page->drawText(I18N.get(StrId::STR_SEARCH), 20, y, UITheme::getTheme().colorText, 2);
  y += 40;

  // Instructions
  page->drawText("Enter search query:", 20, y, UITheme::getTheme().colorText, 0);
  y += 25;

  // Query input field (with simple text display)
  std::string displayQuery = searchQuery;
  if (displayQuery.empty()) {
    displayQuery = "[Type here]";
  }
  page->drawText(displayQuery, 20, y, UITheme::getTheme().colorHighlight, 1);
  y += 35;

  // Instructions for navigation
  page->drawText("UP: Delete char  RIGHT: Search  BACK: Cancel", 20, y, UITheme::getTheme().colorText, 0);

  renderer.renderPage(std::move(page));
}

void EpubReaderSearchActivity::renderResults(RenderLock& lock) {
  auto page = Page::create(renderer.getDisplayWidth(), renderer.getDisplayHeight());

  // Title with result count
  std::string title = "Search Results (" + std::to_string(searchResults.size()) + ")";
  page->drawText(title, 20, 20, UITheme::getTheme().colorText, 2);

  // Search query reminder
  page->drawText("Query: " + searchQuery, 20, 55, UITheme::getTheme().colorText, 0);

  int y = 85;
  const int itemHeight = 65;
  const int maxResults = (renderer.getDisplayHeight() - y - 40) / itemHeight;

  // Display results
  for (int i = 0; i < std::min(static_cast<int>(searchResults.size()), maxResults); ++i) {
    const auto& result = searchResults[i];
    bool isSelected = (i == selectedResultIndex);

    // Draw selection highlight
    if (isSelected) {
      page->fillRect(15, y - 5, renderer.getDisplayWidth() - 30, itemHeight - 10,
                     UITheme::getTheme().colorHighlight);
    }

    // Draw location
    std::string location = searchEngine.getLocationString(result);
    page->drawText(location, 25, y, isSelected ? UITheme::getTheme().colorBackground : UITheme::getTheme().colorText,
                   0);

    // Draw context (truncated)
    std::string context = result.contextBefore + " " + result.matchText + " " + result.contextAfter;
    if (context.length() > 50) {
      context = context.substr(0, 47) + "...";
    }
    page->drawText(context, 25, y + 20, UITheme::getTheme().colorText, 0);

    y += itemHeight;
  }

  // Instructions
  int bottomY = renderer.getDisplayHeight() - 30;
  page->drawText("UP/DOWN: Navigate  CONFIRM: Jump to result  BACK: Edit query", 20, bottomY,
                 UITheme::getTheme().colorText, 0);

  renderer.renderPage(std::move(page));
}

void EpubReaderSearchActivity::renderEmptyResults(RenderLock& lock) {
  auto page = Page::create(renderer.getDisplayWidth(), renderer.getDisplayHeight());

  page->drawText("No Results", 20, 50, UITheme::getTheme().colorText, 2);
  page->drawText("Query: " + searchQuery, 20, 100, UITheme::getTheme().colorText, 0);
  page->drawText("No matches found. Try a different search term.", 20, 150, UITheme::getTheme().colorText, 0);

  int bottomY = renderer.getDisplayHeight() - 30;
  page->drawText("BACK: Edit query  LEFT: Close search", 20, bottomY, UITheme::getTheme().colorText, 0);

  renderer.renderPage(std::move(page));
}

void EpubReaderSearchActivity::loop() {
  handleInput();
}

void EpubReaderSearchActivity::render(RenderLock&& lock) {
  switch (currentState) {
    case UIState::QUERY_INPUT:
      renderQueryInput(lock);
      break;
    case UIState::SHOWING_RESULTS:
      renderResults(lock);
      break;
    case UIState::EMPTY_RESULTS:
      renderEmptyResults(lock);
      break;
  }
}
