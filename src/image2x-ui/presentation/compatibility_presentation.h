/*
    Copyright (C) 2026 AIPEAC

    This file is part of Waifu2x-Extension-GUI Reconstructed.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

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
