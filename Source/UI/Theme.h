#pragma once

#include <JuceHeader.h>

namespace Theme
{
    // background
    inline const auto bgMain        = juce::Colour(0xFF1A1A2E);
    inline const auto bgPanel       = juce::Colour(0xFF13294B);
    inline const auto bgButton      = juce::Colour(0xFF1A2A4A);
    inline const auto bgButtonHover = juce::Colour(0x221A2A4A);
    inline const auto bgDialog      = juce::Colour(0xFF2A2A4A);
    inline const auto bgOverlay     = juce::Colour(0xAA000000);

    // text
    inline const auto textPrimary   = juce::Colour(0xFFE0EDFF);
    inline const auto textHint      = juce::Colours::grey;
    inline const auto textOnButton  = juce::Colours::white;
    inline const auto textActiveLine = juce::Colour(0xFFF0C040);
    inline const auto textPause     = juce::Colour(0xFFFFCC00);
    inline const auto textError     = juce::Colour(0xCCFF0000);

    // status bar
    inline const auto textStatusBar = juce::Colours::white.withAlpha(0.3f);

    // dialogs
    inline const auto accentBorder  = juce::Colour(0xFF4444AA);
    inline const auto accentClose   = juce::Colour(0xFF6666CC);

    // sliders
    inline const auto sliderTrack   = juce::Colour(0xFF4A6FA5);
    inline const auto sliderThumb   = juce::Colour(0xFFA0C4FF);

    // countdown
    inline const auto countdown     = juce::Colour(0xFF44FF44);

    // icons
    inline const auto iconPrimary   = juce::Colour(0xFFE0EDFF);
    inline const auto iconHover     = juce::Colour(0xFFF0C040);

    // text editor
    inline const auto editorBg      = bgPanel;
    inline const auto editorText    = textPrimary;
    inline const auto editorCaret   = textPrimary;
    inline const auto editorHighlight = bgButton;
    inline const auto editorHighlightedText = textOnButton;
    inline const auto editorOutline  = juce::Colours::transparentBlack;
}
