#include "Theme.h"

#define LOAD_COLOUR(name, fallback) \
    if (obj->hasProperty(#name)) \
        Theme::name = juce::Colour::fromString(obj->getProperty(#name).toString()); \
    else \
        Theme::name = fallback;

void Theme::loadFromFile(const juce::File& file)
{
    Theme::ensureDefaults();

    if (!file.existsAsFile())
        return;

    auto parsed = juce::JSON::parse(file);
    if (!parsed.isObject()) return;
    auto* obj = parsed.getDynamicObject();
    if (obj == nullptr) return;

    LOAD_COLOUR(bgMain, juce::Colour(0xFF1A1A2E))
    LOAD_COLOUR(bgPanel, juce::Colour(0xFF13294B))
    LOAD_COLOUR(bgButton, juce::Colour(0xFF1A2A4A))
    LOAD_COLOUR(bgButtonHover, juce::Colour(0x881A2A4A))
    LOAD_COLOUR(bgDialog, juce::Colour(0xFF2A2A4A))
    LOAD_COLOUR(bgOverlay, juce::Colour(0xAA000000))

    LOAD_COLOUR(textPrimary, juce::Colour(0xFFE0EDFF))
    LOAD_COLOUR(textHint, juce::Colour(0xFF808080))
    LOAD_COLOUR(textOnButton, juce::Colour(0xFFFFFFFF))
    LOAD_COLOUR(textActiveLine, juce::Colour(0xFFF0C040))
    LOAD_COLOUR(textPause, juce::Colour(0xFFFFCC00))
    LOAD_COLOUR(textError, juce::Colour(0xCCFF0000))
    LOAD_COLOUR(textStatusBar, juce::Colour(0x8CFFFFFF))

    LOAD_COLOUR(accentBorder, juce::Colour(0xFF4444AA))
    LOAD_COLOUR(accentClose, juce::Colour(0xFF6666CC))

    LOAD_COLOUR(sliderTrack, juce::Colour(0xFF4A6FA5))
    LOAD_COLOUR(sliderThumb, juce::Colour(0xFFA0C4FF))

    LOAD_COLOUR(countdown, juce::Colour(0xFF44FF44))

    LOAD_COLOUR(iconPrimary, juce::Colour(0xFFE0EDFF))
    LOAD_COLOUR(iconHover, juce::Colour(0xFFF0C040))

    LOAD_COLOUR(editorBg, juce::Colour(0xFF13294B))
    LOAD_COLOUR(editorText, juce::Colour(0xFFE0EDFF))
    LOAD_COLOUR(editorCaret, juce::Colour(0xFFE0EDFF))
    LOAD_COLOUR(editorHighlight, juce::Colour(0xFF3A5A8A))
    LOAD_COLOUR(editorHighlightedText, juce::Colour(0xFFFFFFFF))
    LOAD_COLOUR(editorOutline, juce::Colour(0x00000000))

    if (obj->hasProperty("baseFontSize"))
        baseFontSize = (float)(double)obj->getProperty("baseFontSize");
    if (obj->hasProperty("tooltipSize"))
        tooltipSize = (float)(double)obj->getProperty("tooltipSize");
    LOAD_COLOUR(tooltipBg,      juce::Colour(0xFF1A1A2E))
    LOAD_COLOUR(tooltipText,    juce::Colour(0xFFFFCC00))
    LOAD_COLOUR(tooltipOutline, juce::Colour(0xFF4A6FA5))
}

void Theme::ensureDefaults()
{
    bgMain = juce::Colour(0xFF1A1A2E);
    bgPanel = juce::Colour(0xFF13294B);
    bgButton = juce::Colour(0xFF1A2A4A);
    bgButtonHover = juce::Colour(0x881A2A4A);
    bgDialog = juce::Colour(0xFF2A2A4A);
    bgOverlay = juce::Colour(0xAA000000);

    textPrimary = juce::Colour(0xFFE0EDFF);
    textHint = juce::Colour(0xFF808080);
    textOnButton = juce::Colour(0xFFFFFFFF);
    textActiveLine = juce::Colour(0xFFF0C040);
    textPause = juce::Colour(0xFFFFCC00);
    textError = juce::Colour(0xCCFF0000);
    textStatusBar = juce::Colour(0x8CFFFFFF);

    accentBorder = juce::Colour(0xFF4444AA);
    accentClose = juce::Colour(0xFF6666CC);

    sliderTrack = juce::Colour(0xFF4A6FA5);
    sliderThumb = juce::Colour(0xFFA0C4FF);

    countdown = juce::Colour(0xFF44FF44);

    iconPrimary = juce::Colour(0xFFE0EDFF);
    iconHover = juce::Colour(0xFFF0C040);

    editorBg = juce::Colour(0xFF13294B);
    editorText = juce::Colour(0xFFE0EDFF);
    editorCaret = juce::Colour(0xFFE0EDFF);
    editorHighlight = juce::Colour(0xFF3A5A8A);
    editorHighlightedText = juce::Colour(0xFFFFFFFF);
    editorOutline = juce::Colour(0x00000000);

    baseFontSize      = 12.0f;
    tooltipSize       = 1.0f;
    tooltipBg         = juce::Colour(0xFF1A1A2E);
    tooltipText       = juce::Colour(0xFFFFCC00);
    tooltipOutline    = juce::Colour(0xFF4A6FA5);
}
