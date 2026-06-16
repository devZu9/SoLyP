#pragma once

#include <JuceHeader.h>

struct Section
{
    juce::String name;
    juce::StringArray lines;
};

struct LegendEntry
{
    juce::String section;
    juce::String note;
};

struct Song
{
    juce::Array<Section> sections;
    juce::Array<LegendEntry> legends;
    juce::String fileTitle;
    int preLinesOnPause = -1;

    // pre-processed display lines (split long lines by words, mutable for lazy build)
    mutable juce::StringArray displayLines;
    mutable juce::Array<int> lineToSection;
    mutable juce::Array<int> lineToLine;
    mutable float lastBuildWidth = 0;
    mutable float lastBuildFontSize = 0;

    void rebuildDisplayLines(float maxWidth, float fontSize) const;

    static Song fromJson(const juce::String& jsonText);
    static Song fromText(const juce::String& text);
    juce::String toJson() const;

    bool isValid() const { return !sections.isEmpty(); }
    juce::String validationError;
};
