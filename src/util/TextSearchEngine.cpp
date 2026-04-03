#include "TextSearchEngine.h"

#include <Logging.h>
#include <Epub/Section.h>
#include <Epub/Page.h>
#include <Epub/blocks/TextBlock.h>
#include <cctype>
#include <algorithm>
#include <cstring>

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
  if (!page || page->elements.empty()) {
    return pageText;
  }

  // Iterate through all page elements and extract text from TextBlocks
  for (const auto& element : page->elements) {
    if (element && element->getTag() == TAG_PageLine) {
      // Use static_cast since tag checking confirms type (RTTI disabled)
      auto* pageLine = static_cast<PageLine*>(element.get());
      const auto& textBlock = pageLine->getBlock();
      if (textBlock) {
        const auto& words = textBlock->getWords();
        // Join words with spaces
        for (size_t i = 0; i < words.size(); ++i) {
          if (i > 0) pageText += " ";
          pageText += words[i];
        }
        // Add newline between lines
        pageText += "\n";
      }
    }
    // Skip PageImage elements for text search
  }
  
  return pageText;
}

std::vector<TextSearchEngine::SearchResult> TextSearchEngine::search(const std::string& query,
                                                                      int startSpineIndex) {
  std::vector<SearchResult> results;

  if (!epub || query.empty()) {
    return results;
  }

  std::string normalizedQuery = normalize(query);
  int spineCount = epub->getSpineItemsCount();
  
  // Clamp startSpineIndex to valid range
  startSpineIndex = std::max(0, std::min(startSpineIndex, spineCount - 1));

  LOG_INF("SEARCH", "Starting search for '%s' (%s) in %d sections from index %d", query.c_str(),
          normalizedQuery.c_str(), spineCount, startSpineIndex);

  // Search starting from current spine, then wrap around
  for (int i = 0; i < spineCount; ++i) {
    int spineIndex = (startSpineIndex + i) % spineCount;
    LOG_DBG("SEARCH", "Creating section for spine index %d", spineIndex);
    
    auto section = std::make_unique<Section>(epub, spineIndex, renderer);

    LOG_DBG("SEARCH", "Searching spine index %d", spineIndex);

    int pageNumber = 0;
    int pagesChecked = 0;
    const int MAX_PAGES_PER_SECTION = 500;  // Safety limit

    // Load pages one by one to conserve memory
    std::unique_ptr<Page> page;
    while ((page = section->loadPageFromSectionFile()) != nullptr && pagesChecked < MAX_PAGES_PER_SECTION) {
      pagesChecked++;
      
      // Extract all text from this page
      std::string pageText = extractTextFromPage(page);
      std::string normalizedPageText = normalize(pageText);

      // Search for first occurrence of the query in this page (one result per page)
      size_t pos = normalizedPageText.find(normalizedQuery);
      if (pos != std::string::npos) {
        LOG_DBG("SEARCH", "Found match in spine %d, page %d at position %zu", spineIndex, pageNumber, pos);

        SearchResult result;
        result.spineIndex = spineIndex;
        result.pageNumber = pageNumber;

        const int CONTEXT_LENGTH = 80;
        result.contextBefore = extractContext(pageText, pos, true, CONTEXT_LENGTH);
        result.contextAfter = extractContext(pageText, pos + query.length(), false, CONTEXT_LENGTH);
        result.matchText = pageText.substr(pos, query.length());  // Use actual cased text from the page

        results.push_back(result);

        if (results.size() >= 100) {
          LOG_INF("SEARCH", "Limiting search results to 100 matches");
          return results;
        }
      }

      pageNumber++;
    }
    
    LOG_DBG("SEARCH", "Finished spine %d, checked %d pages", spineIndex, pagesChecked);
    
    // If no pages were loaded, section files don't exist yet - stop searching
    // (user probably hasn't read this chapter yet)
    if (pagesChecked == 0) {
      // For the first section, this is expected if no pages are cached
      // Return with results found so far
      LOG_INF("SEARCH", "No cached pages found in section %d - search complete with %zu results", spineIndex,
              results.size());
      return results;
    }
  }

  LOG_INF("SEARCH", "Search completed, found %zu results", results.size());
  return results;
}

std::string TextSearchEngine::getLocationString(const SearchResult& result) const {
  if (!epub) {
    return "Unknown location";
  }

  // Get TOC entry for spine index to get chapter name
  int tocIndex = epub->getTocIndexForSpineIndex(result.spineIndex);
  if (tocIndex >= 0 && tocIndex < epub->getTocItemsCount()) {
    auto tocEntry = epub->getTocItem(tocIndex);
    return tocEntry.title + ", Page " + std::to_string(result.pageNumber + 1);
  }

  return "Chapter " + std::to_string(result.spineIndex) + ", Page " + std::to_string(result.pageNumber + 1);
}
