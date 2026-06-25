#pragma once

#include <JuceHeader.h>

class ControlsPanel : public juce::Component
{
public:
    ControlsPanel();

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseEnter(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;

    juce::Slider linesSlider;
    juce::Slider fontSizeSlider;
    juce::Slider gapSlider;

    static constexpr int panelWidth = 260;
    static constexpr int iconSize = 36;
    static constexpr int gap = 8;
    static constexpr int panelHeight = 140;
    static constexpr int compWidth = panelWidth + gap + iconSize;
    static constexpr int compHeight = panelHeight;

private:
    juce::Rectangle<int> getIconRect() const;
    void setHovered(bool h);

    bool hovered = false;
    std::unique_ptr<juce::Drawable> iconDrawable;
};
