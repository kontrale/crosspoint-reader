#pragma once

#include <Epub.h>
#include <GfxRenderer.h>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

class GfxRenderer;

/**
 * TextSearchEngine provides lightweight search functionality for EPUB content.
 * 
 * Optimized for constrained hardware:
 * - Streams through sections rather than loading entire book into memory
 * - Case-insensitive search
 * - Returns results with chapter and page information
 * - No persistent storage of search results (regenerated on each search)
 */
class TextSearchEngine {
 public:
  struct SearchResult {
    int spineIndex;           // Chapter/section index
    int pageNumber;           // Page within the chapter
    std::string contextBefore;  // Text before match (up to 50 chars)
    std::string matchText;      // The matched text
    std::string contextAfter;   // Text after match (up to 50 chars)
  };

  explicit TextSearchEngine(const std::shared_ptr<Epub>& epub, GfxRenderer& renderer)
      : epub(epub), renderer(renderer) {}
  
  /**
   * Search for a query string within the book.
   * Returns all matching results found.
   * 
   * @param query Search text (case-insensitive)
   * @return Vector of search results
   */
  std::vector<SearchResult> search(const std::string& query);

  /**
   * Get human-readable location string for a result
   * Example: "Chapter 5, Page 3"
   */
  std::string getLocationString(const SearchResult& result) const;

 private:
  std::shared_ptr<Epub> epub;
  GfxRenderer& renderer;

  /**
   * Normalize text for comparison (lowercase, trim)
   */
  static std::string normalize(const std::string& text);

  /**
   * Extract context around a match position in text
   */
  static std::string extractContext(const std::string& text, size_t matchPos, bool before, int contextLength = 50);
};
