#pragma once

#include <string>
#include <set>
#include <vector>

namespace OwnCAD {
namespace UI {

/**
 * @brief Manages the set of selected entities
 */
class SelectionManager {
public:
    SelectionManager();

    // Selection operations
    void select(const std::string& handle);
    void deselect(const std::string& handle);
    void toggle(const std::string& handle);
    void clear();

    // Query state
    bool isSelected(const std::string& handle) const;
    size_t selectedCount() const;
    std::vector<std::string> selectedHandles() const;
    bool isEmpty() const;

private:
    std::set<std::string> selectedHandles_;
};

} // namespace UI
} // namespace OwnCAD
