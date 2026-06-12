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
    int preLinesOnPause = -1;

    static Song fromJson(const juce::String& jsonText);
    static Song fromText(const juce::String& text);
    juce::String toJson() const;

    bool isValid() const { return !sections.isEmpty(); }
    juce::String validationError;
};
