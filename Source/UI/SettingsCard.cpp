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

    // draw separator lines
    auto content = b.reduced(16);
    content.removeFromTop(28 + 8);

    for (auto& row : rows)
    {
        if (row.isSeparator)
        {
            int sy = content.getY() + 12;
            g.setColour(Theme::accentBorder.withAlpha(0.3f));
            g.drawHorizontalLine(sy, (float)content.getX() + 4, (float)content.getRight() - 4);
            content.removeFromTop(24);
        }
        else if (row.control == nullptr)
        {
            content.removeFromTop(8);
        }
        else
        {
            content.removeFromTop(28);
            content.removeFromTop(12);
        }
    }
}

void SettingsCard::resized()
{
    auto b = getLocalBounds().reduced(16);

    title.setBounds(b.removeFromTop(28));
    b.removeFromTop(8);

    for (auto& row : rows)
    {
        if (row.isSeparator)
        {
            b.removeFromTop(24);
            continue;
        }

        if (row.control == nullptr)
        {
            b.removeFromTop(8);
            continue;
        }

        int rowH = 28;
        if (rowH > b.getHeight()) break;

        auto rowArea = b.removeFromTop(rowH);
        if (row.label) row.label->setBounds(rowArea.removeFromLeft(rowArea.getWidth() / 2));

        // toggle buttons: fit to content, don't stretch
        if (dynamic_cast<juce::ToggleButton*>(row.control) != nullptr)
            row.control->setBounds(rowArea.removeFromLeft(30));
        else
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

void SettingsCard::addSeparator()
{
    auto& row = rows.emplace_back();
    row.isSeparator = true;
}

juce::Label* SettingsCard::getLastLabel()
{
    for (auto it = rows.rbegin(); it != rows.rend(); ++it)
        if (it->label != nullptr)
            return it->label.get();
    return nullptr;
}

int SettingsCard::getPreferredHeight() const
{
    int h = 16 + 28 + 8; // padding top + title + gap
    for (auto& row : rows)
    {
        if (row.isSeparator)
            h += 24;
        else if (row.control == nullptr)
            h += 8;
        else
            h += 28 + 12;
    }
    return h + 16; // bottom padding
}
