/*
 *  tguide — UI_paginator.h
 *  paginated list renderer — callers pass pre-built string lines
 *
 *  written by voidoxin
 */

#pragma once
#include "UI_input.h"
#include "UI_utils.h"
#include <string>
#include <vector>

// ==================== PAGINATOR ====================

// non-template paginator — callers build vector<string> display lines and pass them in
// handles all page math, rendering, and item selection
class Paginator {
public:
    static constexpr int PAGE_SIZE = 10;

    // items      — pre-built display lines (one per item)
    // selectable — if true, items are numbered and selectable by the user
    Paginator(const std::vector<std::string>& items, bool selectable = true);

    // clear screen and render current page with banner, breadcrumb, items, and nav hint
    void render(const std::string& breadcrumb) const;

    // advance to next page — returns false if already on last page
    bool nextPage();

    // go back to previous page — returns false if already on first page
    bool prevPage();

    // resolve raw user input to a 0-based index into the full items vector, -1 if invalid
    int select(const std::string& input) const;

    // current page number, 1-based
    int  currentPage() const;

    // total number of pages
    int  totalPages()  const;

    // true if there is a page after the current one
    bool hasNext()     const;

    // true if there is a page before the current one
    bool hasPrev()     const;

    // total number of items across all pages
    int  totalItems()  const;

private:
    std::vector<std::string> m_items;
    bool                     m_selectable;
    int                      m_page;        // 0-based internally
    int                      m_totalPages;

    // 0-based index of first item on current page
    int pageStart() const;

    // 0-based index one past the last item on current page
    int pageEnd()   const;
};