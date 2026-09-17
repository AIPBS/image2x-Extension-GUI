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

#ifndef COMPATIBILITY_PRESENTATION_H
#define COMPATIBILITY_PRESENTATION_H

struct CompatibilityPairState
{
    bool gpuChecked = false;
    bool cpuChecked = false;
    bool cpuEnabled = true;
};

class CompatibilityPresentation
{
public:
    static CompatibilityPairState pairState(bool gpuResult, bool cpuResult)
    {
        return CompatibilityPairState{gpuResult, cpuResult, !gpuResult};
    }
};

#endif // COMPATIBILITY_PRESENTATION_H
