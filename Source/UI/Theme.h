#pragma once

#include <JuceHeader.h>

namespace Theme
{
    inline juce::Colour bgMain;
    inline juce::Colour bgPanel;
    inline juce::Colour bgButton;
    inline juce::Colour bgButtonHover;
    inline juce::Colour bgDialog;
    inline juce::Colour bgOverlay;

    inline juce::Colour textPrimary;
    inline juce::Colour textHint;
    inline juce::Colour textOnButton;
    inline juce::Colour textActiveLine;
    inline juce::Colour textPause;
    inline juce::Colour textError;
    inline juce::Colour textStatusBar;

    inline juce::Colour accentBorder;
    inline juce::Colour accentClose;

    inline juce::Colour sliderTrack;
    inline juce::Colour sliderThumb;

    inline juce::Colour countdown;

    inline juce::Colour iconPrimary;
    inline juce::Colour iconHover;

    inline juce::Colour editorBg;
    inline juce::Colour editorText;
    inline juce::Colour editorCaret;
    inline juce::Colour editorHighlight;
    inline juce::Colour editorHighlightedText;
    inline juce::Colour editorOutline;

    inline float baseFontSize = 12.0f;
    inline float tooltipSize = 1.0f;
    inline juce::Colour tooltipBg;
    inline juce::Colour tooltipText;
    inline juce::Colour tooltipOutline;

    void loadFromFile(const juce::File& file);
    void ensureDefaults();
}
