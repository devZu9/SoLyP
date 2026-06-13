#include "LeftPanel.h"
#include "icons/Icons.h"

LeftPanel::LeftPanel()
{
    // panel icon (squares-four)
    auto panelSvg = juce::String(Icons::squaresFour).replace("#000000", "#E0EDFF");
    auto panelXml = juce::XmlDocument::parse(panelSvg);
    if (panelXml != nullptr)
        iconDrawable = juce::Drawable::createFromSVG(*panelXml);

    // settings icon (gear)
    auto gearSvg = juce::String(Icons::gear).replace("#000000", "#E0EDFF");
    auto gearXml = juce::XmlDocument::parse(gearSvg);
    if (gearXml != nullptr)
    {
        auto gearDrawable = juce::Drawable::createFromSVG(*gearXml);
        settingsBtn.setImages(gearDrawable.get());
    }

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

    settingsBtn.setVisible(false);
    settingsBtn.setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
    settingsBtn.setColour(juce::DrawableButton::backgroundOnColourId, juce::Colour(0x221A2A4A));
    addAndMakeVisible(settingsBtn);

    repaint();
}

juce::Rectangle<int> LeftPanel::getIconRect() const
{
    return juce::Rectangle<int>(4, 6, iconSize, iconSize);
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

        if (iconDrawable != nullptr)
        {
            auto dest = iconArea.reduced(6).toFloat();
            iconDrawable->drawWithin(g, dest, juce::RectanglePlacement::centred, 1.0f);
        }
    }
}

void LeftPanel::resized()
{
    if (!hovered) return;

    auto inner = getLocalBounds().reduced(10, 12);

    loadButton.setBounds(inner.removeFromLeft(66));
    inner.removeFromLeft(8);
    editButton.setBounds(inner.removeFromLeft(66));
    inner.removeFromLeft(8);
    settingsBtn.setBounds(inner.removeFromLeft(28));
}

void LeftPanel::mouseEnter(const juce::MouseEvent& e)
{
    if (getIconRect().contains(e.getPosition()))
        setHovered(true);
}

void LeftPanel::mouseExit(const juce::MouseEvent&)
{
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
    settingsBtn.setVisible(h);
    resized();
    repaint();
}
