#include "ui/SelectionManager.h"
#include <algorithm>

namespace OwnCAD {
namespace UI {

SelectionManager::SelectionManager() {}

void SelectionManager::select(const std::string& handle) {
    if (!handle.empty()) {
        selectedHandles_.insert(handle);
    }
}

void SelectionManager::deselect(const std::string& handle) {
    selectedHandles_.erase(handle);
}

void SelectionManager::toggle(const std::string& handle) {
    if (isSelected(handle)) {
        deselect(handle);
    } else {
        select(handle);
    }
}

void SelectionManager::clear() {
    selectedHandles_.clear();
}

bool SelectionManager::isSelected(const std::string& handle) const {
    return selectedHandles_.find(handle) != selectedHandles_.end();
}

size_t SelectionManager::selectedCount() const {
    return selectedHandles_.size();
}

std::vector<std::string> SelectionManager::selectedHandles() const {
    return std::vector<std::string>(selectedHandles_.begin(), selectedHandles_.end());
}

bool SelectionManager::isEmpty() const {
    return selectedHandles_.empty();
}

} // namespace UI
} // namespace OwnCAD
