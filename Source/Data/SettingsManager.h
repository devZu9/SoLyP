#pragma once

#include <JuceHeader.h>

namespace SettingsManager
{
    // display
    inline int visibleLines = 6;
    inline float fontSize = 80.0f;
    inline int preLinesOnPause = 1;
    inline juce::String pauseText;
    inline int longLineBehavior = 0; // 0 = wrap, 1 = shrink

    // language & theme
    inline juce::String language = "ru";
    inline juce::String themeName = "dark";

    // MIDI
    inline int octaveSystem = 0;
    inline int landmarkOctave = 1;
    inline int triggerOctave = 2;
    inline int midiChannel = 0;
    inline int triggerPlay = 0;
    inline int triggerPause = 2;
    inline int triggerNextSection = 4;
    inline int triggerHybrid = 5;
    inline int triggerCountdown3 = 7;
    inline int triggerCountdown5 = 9;

    // sync
    inline bool manualBpmEnabled = false;
    inline float manualBpmValue = 120.0f;
    inline int timeSignature = 4;
    inline float linesPerBar = 1.0f;

    // startup guard
    inline bool noStartSave = true;

    // cursor
    inline bool cursorEnabled = false;
    inline int cursorShape = 0;       // 0=triangle, 1=square
    inline int cursorSize = 32;       // 8–64 px
    inline bool cursorRotation = false;
    inline int cursorRotDir = 0;      // 0=CCW, 1=CW
    inline int cursorRotSpeed = 4;    // 1–20
    inline juce::String cursorColor = "7861FE";

    // window
    inline bool alwaysOnTop = true;
    inline int windowWidth = 0;
    inline int windowHeight = 0;
    inline int windowX = 0;
    inline int windowY = 0;

    juce::File getFile();
    void load();
    void save();
}
