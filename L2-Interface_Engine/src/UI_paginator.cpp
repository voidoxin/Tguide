/*
 *  tguide — UI_paginator.cpp
 *  paginated list renderer implementation
 *
 *  written by voidoxin
 */

#include "../includes/UI_paginator.h"
#include "../includes/UI_colors.h"
#include <iostream>
#include <algorithm>

using namespace std;

// ==================== CONSTRUCTOR ====================

Paginator::Paginator(const vector<string>& items, bool selectable)
    : m_items(items)
    , m_selectable(selectable)
    , m_page(0)
    , m_totalPages(0)
{
    // at least one page even when items is empty
    int count = static_cast<int>(m_items.size());
    m_totalPages = (count == 0) ? 1 : (count + PAGE_SIZE - 1) / PAGE_SIZE;
}

// ==================== PAGE BOUNDS ====================

// 0-based index of the first item on the current page
int Paginator::pageStart() const {
    return m_page * PAGE_SIZE;
}

// 0-based index one past the last item on the current page
int Paginator::pageEnd() const {
    int end = pageStart() + PAGE_SIZE;
    return min(end, static_cast<int>(m_items.size()));
}

// ==================== STATE ====================

// 1-based current page number
int Paginator::currentPage() const {
    return m_page + 1;
}

int Paginator::totalPages() const {
    return m_totalPages;
}

bool Paginator::hasNext() const {
    return m_page < m_totalPages - 1;
}

bool Paginator::hasPrev() const {
    return m_page > 0;
}

int Paginator::totalItems() const {
    return static_cast<int>(m_items.size());
}

// ==================== NAVIGATION ====================

// advance one page — returns false if already on last page
bool Paginator::nextPage() {
    if (!hasNext()) return false;
    ++m_page;
    return true;
}

// go back one page — returns false if already on first page
bool Paginator::prevPage() {
    if (!hasPrev()) return false;
    --m_page;
    return true;
}

// ==================== RENDER ====================

// clear screen, print banner + breadcrumb + items for current page + page indicator
void Paginator::render(const string& breadcrumb) const {
    UI::clearScreen();
    UI::printBanner();
    UI::printBreadcrumb(breadcrumb);

    int start = pageStart();
    int end   = pageEnd();

    for (int i = start; i < end; ++i) {
        if (m_selectable) {
            // 1-based index into full list — N never resets per page
            cout << "  ["  << (i + 1) << "]  "
                 << (colorsEnabled() ? Color::DIM   : "")
                 << m_items[i]
                 << (colorsEnabled() ? Color::RESET : "")
                 << "\n";
        } else {
            cout << "       "
                 << (colorsEnabled() ? Color::DIM   : "")
                 << m_items[i]
                 << (colorsEnabled() ? Color::RESET : "")
                 << "\n";
        }
    }

    cout << "\n";
    UI::printDivider();

    // page indicator — hide prev/next hints when navigation is not available
    cout << (colorsEnabled() ? Color::DIM   : "")
         << "  [page " << currentPage() << "/" << m_totalPages << "]";
    if (hasPrev()) cout << "  prev";
    if (hasNext()) cout << "  next";
    cout << "  back\n"
         << (colorsEnabled() ? Color::RESET : "")
         << "\n";
}

// ==================== SELECT ====================

// resolve raw user input to a 0-based index into the full items vector
// number input is 1-based; name input matched against first word of each item
int Paginator::select(const string& input) const {
    string norm = normalize(input);

    // 1. number match — 1-based into full list
    int n = toNumber(norm);
    if (n >= 0) {
        if (n >= 1 && n <= totalItems()) return n - 1;
        return -1;
    }

    // 2/3. name and prefix match — extract first word of each item as label
    vector<string> labels;
    labels.reserve(m_items.size());
    for (const auto& item : m_items) {
        size_t space = item.find(' ');
        labels.push_back(space == string::npos ? item : item.substr(0, space));
    }
    return matchOption(input, labels);
}