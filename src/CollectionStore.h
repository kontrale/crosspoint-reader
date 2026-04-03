#pragma once

#include <string>
#include <vector>

/**
 * CollectionStore manages user-defined virtual book collections (tags/shelves).
 *
 * Collections group book file paths under a named shelf (e.g. "Currently Reading").
 * Books are referenced by their SD-card path; metadata is resolved on-demand from
 * RecentBooksStore or EPUB files.
 *
 * Persisted as JSON at /.crosspoint/collections.json.
 */
class CollectionStore {
 public:
  static CollectionStore& getInstance();
  CollectionStore(const CollectionStore&) = delete;
  CollectionStore& operator=(const CollectionStore&) = delete;

  struct Collection {
    std::string id;    // unique string ID (numeric timestamp as string)
    std::string name;
    std::vector<std::string> bookPaths;  // SD paths, e.g. "/books/foo.epub"
  };

  // Access
  const std::vector<Collection>& getCollections() const { return _collections; }
  const Collection* getCollection(const std::string& id) const;
  int getCount() const { return static_cast<int>(_collections.size()); }

  // Mutation (each saves automatically)
  std::string createCollection(const std::string& name);          // returns new collection ID
  void deleteCollection(const std::string& id);
  void addBook(const std::string& collectionId, const std::string& bookPath);
  void removeBook(const std::string& collectionId, const std::string& bookPath);
  bool isBookInCollection(const std::string& collectionId, const std::string& bookPath) const;

  void loadFromFile();
  void saveToFile() const;

 private:
  CollectionStore() = default;

  Collection* findCollection(const std::string& id);

  std::vector<Collection> _collections;
};

#define COLLECTIONS CollectionStore::getInstance()
