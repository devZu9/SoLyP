#pragma once

#include <JuceHeader.h>

struct Settings
{
    // display
    int visibleLines = 6;
    float fontSize = 80.0f;
    int preLinesOnPause = 1;
    juce::String pauseText = "";
    int longLineBehavior = 0; // 0 = wrap, 1 = shrink

    // language & theme
    juce::String language = "ru";
    juce::String themeName = "dark";

    // MIDI
    int octaveSystem = 0; // 0 = yamaha, 1 = roland, 2 = c3, 3 = c4
    int landmarkOctave = 1;
    int triggerOctave = 2;
    int midiChannel = 0;  // 0 = Omni, 1-16 = specific

    // trigger note indices within triggerOctave (0-11 = C to B)
    int triggerPlay = 0;        // C
    int triggerPause = 2;       // D
    int triggerNextSection = 4; // E
    int triggerHybrid = 5;      // F
    int triggerCountdown3 = 7;  // G
    int triggerCountdown5 = 9;  // A

    // sync
    bool manualBpmEnabled = false;
    float manualBpmValue = 120.0f;

    // window
    int windowWidth = 0;
    int windowHeight = 0;
    int windowX = 0;
    int windowY = 0;
};

namespace SettingsManager
{
    juce::File getFile();
    Settings load();
    void save(const Settings& s);
}
