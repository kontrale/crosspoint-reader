#pragma once

#include <Epub.h>
#include <memory>
#include <string>
#include <vector>

#include "activities/Activity.h"
#include "util/TextSearchEngine.h"

/**
 * EpubReaderSearchActivity provides in-book search functionality.
 * 
 * Users can:
 * - Enter a search query (using the keyboard activity)
 * - View search results with chapter/page information
 * - Navigate between results
 * - Jump to a result location in the book
 */
class EpubReaderSearchActivity final : public Activity {
 public:
  explicit EpubReaderSearchActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                    const std::shared_ptr<Epub>& epub)
      : Activity("EpubReaderSearch", renderer, mappedInput), epub(epub), searchEngine(epub, renderer) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  enum class UIState {
    SHOWING_RESULTS,  // Showing list of results
    EMPTY_RESULTS     // No results found
  };

  std::shared_ptr<Epub> epub;

  UIState currentState = UIState::EMPTY_RESULTS;
  std::string searchQuery;
  std::vector<TextSearchEngine::SearchResult> searchResults;
  int selectedResultIndex = 0;

  TextSearchEngine searchEngine;

  /**
   * Perform the actual search with current query
   */
  void performSearch();

  /**
   * Render the results list
   */
  void renderResults(RenderLock& lock);

  /**
   * Render empty results message
   */
  void renderEmptyResults(RenderLock& lock);

  /**
   * Handle button input based on current state
   */
  void handleInput();

  /**
   * Launch the keyboard activity for text input
   */
  void launchKeyboardInput();

  /**
   * Handle result from keyboard activity
   */
  void onKeyboardResult(const ActivityResult& result);

  /**
   * Handle result from search result selection
   */
  void onResultSelected(const TextSearchEngine::SearchResult& result);
};
