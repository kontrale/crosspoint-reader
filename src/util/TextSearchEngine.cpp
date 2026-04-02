#include "TextSearchEngine.h"

#include <Logging.h>
#include <Epub/Section.h>
#include <Epub/Page.h>
#include <Epub/blocks/TextBlock.h>
#include <cctype>
#include <algorithm>

std::string TextSearchEngine::normalize(const std::string& text) {
  std::string result;
  for (char c : text) {
    result += std::tolower(static_cast<unsigned char>(c));
  }
  return result;
}

std::string TextSearchEngine::extractContext(const std::string& text, size_t matchPos, bool before,
                                               int contextLength) {
  if (before) {
    // Extract context before the match
    int startPos = std::max(0, static_cast<int>(matchPos) - contextLength);
    if (startPos > 0) {
      return "..." + text.substr(startPos, matchPos - startPos);
    }
    return text.substr(0, matchPos);
  } else {
    // Extract context after the match
    int endPos = std::min(static_cast<int>(text.length()), static_cast<int>(matchPos) + contextLength);
    if (endPos < static_cast<int>(text.length())) {
      return text.substr(matchPos, endPos - matchPos) + "...";
    }
    return text.substr(matchPos);
  }
}

/**
 * Extract all text from a page into a single string
 * This reconstructs the continuous text from TextBlocks
 */
static std::string extractTextFromPage(const std::unique_ptr<Page>& page) {
  std::string pageText;
  if (!page) {
    return pageText;
  }

  // Get all elements from the page
  // Note: This is a simplified extraction - in reality we'd need to iterate through
  // the page's elements and extract text from TextBlocks
  // The Page class stores PageElements which can be PageLine or PageImage
  // For now, we'll use a basic approach
  
  return pageText;
}

std::vector<TextSearchEngine::SearchResult> TextSearchEngine::search(const std::string& query) {
  std::vector<SearchResult> results;

  if (!epub || query.empty()) {
    return results;
  }

  std::string normalizedQuery = normalize(query);
  int spineCount = epub->getSpineItemsCount();

  LOG_INF("SEARCH", "Starting search for '%s' in %d sections", query.c_str(), spineCount);

  // Stream through each spine item (chapter)
  for (int spineIndex = 0; spineIndex < spineCount; ++spineIndex) {
    auto section = std::make_unique<Section>(epub, spineIndex, nullptr);  // Note: renderer will be set in real implementation
    
    // For now, we'll implement a simplified version
    // In production, this would load the section cache and parse pages
    // This is a placeholder that shows the algorithm structure
    
    LOG_DBG("SEARCH", "Searching spine index %d", spineIndex);
  }

  LOG_INF("SEARCH", "Search completed, found %zu results", results.size());
  return results;
}

std::string TextSearchEngine::getLocationString(const SearchResult& result) const {
  if (!epub) {
    return "Unknown location";
  }

  // Get TCP entry for spine index to get chapter name
  int tocIndex = epub->getTocIndexForSpineIndex(result.spineIndex);
  if (tocIndex >= 0 && tocIndex < epub->getTocItemsCount()) {
    auto tocEntry = epub->getTocItem(tocIndex);
    return tocEntry.label + ", Page " + std::to_string(result.pageNumber + 1);
  }

  return "Chapter " + std::to_string(result.spineIndex) + ", Page " + std::to_string(result.pageNumber + 1);
}
