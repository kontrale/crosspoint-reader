#include "CollectionViewActivity.h"

#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>

#include "CollectionStore.h"
#include "MappedInputManager.h"
#include "RecentBooksStore.h"
#include "activities/ActivityManager.h"
#include "activities/util/ConfirmationActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"

void CollectionViewActivity::onEnter() {
  Activity::onEnter();
  mode = Mode::VIEW;
  viewSelector = 0;
  requestUpdate();
}

void CollectionViewActivity::onExit() { Activity::onExit(); }

void CollectionViewActivity::loadAddCandidates() {
  addCandidates.clear();
  for (const auto& book : RECENT_BOOKS.getBooks()) {
    if (!Storage.exists(book.path.c_str())) continue;
    if (!COLLECTIONS.isBookInCollection(collectionId, book.path)) {
      addCandidates.push_back(book);
    }
  }
}

void CollectionViewActivity::enterAddMode() {
  loadAddCandidates();
  addSelector = 0;
  mode = Mode::ADD;
  requestUpdate();
}

void CollectionViewActivity::exitAddMode() {
  mode = Mode::VIEW;
  addCandidates.clear();
  requestUpdate();
}

void CollectionViewActivity::loop() {
  if (mode == Mode::VIEW) {
    const CollectionStore::Collection* col = COLLECTIONS.getCollection(collectionId);
    const int count = col ? static_cast<int>(col->bookPaths.size()) : 0;
    const int pageItems = UITheme::getInstance().getNumberOfItemsPerPage(renderer, true, false, true, true);

    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      activityManager.goToCollections();
      return;
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Left)) {
      enterAddMode();
      return;
    }

    const bool longPress = mappedInput.getHeldTime() >= 1000;

    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      if (longPress) {
        // Long press = remove from collection
        if (col && viewSelector < count) {
          const std::string pathToRemove = col->bookPaths[viewSelector];
          const std::string label = pathToRemove.substr(pathToRemove.find_last_of('/') + 1);
          startActivityForResult(
              std::make_unique<ConfirmationActivity>(renderer, mappedInput, tr(STR_DELETE) + std::string("?"), label),
              [this, pathToRemove](const ActivityResult& result) {
                if (result.isCancelled) return;
                COLLECTIONS.removeBook(collectionId, pathToRemove);
                const CollectionStore::Collection* c = COLLECTIONS.getCollection(collectionId);
                const int newCount = c ? static_cast<int>(c->bookPaths.size()) : 0;
                if (viewSelector >= newCount && viewSelector > 0) viewSelector = newCount - 1;
                requestUpdate();
              });
        }
      } else {
        // Short press = open book
        if (col && viewSelector < count) {
          activityManager.goToReader(col->bookPaths[viewSelector]);
        }
      }
      return;
    }

    buttonNavigator.onNextRelease([this, count] {
      viewSelector = ButtonNavigator::nextIndex(viewSelector, count);
      requestUpdate();
    });
    buttonNavigator.onPreviousRelease([this, count] {
      viewSelector = ButtonNavigator::previousIndex(viewSelector, count);
      requestUpdate();
    });
    buttonNavigator.onNextContinuous([this, count, pageItems] {
      viewSelector = ButtonNavigator::nextPageIndex(viewSelector, count, pageItems);
      requestUpdate();
    });
    buttonNavigator.onPreviousContinuous([this, count, pageItems] {
      viewSelector = ButtonNavigator::previousPageIndex(viewSelector, count, pageItems);
      requestUpdate();
    });

  } else {
    // ADD mode
    const int count = static_cast<int>(addCandidates.size());
    const int pageItems = UITheme::getInstance().getNumberOfItemsPerPage(renderer, true, false, true, true);

    if (mappedInput.wasReleased(MappedInputManager::Button::Back) ||
        mappedInput.wasReleased(MappedInputManager::Button::Left)) {
      exitAddMode();
      return;
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      if (addSelector < count) {
        COLLECTIONS.addBook(collectionId, addCandidates[addSelector].path);
        // Remove from candidates and stay in add mode
        addCandidates.erase(addCandidates.begin() + addSelector);
        if (addSelector >= static_cast<int>(addCandidates.size()) && addSelector > 0) addSelector--;
        requestUpdate();
      }
      return;
    }

    buttonNavigator.onNextRelease([this, count] {
      addSelector = ButtonNavigator::nextIndex(addSelector, count);
      requestUpdate();
    });
    buttonNavigator.onPreviousRelease([this, count] {
      addSelector = ButtonNavigator::previousIndex(addSelector, count);
      requestUpdate();
    });
    buttonNavigator.onNextContinuous([this, count, pageItems] {
      addSelector = ButtonNavigator::nextPageIndex(addSelector, count, pageItems);
      requestUpdate();
    });
    buttonNavigator.onPreviousContinuous([this, count, pageItems] {
      addSelector = ButtonNavigator::previousPageIndex(addSelector, count, pageItems);
      requestUpdate();
    });
  }
}

void CollectionViewActivity::render(RenderLock&&) {
  renderer.clearScreen();

  const auto& metrics = UITheme::getInstance().getMetrics();
  const int pageWidth = renderer.getScreenWidth();
  const int pageHeight = renderer.getScreenHeight();
  const int contentTop = metrics.topPadding + metrics.headerHeight + metrics.verticalSpacing;
  const int contentHeight = pageHeight - contentTop - metrics.buttonHintsHeight - metrics.verticalSpacing;

  const CollectionStore::Collection* col = COLLECTIONS.getCollection(collectionId);
  const std::string title = col ? col->name : tr(STR_COLLECTIONS);

  if (mode == Mode::VIEW) {
    GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, title.c_str(), nullptr);

    if (!col || col->bookPaths.empty()) {
      renderer.drawText(UI_10_FONT_ID, metrics.contentSidePadding, contentTop + 20, tr(STR_COLLECTION_EMPTY));
    } else {
      const auto& paths = col->bookPaths;
      GUI.drawList(
          renderer, Rect{0, contentTop, pageWidth, contentHeight}, static_cast<int>(paths.size()), viewSelector,
          [&paths](int i) {
            const std::string& p = paths[i];
            const size_t slash = p.find_last_of('/');
            std::string name = (slash != std::string::npos) ? p.substr(slash + 1) : p;
            // Strip extension
            const size_t dot = name.find_last_of('.');
            if (dot != std::string::npos) name.resize(dot);
            return name;
          },
          nullptr, nullptr);
    }

    const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_OPEN), tr(STR_ADD_BOOK), tr(STR_DIR_UP));
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  } else {
    // ADD mode header
    const std::string header = tr(STR_ADD_BOOK);
    GUI.drawHeader(renderer, Rect{0, metrics.topPadding, pageWidth, metrics.headerHeight}, header.c_str(), nullptr);

    if (addCandidates.empty()) {
      renderer.drawText(UI_10_FONT_ID, metrics.contentSidePadding, contentTop + 20, tr(STR_NO_RECENT_BOOKS));
    } else {
      GUI.drawList(
          renderer, Rect{0, contentTop, pageWidth, contentHeight}, static_cast<int>(addCandidates.size()), addSelector,
          [this](int i) { return addCandidates[i].title.empty() ? addCandidates[i].path : addCandidates[i].title; },
          [this](int i) { return addCandidates[i].author; }, nullptr);
    }

    const auto labels = mappedInput.mapLabels(tr(STR_BACK), tr(STR_SELECT), "", tr(STR_DIR_UP));
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  }

  renderer.displayBuffer();
}
