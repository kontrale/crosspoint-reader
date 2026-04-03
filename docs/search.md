# Search Within Book

The Search Within Book feature allows users to search for text across the entire content of an EPUB book.

## User Workflow

1. Open a book in the EPUB reader
2. Press the **Search** button from the reader menu
3. The keyboard activity appears - use up/down buttons to navigate characters
4. Press **Confirm** to search or **Back** to cancel
5. View search results displayed with chapter/page location information
6. Use up/down buttons to navigate between results
7. Press **Confirm** to jump to that location in the book

## Architecture

### Components

**TextSearchEngine** (`src/util/TextSearchEngine.h/cpp`)
- Core search algorithm that operates on EPUB content
- Performs case-insensitive full-text matching
- Returns search results with context (50 characters before/after match)
- Limits results to 100 to manage memory on embedded device
- Extracts text from pages by iterating TextBlocks and joining words

**EpubReaderSearchActivity** (`src/activities/reader/EpubReaderSearchActivity.h/cpp`)
- Activity-based UI for search interaction
- Uses `KeyboardEntryActivity` for button-based text input (non-touchscreen compatible)
- States:
  - `SHOWING_RESULTS`: Display list of matches with navigation
  - `EMPTY_RESULTS`: Display "no matches" message
- Handles button input for result navigation and selection

**Integration Points**
- `EpubReaderMenuActivity`: Search menu action integrated into reader menu
- `EpubReaderActivity`: Result handler that navigates to selected search location
- `ActivityResult.h`: `SearchResultData` struct carries spine index and page number

### Button Navigation

**Text Entry (KeyboardEntryActivity)**
- Up/Down: Navigate character selection
- Confirm: Execute search or select character
- Back: Cancel text entry

**Results List**
- Up/Down: Navigate between search results
- Confirm: Jump to selected result location
- Back: Close search and return to reader

## Implementation Details

### Text Extraction

Text is extracted from EPUB pages using the existing page structure:
```
Page → PageLine (via getTag() == TAG_PageLine) → TextBlock → words
```

Words from TextBlocks are joined with spaces, and PageLines are separated with newlines.

### Search Algorithm

1. Normalize query and page text to lowercase for case-insensitive matching
2. Iterate through all sections of the EPUB
3. Load pages from each section
4. Extract text and search for query matches
5. For each match, capture context (50 chars before/after)
6. Return results with spine index, page number, and location string

### Memory Management

- Limits to 100 results maximum to prevent excessive memory usage
- Streams through sections and pages (doesn't load entire book at once)
- SearchResult struct is lightweight: spine index, page number, and string context

## Internationalization

The search feature is fully internationalized (19 languages). The primary string `STR_SEARCH` is translated in all language files under `src/activities/reader/i18n/`.

## Testing

To test search functionality:
1. Build the firmware: `pio run`
2. Flash to device
3. Open an EPUB book
4. Press Search from the reader menu
5. Enter a search term using the button-based keyboard
6. Verify results appear with correct chapter/page information
7. Navigate and select results
8. Confirm navigation to the correct location in the book

## Future Enhancements

Possible improvements for future versions:
- Search history
- Regex pattern search
- Search result filtering by chapter
- Bookmark search results
- Advanced search (AND/OR/NOT operators)
