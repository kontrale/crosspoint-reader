#pragma once

#include <string>

#include "../Activity.h"
#include "CollectionStore.h"
#include "util/ButtonNavigator.h"

/**
 * CollectionListActivity shows all saved collections and lets the user:
 *   - Navigate into a collection to view/manage its books
 *   - Create a new collection (button 3 → keyboard)
 *   - Delete a collection (long-press Confirm → confirmation dialog)
 */
class CollectionListActivity final : public Activity {
 public:
  explicit CollectionListActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("CollectionList", renderer, mappedInput) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  void openSelected();
  void promptNewCollection();
  void deleteSelected();

  ButtonNavigator buttonNavigator;
  int selectorIndex = 0;
};
