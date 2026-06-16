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

    juce::TextButton editButton{ "" };
    juce::TextButton loadButton{ "" };
    juce::DrawableButton settingsBtn{ "", juce::DrawableButton::ImageFitted };

    static constexpr int iconSize = 36;
    static constexpr int panelWidth = 160;
    static constexpr int settingsSize = 28;
    static constexpr int panelHeight = 66;
    static constexpr int compWidth = panelWidth + 8 + settingsSize;
    static constexpr int compHeight = panelHeight;

    void setHovered(bool h);
    void refreshText();
    int getRequiredWidth() const;

private:
    juce::Rectangle<int> getIconRect() const;

    bool hovered = false;
    std::unique_ptr<juce::Drawable> iconDrawable;
};
