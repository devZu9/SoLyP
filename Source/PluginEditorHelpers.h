#pragma once

#include <JuceHeader.h>
#include "Data/Song.h"

juce::String getDefaultSongDir();
juce::String songToText(const Song& song);
