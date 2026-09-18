/*
    Copyright (C) 2026 AIPEAC

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Affero General Public License for more details.
*/

#ifndef UI_ROUTING_H
#define UI_ROUTING_H

#include <QTabWidget>
#include <QWidget>

class UiRouting
{
public:
    static int compatibilityTabIndex(const QTabWidget *tabs, const QWidget *page)
    {
        return tabs == nullptr || page == nullptr
            ? -1
            : tabs->indexOf(const_cast<QWidget *>(page));
    }
};

#endif // UI_ROUTING_H
