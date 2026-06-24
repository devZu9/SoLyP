#pragma once

#include <JuceHeader.h>

struct TextLine
{
    int sectionId = 0;
    juce::String sectionName;
    juce::String text;
};

struct DisplayLine
{
    int sectionId = 0;
    juce::String sectionName;
    juce::String text;
    int parts = 1;
};

struct Song
{
    juce::Array<TextLine> textSong;
    juce::StringArray legends;        // indexed by sectionId
    juce::String fileTitle;
    int preLinesOnPause = -1;

    mutable juce::Array<DisplayLine> displayLines;
    mutable float lastBuildWidth = 0;
    mutable float lastBuildFontSize = 0;
    int nextLineIndex = 0;

    void rebuildDisplayLines(float maxWidth, float fontSize) const;

    static Song fromJson(const juce::String& jsonText);
    static Song fromText(const juce::String& text);
    juce::String toJson() const;

    bool isValid() const { return !textSong.isEmpty(); }
    juce::String validationError;
};
