#pragma once

#include <JuceHeader.h>

class LeftPanel : public juce::Component
{
public:
    LeftPanel();

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseEnter(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;

    juce::TextButton editButton{ "Edit" };
    juce::TextButton loadButton{ "Load" };

    static constexpr int iconSize = 36;
    static constexpr int panelWidth = 160;
    static constexpr int gap = 8;
    static constexpr int panelHeight = 66;
    static constexpr int compWidth = iconSize + gap + panelWidth;
    static constexpr int compHeight = panelHeight;

    void setHovered(bool h);

private:
    juce::Rectangle<int> getIconRect() const;

    bool hovered = false;
};
