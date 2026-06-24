#pragma once

#include <JuceHeader.h>
#include "Data/Song.h"

juce::String getDefaultSongDir();
juce::String songToText(const Song& song);
int calcFittingLines(int height, float fontSize, const Song& song);
int findSectionStart(const Song& song, int sectionId);
