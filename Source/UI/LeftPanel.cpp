#include "LeftPanel.h"
#include "icons/Icons.h"
#include "Theme.h"
#include "../Data/I18n.h"

LeftPanel::LeftPanel()
{
    // panel icon (squares-four)
    auto panelSvg = juce::String(Icons::squaresFour).replace("#000000", "#" + Theme::iconPrimary.toDisplayString(false));
    auto panelXml = juce::XmlDocument::parse(panelSvg);
    if (panelXml != nullptr)
        iconDrawable = juce::Drawable::createFromSVG(*panelXml);

    auto setupIconBtn = [&](juce::DrawableButton& btn, const char* svgData, const juce::String& tooltip) {
        auto svg = juce::String(svgData).replace("#000000", "#" + Theme::iconPrimary.toDisplayString(false));
        auto hoverSvg = juce::String(svgData).replace("#000000", "#" + Theme::iconHover.toDisplayString(false));
        auto xml = juce::XmlDocument::parse(svg);
        auto hoverXml = juce::XmlDocument::parse(hoverSvg);
        if (xml && hoverXml) {
            auto normal = juce::Drawable::createFromSVG(*xml);
            auto hover = juce::Drawable::createFromSVG(*hoverXml);
            btn.setImages(normal.get(), hover.get());
        }
        btn.setVisible(false);
        btn.setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
        btn.setColour(juce::DrawableButton::backgroundOnColourId, Theme::bgButtonHover);
        btn.setTooltip(tooltip);
        addAndMakeVisible(btn);
    };

    setupIconBtn(loadButton, Icons::folderOpen, I18n::get("button.load"));
    setupIconBtn(newButton, Icons::filePlus, I18n::get("button.new"));
    setupIconBtn(editButton, Icons::clipboardText, I18n::get("button.edit"));

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
    settingsBtn.setVisible(false);
    settingsBtn.setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
    settingsBtn.setColour(juce::DrawableButton::backgroundOnColourId, Theme::bgButtonHover);
    addAndMakeVisible(settingsBtn);
    settingsBtn.setTooltip(I18n::get("settings.go"));

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

    float wBtn = 28.0f;
    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::row;
    fb.justifyContent = juce::FlexBox::JustifyContent::flexStart;
    fb.items.add(juce::FlexItem(loadButton).withWidth(wBtn));
    fb.items.add(juce::FlexItem(6.0f, 1.0f));
    fb.items.add(juce::FlexItem(newButton).withWidth(wBtn));
    fb.items.add(juce::FlexItem(6.0f, 1.0f));
    fb.items.add(juce::FlexItem(editButton).withWidth(wBtn));
    fb.items.add(juce::FlexItem(6.0f, 1.0f));
    fb.items.add(juce::FlexItem(settingsBtn).withWidth(wBtn));
    fb.performLayout(bounds);
}

int LeftPanel::getRequiredWidth() const
{
    return 10 + 28 + 6 + 28 + 6 + 28 + 6 + 28 + 10 + 8;
}

void LeftPanel::mouseEnter(const juce::MouseEvent& e)
{
    toFront(false);
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
    loadButton.setVisible(h);
    newButton.setVisible(h);
    editButton.setVisible(h);
    settingsBtn.setVisible(h);
    resized();
    repaint();
}

void LeftPanel::refreshText()
{
    loadButton.setTooltip(I18n::get("button.load"));
    newButton.setTooltip(I18n::get("button.new"));
    editButton.setTooltip(I18n::get("button.edit"));
    settingsBtn.setTooltip(I18n::get("settings.go"));
}
