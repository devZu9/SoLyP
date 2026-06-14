#include "LeftPanel.h"
#include "icons/Icons.h"
#include "Theme.h"
#include "../Data/I18n.h"

namespace
{
    int calcBtnWidth(int chars)
    {
        static const int cw = []{
            juce::Font f(juce::FontOptions(13.0f));
            juce::GlyphArrangement ga;
            ga.addLineOfText(f, "W", 0.0f, 0.0f);
            return juce::roundToInt(ga.getBoundingBox(0, 1, false).getWidth());
        }();
        return chars * cw + 20;
    }
}

LeftPanel::LeftPanel()
{
    editButton.setButtonText(I18n::get("button.edit"));
    loadButton.setButtonText(I18n::get("button.load"));

    // panel icon (squares-four)
    auto panelSvg = juce::String(Icons::squaresFour).replace("#000000", "#" + Theme::iconPrimary.toDisplayString(false));
    auto panelXml = juce::XmlDocument::parse(panelSvg);
    if (panelXml != nullptr)
        iconDrawable = juce::Drawable::createFromSVG(*panelXml);

    // settings icon (gear — normal + hover)
    auto gearSvg = juce::String(Icons::gear).replace("#000000", "#" + Theme::iconPrimary.toDisplayString(false));
    auto gearHoverSvg = juce::String(Icons::gear).replace("#000000", "#" + Theme::iconHover.toDisplayString(false));
    auto gearXml = juce::XmlDocument::parse(gearSvg);
    auto gearHoverXml = juce::XmlDocument::parse(gearHoverSvg);
    if (gearXml != nullptr && gearHoverXml != nullptr)
    {
        auto gearNormal = juce::Drawable::createFromSVG(*gearXml);
        auto gearHover  = juce::Drawable::createFromSVG(*gearHoverXml);
        settingsBtn.setImages(gearNormal.get(), gearHover.get());
    }

    editButton.setVisible(false);
    editButton.setColour(juce::TextButton::buttonColourId, Theme::bgButton);
    editButton.setColour(juce::TextButton::textColourOnId, Theme::textOnButton);
    editButton.setColour(juce::TextButton::textColourOffId, Theme::iconPrimary);
    addAndMakeVisible(editButton);

    loadButton.setVisible(false);
    loadButton.setColour(juce::TextButton::buttonColourId, Theme::bgButton);
    loadButton.setColour(juce::TextButton::textColourOnId, Theme::textOnButton);
    loadButton.setColour(juce::TextButton::textColourOffId, Theme::iconPrimary);
    addAndMakeVisible(loadButton);

    settingsBtn.setVisible(false);
    settingsBtn.setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
    settingsBtn.setColour(juce::DrawableButton::backgroundOnColourId, Theme::bgButtonHover);
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
        g.setColour(Theme::bgPanel);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 18.0f);
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

void LeftPanel::resized()
{
    if (!hovered) return;

    auto bounds = getLocalBounds().reduced(10, 12).toFloat();

    float wLoad = (float)calcBtnWidth(juce::String(I18n::get("button.load")).length());
    float wEdit = (float)calcBtnWidth(juce::String(I18n::get("button.edit")).length());

    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::row;
    fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    fb.items.add(juce::FlexItem(loadButton).withWidth(wLoad));
    fb.items.add(juce::FlexItem(8.0f, 1.0f));
    fb.items.add(juce::FlexItem(editButton).withWidth(wEdit));
    fb.items.add(juce::FlexItem(6.0f, 1.0f));
    fb.items.add(juce::FlexItem(settingsBtn).withWidth(28.0f));
    fb.performLayout(bounds);
}

int LeftPanel::getRequiredWidth() const
{
    int wLoad = calcBtnWidth(juce::String(I18n::get("button.load")).length());
    int wEdit = calcBtnWidth(juce::String(I18n::get("button.edit")).length());
    return 10 + wLoad + 8 + wEdit + 6 + 28 + 10;
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
