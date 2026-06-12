#include "LeftPanel.h"

LeftPanel::LeftPanel()
{
    editButton.setVisible(false);
    editButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF1A2A4A));
    editButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    editButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFE0EDFF));
    addAndMakeVisible(editButton);

    loadButton.setVisible(false);
    loadButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF1A2A4A));
    loadButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    loadButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xFFE0EDFF));
    addAndMakeVisible(loadButton);

    repaint();
}

juce::Rectangle<int> LeftPanel::getIconRect() const
{
    return juce::Rectangle<int>(0, 6, iconSize, iconSize);
}

void LeftPanel::paint(juce::Graphics& g)
{
    auto iconArea = getIconRect();

    if (hovered)
    {
        g.setColour(juce::Colour(0xFF13294B));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 18.0f);
    }
    else
    {
        g.setColour(juce::Colour(0xFF13294B));
        g.fillRoundedRectangle(iconArea.toFloat(), 10.0f);

        // plus icon: horizontal + vertical bars
        g.setColour(juce::Colour(0xFFE0EDFF));
        int barW = 14;
        int barH = 2;
        int cx = iconArea.getCentreX();
        int cy = iconArea.getCentreY();
        auto hBar = juce::Rectangle<int>(cx - barW / 2, cy - barH / 2, barW, barH);
        g.fillRoundedRectangle(hBar.toFloat(), 1.0f);
        auto vBar = juce::Rectangle<int>(cx - barH / 2, cy - barW / 2, barH, barW);
        g.fillRoundedRectangle(vBar.toFloat(), 1.0f);
    }
}

void LeftPanel::resized()
{
    if (!hovered) return;

    auto inner = getLocalBounds().reduced(10, 12);
    inner.removeFromLeft(iconSize + gap);

    int btnW = (inner.getWidth() - 8) / 2;

    editButton.setBounds(inner.removeFromLeft(btnW));
    inner.removeFromLeft(8);
    loadButton.setBounds(inner.removeFromLeft(btnW));
}

void LeftPanel::mouseEnter(const juce::MouseEvent& e)
{
    if (getIconRect().contains(e.getPosition()))
        setHovered(true);
}

void LeftPanel::mouseExit(const juce::MouseEvent&)
{
    // don't close if cursor is still over this panel or its children (buttons)
    if (isMouseOver(true))
        return;
    setHovered(false);
}

void LeftPanel::mouseMove(const juce::MouseEvent& e)
{
    if (!hovered && getIconRect().contains(e.getPosition()))
        setHovered(true);
}

void LeftPanel::setHovered(bool h)
{
    if (hovered == h) return;
    hovered = h;
    editButton.setVisible(h);
    loadButton.setVisible(h);
    resized();
    repaint();
}
