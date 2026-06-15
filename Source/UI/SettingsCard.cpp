#include "SettingsCard.h"
#include "Theme.h"

SettingsCard::SettingsCard(const juce::String& titleText)
{
    title.setText(titleText, juce::dontSendNotification);
    title.setColour(juce::Label::textColourId, Theme::textPrimary);
    title.setFont(juce::FontOptions(16.0f));
    addAndMakeVisible(title);
}

void SettingsCard::paint(juce::Graphics& g)
{
    auto b = getLocalBounds();

    g.setColour(Theme::bgPanel);
    g.fillRoundedRectangle(b.toFloat(), 16.0f);

    g.setColour(Theme::accentBorder.withAlpha(0.2f));
    g.drawRoundedRectangle(b.toFloat(), 16.0f, 1.0f);
}

void SettingsCard::resized()
{
    auto b = getLocalBounds().reduced(16);

    title.setBounds(b.removeFromTop(28));
    b.removeFromTop(8);

    for (auto& row : rows)
    {
        if (row.control == nullptr)
        {
            b.removeFromTop(8);
            continue;
        }

        int rowH = 28;
        if (rowH > b.getHeight()) break;

        auto rowArea = b.removeFromTop(rowH);
        if (row.label) row.label->setBounds(rowArea.removeFromLeft(rowArea.getWidth() / 2));
        row.control->setBounds(rowArea);

        b.removeFromTop(12);
    }
}

juce::ComboBox* SettingsCard::addCombo(const juce::String& labelText)
{
    auto& row = rows.emplace_back();
    row.label = std::make_unique<juce::Label>(juce::String(), labelText);
    row.label->setColour(juce::Label::textColourId, Theme::textPrimary);
    row.label->setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(row.label.get());

    auto* cb = new juce::ComboBox();
    cb->setColour(juce::ComboBox::backgroundColourId, Theme::bgPanel);
    cb->setColour(juce::ComboBox::textColourId, Theme::textPrimary);
    cb->setColour(juce::ComboBox::arrowColourId, Theme::textPrimary);
    cb->setColour(juce::ComboBox::buttonColourId, Theme::bgButton);
    cb->setColour(juce::PopupMenu::backgroundColourId, Theme::bgPanel);
    cb->setColour(juce::PopupMenu::textColourId, Theme::textPrimary);
    cb->setColour(juce::PopupMenu::highlightedBackgroundColourId, Theme::bgButton);
    row.control = cb;
    addAndMakeVisible(cb);
    return cb;
}

juce::Slider* SettingsCard::addSlider(const juce::String& labelText, double min, double max, double step)
{
    auto& row = rows.emplace_back();
    row.label = std::make_unique<juce::Label>(juce::String(), labelText);
    row.label->setColour(juce::Label::textColourId, Theme::textPrimary);
    row.label->setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(row.label.get());

    auto* sl = new juce::Slider();
    sl->setSliderStyle(juce::Slider::LinearHorizontal);
    sl->setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    sl->setRange(min, max, step);
    sl->setColour(juce::Slider::trackColourId, Theme::sliderTrack);
    sl->setColour(juce::Slider::thumbColourId, Theme::sliderThumb);
    sl->setColour(juce::Slider::backgroundColourId, Theme::bgPanel);
    sl->setColour(juce::Slider::textBoxBackgroundColourId, Theme::bgPanel);
    sl->setColour(juce::Slider::textBoxTextColourId, Theme::textPrimary);
    sl->setColour(juce::Slider::textBoxOutlineColourId, Theme::bgPanel);
    row.control = sl;
    addAndMakeVisible(sl);
    return sl;
}

juce::ToggleButton* SettingsCard::addToggle(const juce::String& labelText)
{
    auto& row = rows.emplace_back();
    row.label = std::make_unique<juce::Label>(juce::String(), labelText);
    row.label->setColour(juce::Label::textColourId, Theme::textPrimary);
    row.label->setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(row.label.get());

    auto* tg = new juce::ToggleButton();
    tg->setColour(juce::ToggleButton::tickColourId, Theme::textPrimary);
    tg->setColour(juce::ToggleButton::textColourId, Theme::textPrimary);
    row.control = tg;
    addAndMakeVisible(tg);
    return tg;
}

int SettingsCard::getPreferredHeight() const
{
    int h = 16 + 28 + 8; // padding top + title + gap
    for (auto& row : rows)
    {
        if (row.control == nullptr)
            h += 8;
        else
            h += 28 + 12;
    }
    return h + 16; // bottom padding
}
