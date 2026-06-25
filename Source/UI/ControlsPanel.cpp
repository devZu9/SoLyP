#include "ControlsPanel.h"
#include "icons/Icons.h"
#include "Theme.h"
#include "../Data/I18n.h"
#include "../Data/SettingsManager.h"

ControlsPanel::ControlsPanel()
{
    auto svgStr = juce::String(Icons::textAlignLeft).replace("#000000", "#" + Theme::iconPrimary.toDisplayString(false));
    auto xml = juce::XmlDocument::parse(svgStr);
    if (xml != nullptr)
        iconDrawable = juce::Drawable::createFromSVG(*xml);

    linesSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    linesSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
    linesSlider.setRange(2.0, 20.0, 1.0);
    linesSlider.setValue(6.0);
    linesSlider.setColour(juce::Slider::trackColourId, Theme::sliderTrack);
    linesSlider.setColour(juce::Slider::thumbColourId, Theme::sliderThumb);
    linesSlider.setColour(juce::Slider::textBoxTextColourId, Theme::textPrimary);
    linesSlider.setColour(juce::Slider::textBoxBackgroundColourId, Theme::bgPanel);
    linesSlider.setVisible(false);
    addAndMakeVisible(linesSlider);

    fontSizeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    fontSizeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
    fontSizeSlider.setRange(20.0, 200.0, 1.0);
    fontSizeSlider.setValue(80.0);
    fontSizeSlider.setColour(juce::Slider::trackColourId, Theme::sliderTrack);
    fontSizeSlider.setColour(juce::Slider::thumbColourId, Theme::sliderThumb);
    fontSizeSlider.setColour(juce::Slider::textBoxTextColourId, Theme::textPrimary);
    fontSizeSlider.setColour(juce::Slider::textBoxBackgroundColourId, Theme::bgPanel);
    addAndMakeVisible(fontSizeSlider);

    gapSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    gapSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 20);
    gapSlider.setRange(0.0, 2.0, 0.05);
    gapSlider.setValue((double)SettingsManager::lineSpacing);
    gapSlider.setColour(juce::Slider::trackColourId, Theme::sliderTrack);
    gapSlider.setColour(juce::Slider::thumbColourId, Theme::sliderThumb);
    gapSlider.setColour(juce::Slider::textBoxTextColourId, Theme::textPrimary);
    gapSlider.setColour(juce::Slider::textBoxBackgroundColourId, Theme::bgPanel);
    gapSlider.setVisible(false);
    addAndMakeVisible(gapSlider);

    repaint();
}

juce::Rectangle<int> ControlsPanel::getIconRect() const
{
    return juce::Rectangle<int>(panelWidth + gap, 6, iconSize, iconSize);
}

void ControlsPanel::paint(juce::Graphics& g)
{
    auto iconArea = getIconRect();

    if (hovered)
    {
        g.setColour(Theme::bgPanel);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 18.0f);

        auto inner = getLocalBounds().reduced(20, 18);
        int rowH = 28;
        int labelW = 50;
        int rowGap = 13;

        auto row1 = inner.removeFromTop(rowH);
        g.setColour(Theme::iconPrimary);
        g.setFont(juce::FontOptions(13.0f));
        g.drawText(I18n::get("panel.lines"), row1.removeFromLeft(labelW), juce::Justification::centredLeft);

        inner.removeFromTop(rowGap);

        auto row2 = inner.removeFromTop(rowH);
        g.setColour(Theme::iconPrimary);
        g.setFont(juce::FontOptions(13.0f));
        g.drawText(I18n::get("panel.size"), row2.removeFromLeft(labelW), juce::Justification::centredLeft);

        inner.removeFromTop(rowGap);

        auto row3 = inner.removeFromTop(rowH);
        g.setColour(Theme::iconPrimary);
        g.setFont(juce::FontOptions(13.0f));
        g.drawText(I18n::get("panel.gap"), row3.removeFromLeft(labelW), juce::Justification::centredLeft);
    }
    else
    {
        g.setColour(Theme::bgPanel);
        g.fillRoundedRectangle(iconArea.toFloat(), 10.0f);

        if (iconDrawable != nullptr)
        {
            auto dest = iconArea.reduced(6).toFloat();
            iconDrawable->drawWithin(g, dest, juce::RectanglePlacement::centred, 1.0f);
        }
    }
}

void ControlsPanel::resized()
{
    if (!hovered) return;

    auto inner = getLocalBounds().reduced(20, 18);
    int rowH = 28;
    int labelW = 50;
    int rowGap = 13;

    int sliderX = inner.getX() + labelW;
    int sliderW = inner.getWidth() - labelW;

    auto row1 = inner.removeFromTop(rowH);
    linesSlider.setBounds(sliderX, row1.getY(), sliderW, rowH);

    inner.removeFromTop(rowGap);

    auto row2 = inner.removeFromTop(rowH);
    fontSizeSlider.setBounds(sliderX, row2.getY(), sliderW, rowH);

    inner.removeFromTop(rowGap);

    auto row3 = inner.removeFromTop(rowH);
    gapSlider.setBounds(sliderX, row3.getY(), sliderW, rowH);
}

void ControlsPanel::mouseEnter(const juce::MouseEvent& e)
{
    toFront(false);
    if (getIconRect().contains(e.getPosition()))
        setHovered(true);
}

void ControlsPanel::mouseExit(const juce::MouseEvent&)
{
    // don't close while dragging a slider, or if cursor is still over children
    if (linesSlider.isMouseOverOrDragging() || fontSizeSlider.isMouseOverOrDragging() || gapSlider.isMouseOverOrDragging())
        return;
    if (isMouseOver(true))
        return;
    setHovered(false);
}

void ControlsPanel::mouseMove(const juce::MouseEvent& e)
{
    if (!hovered && getIconRect().contains(e.getPosition()))
        setHovered(true);
}

void ControlsPanel::setHovered(bool h)
{
    if (hovered == h) return;
    hovered = h;
    linesSlider.setVisible(h);
    fontSizeSlider.setVisible(h);
    gapSlider.setVisible(h);
    resized();
    repaint();
}
