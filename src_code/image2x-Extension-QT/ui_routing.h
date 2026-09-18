/*
    Copyright (C) 2026 AIPEAC

    This file is part of Waifu2x-Extension-GUI Reconstructed.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ADDITIONAL PERMISSION under GNU AGPL version 3 section 7:
    As a special exception, the copyright holder of this file gives you
    permission to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell this file without restriction, as though it were licensed
    under a permissive license. This additional permission applies only to
    this specific file and files explicitly marked with this notice.
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
