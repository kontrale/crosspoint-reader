#include "CollectionListActivity.h"

#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>

#include "CollectionStore.h"
#include "MappedInputManager.h"
#include "activities/ActivityManager.h"
#include "activities/util/ConfirmationActivity.h"
#include "activities/util/KeyboardEntryActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"

void CollectionListActivity::onEnter() {
  Activity::onEnter();
  COLLECTIONS.loadFromFile();
  selectorIndex = 0;
  requestUpdate();
}

void CollectionListActivity::onExit() { Activity::onExit(); }

void CollectionListActivity::openSelected() {
  const auto& cols = COLLECTIONS.getCollections();
  if (cols.empty() || selectorIndex >= static_cast<int>(cols.size())) return;
  activityManager.goToCollectionView(cols[selectorIndex].id);
}

void CollectionListActivity::promptNewCollection() {
  startActivityForResult(std::make_unique<KeyboardEntryActivity>(renderer, mappedInput, tr(STR_NEW_COLLECTION), "", 80),
                         [this](const ActivityResult& result) {
                           if (result.isCancelled) return;
                           const auto& kb = std::get<KeyboardResult>(result.data);
                           if (kb.text.empty()) return;
                           COLLECTIONS.createCollection(kb.text);
                           const auto& cols = COLLECTIONS.getCollections();
                           selectorIndex = static_cast<int>(cols.size()) - 1;
                           requestUpdate();
                         });
}

void CollectionListActivity::deleteSelected() {
  const auto& cols = COLLECTIONS.getCollections();
  if (cols.empty() || selectorIndex >= static_cast<int>(cols.size())) return;
  const std::string idToDelete = cols[selectorIndex].id;
  const std::string name = cols[selectorIndex].name;

  startActivityForResult(
      std::make_unique<ConfirmationActivity>(renderer, mappedInput, tr(STR_DELETE) + std::string("?"), name),
      [this, idToDelete](const ActivityResult& result) {
        if (result.isCancelled) return;
        COLLECTIONS.deleteCollection(idToDelete);
        const int count = COLLECTIONS.getCount();
        if (selectorIndex >= count && selectorIndex > 0) selectorIndex = count - 1;
        requestUpdate();
      });
}

void CollectionListActivity::loop() {
  const int count = COLLECTIONS.getCount();
  const int pageItems = UITheme::getInstance().getNumberOfItemsPerPage(renderer, true, false, true, true);

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    onGoHome();
    return;
  }

  const bool longPress = mappedInput.getHeldTime() >= 1000;

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (longPress) {
      deleteSelected();
    } else {
      openSelected();
    }
    return;
  }

  // Button 3 (Left) = New Collection
  if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
    promptNewCollection();
    return;
  }

  buttonNavigator.onNextRelease([this, count] {
    selectorIndex = ButtonNavigator::nextIndex(selectorIndex, count);
    requestUpdate();
  });
  buttonNavigator.onPreviousRelease([this, count] {
    selectorIndex = ButtonNavigator::previousIndex(selectorIndex, count);
    requestUpdate();
  });
  buttonNavigator.onNextContinuous([this, count, pageItems] {
    selectorIndex = ButtonNavigator::nextPageIndex(selectorIndex, count, pageItems);
    requestUpdate();
  });
  buttonNavigator.onPreviousContinuous([this, count, pageItems] {
    selectorIndex = ButtonNavigator::previousPageIndex(selectorIndex, count, pageItems);
    requestUpdate();
  });
}

void CollectionListActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();

  GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, tr(STR_COLLECTIONS), nullptr);

  const int contentTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int contentHeight = pageHeight - contentTop - metrics.buttonHintsHeight - metrics.verticalSpacing;

  const auto& cols = COLLECTIONS.getCollections();
  if (cols.empty()) {
    renderer.drawText(UI_10_FONT_ID, metrics.contentSidePadding, contentTop + 20, tr(STR_NO_COLLECTIONS));
  } else {
    GUI.drawList(
        renderer, Rect{0, contentTop, pageWidth, contentHeight}, static_cast<int>(cols.size()), selectorIndex,
        [&cols](int i) { return cols[i].name; },
        [&cols](int i) { return std::to_string(static_cast<int>(cols[i].bookPaths.size())) + " books"; }, nullptr);
  }

  const auto labels = mappedInput.mapLabels(tr(STR_HOME), tr(STR_SELECT), tr(STR_NEW_COLLECTION), tr(STR_DIR_UP));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}
