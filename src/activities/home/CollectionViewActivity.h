#pragma once

#include <string>
#include <vector>

#include "../Activity.h"
#include "CollectionStore.h"
#include "RecentBooksStore.h"
#include "util/ButtonNavigator.h"

/**
 * CollectionViewActivity shows and manages books within one collection.
 *
 * Two modes:
 *   VIEW - shows books already in the collection. Confirm=Open, Long Confirm=Remove, Left=switch to ADD.
 *   ADD  - shows recent books not yet in the collection. Confirm=Add, Back/Left=cancel back to VIEW.
 */
class CollectionViewActivity final : public Activity {
 public:
  explicit CollectionViewActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                  const std::string& collectionId)
      : Activity("CollectionView", renderer, mappedInput), collectionId(collectionId) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  enum class Mode { VIEW, ADD };

  void loadAddCandidates();
  void enterAddMode();
  void exitAddMode();

  std::string collectionId;
  Mode mode = Mode::VIEW;

  // VIEW mode state
  int viewSelector = 0;

  // ADD mode state — recent books not already in this collection
  std::vector<RecentBook> addCandidates;
  int addSelector = 0;

  ButtonNavigator buttonNavigator;
};
