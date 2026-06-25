#pragma once

#include <JuceHeader.h>
#include "Data/Song.h"
#include "Data/SettingsManager.h"

// реальная высота строки: fontSize * 0.6 (чистая высота букв) * (1 + зазор)
inline float getRealLineHeight(float fontSize) {
    return fontSize * 0.6f * (1.0f + SettingsManager::lineSpacing);
}

juce::String getDefaultSongDir();
juce::String songToText(const Song& song);
int calcFittingLines(int height, float fontSize, const Song& song);
int findSectionStart(const Song& song, int sectionId);
