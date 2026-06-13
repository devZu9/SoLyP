#pragma once

#include <JuceHeader.h>

struct Settings
{
    int visibleLines = 6;
    float fontSize = 80.0f;
};

namespace SettingsManager
{
    juce::File getFile();
    Settings load();
    void save(const Settings& s);
}
