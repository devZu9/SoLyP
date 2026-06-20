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

struct DisplayLine
{
    juce::String text;
    int sectionIndex;
    int parts; // how many visual lines this original line was split into
};

struct Song
{
    juce::Array<Section> sections;
    juce::Array<LegendEntry> legends;
    juce::String fileTitle;
    int preLinesOnPause = -1;

    // pre-processed display lines (split long lines by words)
    mutable juce::Array<DisplayLine> displayLines;
    mutable float lastBuildWidth = 0;
    mutable float lastBuildFontSize = 0;

    void rebuildDisplayLines(float maxWidth, float fontSize, int longLineBehavior) const;

    static Song fromJson(const juce::String& jsonText);
    static Song fromText(const juce::String& text);
    juce::String toJson() const;

    bool isValid() const { return !sections.isEmpty(); }
    juce::String validationError;
};
