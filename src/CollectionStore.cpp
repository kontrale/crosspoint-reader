#include "CollectionStore.h"

#include <ArduinoJson.h>
#include <HalStorage.h>
#include <Logging.h>

#include <algorithm>
#include <cstring>

namespace {
constexpr char COLLECTIONS_JSON[] = "/.crosspoint/collections.json";
}

CollectionStore& CollectionStore::getInstance() {
  static CollectionStore instance;
  return instance;
}

const CollectionStore::Collection* CollectionStore::getCollection(const std::string& id) const {
  for (const auto& c : _collections) {
    if (c.id == id) return &c;
  }
  return nullptr;
}

CollectionStore::Collection* CollectionStore::findCollection(const std::string& id) {
  for (auto& c : _collections) {
    if (c.id == id) return &c;
  }
  return nullptr;
}

std::string CollectionStore::createCollection(const std::string& name) {
  // Use millisecond uptime as a simple unique ID
  const std::string id = std::to_string(millis());
  _collections.push_back({id, name, {}});
  saveToFile();
  LOG_INF("COLL", "Created collection '%s' (id=%s)", name.c_str(), id.c_str());
  return id;
}

void CollectionStore::deleteCollection(const std::string& id) {
  _collections.erase(
      std::remove_if(_collections.begin(), _collections.end(), [&](const Collection& c) { return c.id == id; }),
      _collections.end());
  saveToFile();
}

void CollectionStore::addBook(const std::string& collectionId, const std::string& bookPath) {
  Collection* c = findCollection(collectionId);
  if (!c) return;
  if (std::find(c->bookPaths.begin(), c->bookPaths.end(), bookPath) == c->bookPaths.end()) {
    c->bookPaths.push_back(bookPath);
    saveToFile();
  }
}

void CollectionStore::removeBook(const std::string& collectionId, const std::string& bookPath) {
  Collection* c = findCollection(collectionId);
  if (!c) return;
  c->bookPaths.erase(std::remove(c->bookPaths.begin(), c->bookPaths.end(), bookPath), c->bookPaths.end());
  saveToFile();
}

bool CollectionStore::isBookInCollection(const std::string& collectionId, const std::string& bookPath) const {
  const Collection* c = getCollection(collectionId);
  if (!c) return false;
  return std::find(c->bookPaths.begin(), c->bookPaths.end(), bookPath) != c->bookPaths.end();
}

void CollectionStore::saveToFile() const {
  Storage.mkdir("/.crosspoint");
  JsonDocument doc;
  JsonArray arr = doc["collections"].to<JsonArray>();
  for (const auto& c : _collections) {
    JsonObject obj = arr.add<JsonObject>();
    obj["id"] = c.id;
    obj["name"] = c.name;
    JsonArray books = obj["books"].to<JsonArray>();
    for (const auto& path : c.bookPaths) {
      books.add(path);
    }
  }
  String json;
  serializeJson(doc, json);
  Storage.writeFile(COLLECTIONS_JSON, json);
}

void CollectionStore::loadFromFile() {
  _collections.clear();
  const String json = Storage.readFile(COLLECTIONS_JSON);
  if (json.isEmpty()) return;

  JsonDocument doc;
  if (deserializeJson(doc, json)) {
    LOG_ERR("COLL", "Failed to parse collections.json");
    return;
  }

  for (JsonObject obj : doc["collections"].as<JsonArray>()) {
    Collection c;
    c.id = obj["id"] | std::string("");
    c.name = obj["name"] | std::string("");
    if (c.id.empty() || c.name.empty()) continue;
    for (const char* path : obj["books"].as<JsonArray>()) {
      if (path) c.bookPaths.emplace_back(path);
    }
    _collections.push_back(std::move(c));
  }
  LOG_DBG("COLL", "Loaded %d collections", static_cast<int>(_collections.size()));
}
