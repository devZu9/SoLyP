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
        else if (row.control == nullptr) { content.removeFromTop(8); }
        else { content.removeFromTop(28); content.removeFromTop(12); }
    }
}

void SettingsCard::resized()
{
    auto b = getLocalBounds().reduced(16);
    title.setBounds(b.removeFromTop(28));
    b.removeFromTop(8);

    for (auto& row : rows)
    {
        if (row.isSeparator) { b.removeFromTop(24); continue; }
        if (row.control == nullptr) { b.removeFromTop(8); continue; }

        int rowH = row.rowHeight;
        if (rowH > b.getHeight()) break;
        auto rowArea = b.removeFromTop(rowH);

        if (row.label) {
            if (row.isInfoPair) {
                auto halfW = rowArea.getWidth() / 2;
                float textW = row.label->getFont().getStringWidthFloat(row.label->getText());
                int iconW = 22;
                row.label->setBounds(rowArea.getX(), rowArea.getY(), (int)textW + 4, rowH);
                if (row.control2)
                    row.control2->setBounds(rowArea.getX() + (int)textW + 6, rowArea.getY(), 18, 18);
                rowArea.removeFromLeft((int)halfW);
            } else {
                row.label->setBounds(rowArea.removeFromLeft(rowArea.getWidth() / 2));
            }
        }

        if (row.isPair)
        {
            if (row.isInfoPair) { row.control->setBounds(rowArea.removeFromLeft(30)); }
            else if (row.isInfoPair)
            {
                auto half = rowArea.withWidth(rowArea.getWidth() / 2);
                row.control->setBounds(half.reduced(4, 2));
                if (row.control2)
                    row.control2->setBounds(half.translated((float)rowArea.getWidth() / 2, 0).reduced(4, 2));
            }
            else
            {
                auto half = rowArea.withWidth(rowArea.getWidth() / 2);
                row.control->setBounds(half.reduced(4, 2));
                if (row.control2)
                    row.control2->setBounds(half.translated((float)rowArea.getWidth() / 2, 0).reduced(4, 2));
            }
        }
        else if (dynamic_cast<juce::ToggleButton*>(row.control) != nullptr)
            row.control->setBounds(rowArea.removeFromLeft(30));
        else if (dynamic_cast<juce::TextButton*>(row.control) != nullptr)
            row.control->setBounds(rowArea.removeFromLeft(30).withHeight(28));
        else if (dynamic_cast<juce::TextEditor*>(row.control) != nullptr)
            row.control->setBounds(rowArea.reduced(2, 0));
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
    row.control = tg;
    addAndMakeVisible(tg);
    return tg;
}

juce::DrawableButton* SettingsCard::addInfoIcon()
{
    if (rows.empty()) return nullptr;
    auto& row = rows.back();
    if (row.control == nullptr) return nullptr;
    row.isInfoPair = true;

    auto* info = new juce::DrawableButton("", juce::DrawableButton::ImageFitted);
    info->setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
    info->setColour(juce::DrawableButton::backgroundOnColourId, juce::Colours::transparentBlack);
    row.control2 = info;
    addAndMakeVisible(info);
    return info;
}

void SettingsCard::addRadioPair(const juce::String& labelText, const juce::String& opt1, const juce::String& opt2,
                                juce::ToggleButton*& out1, juce::ToggleButton*& out2)
{
    auto& row = rows.emplace_back();
    row.isPair = true;
    row.label = std::make_unique<juce::Label>(juce::String(), labelText);
    row.label->setColour(juce::Label::textColourId, Theme::textPrimary);
    row.label->setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(row.label.get());

    static int nextRadioGroup = 1;
    auto* b1 = new juce::ToggleButton(opt1);
    auto* b2 = new juce::ToggleButton(opt2);
    b1->setColour(juce::ToggleButton::tickColourId, Theme::textPrimary);
    b1->setRadioGroupId(nextRadioGroup);
    b2->setColour(juce::ToggleButton::tickColourId, Theme::textPrimary);
    b2->setRadioGroupId(nextRadioGroup);
    ++nextRadioGroup;
    row.control = b1;
    row.control2 = b2;
    addAndMakeVisible(b1);
    addAndMakeVisible(b2);
    out1 = b1;
    out2 = b2;
}

void SettingsCard::addImagePair(const juce::String& labelText, const juce::Drawable* img1, const juce::Drawable* img2,
                                juce::DrawableButton*& out1, juce::DrawableButton*& out2)
{
    auto& row = rows.emplace_back();
    row.isPair = true;
    row.label = std::make_unique<juce::Label>(juce::String(), labelText);
    row.label->setColour(juce::Label::textColourId, Theme::textPrimary);
    row.label->setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(row.label.get());

    auto* b1 = new juce::DrawableButton("", juce::DrawableButton::ImageFitted);
    auto* b2 = new juce::DrawableButton("", juce::DrawableButton::ImageFitted);
    b1->setImages(img1, nullptr, nullptr, img1, nullptr, nullptr, img1, nullptr);
    b2->setImages(img2, nullptr, nullptr, img2, nullptr, nullptr, img2, nullptr);
    b1->setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
    b1->setColour(juce::DrawableButton::backgroundOnColourId, Theme::bgButtonHover);
    b2->setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
    b2->setColour(juce::DrawableButton::backgroundOnColourId, Theme::bgButtonHover);
    row.control = b1;
    row.control2 = b2;
    addAndMakeVisible(b1);
    addAndMakeVisible(b2);
    out1 = b1;
    out2 = b2;
}

void SettingsCard::addSeparator()
{
    auto& row = rows.emplace_back();
    row.isSeparator = true;
}

juce::Label* SettingsCard::getLastLabel()
{
    for (auto it = rows.rbegin(); it != rows.rend(); ++it)
        if (it->label != nullptr) return it->label.get();
    return nullptr;
}

int SettingsCard::getPreferredHeight() const
{
    int h = 16 + 28 + 8;
    for (auto& row : rows)
    {
        if (row.isSeparator) h += 24;
        else if (row.control == nullptr) h += 8;
        else h += row.rowHeight + 12;
    }
    return h + 16;
}

// ── EditorBorderLAF ─────────────────────────────────────────────────────────

class EditorBorderLAF : public juce::LookAndFeel_V4
{
    void drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor&) override
    {
        g.setColour(Theme::editorText.withAlpha(0.4f));
        g.drawRoundedRectangle(1.0f, 1.0f, (float)width - 2.0f, (float)height - 2.0f, 2.0f, 1.0f);
    }
};

juce::TextEditor* SettingsCard::addEditor(const juce::String& labelText, int height)
{
    auto& row = rows.emplace_back();
    row.rowHeight = height;
    row.label = std::make_unique<juce::Label>(juce::String(), labelText);
    row.label->setColour(juce::Label::textColourId, Theme::textPrimary);
    row.label->setFont(juce::FontOptions(12.0f));
    row.label->setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(row.label.get());

    auto* ed = new juce::TextEditor();
    editorBorderLAF = std::make_unique<EditorBorderLAF>();
    ed->setLookAndFeel(editorBorderLAF.get());
    ed->setMultiLine(true, false);
    ed->setReturnKeyStartsNewLine(true);
    ed->setFont(juce::Font(juce::FontOptions(14.0f)));
    ed->setScrollbarsShown(true);
    ed->setColour(juce::TextEditor::backgroundColourId, Theme::editorBg);
    ed->setColour(juce::TextEditor::textColourId, Theme::editorText);
    ed->setColour(juce::CaretComponent::caretColourId, Theme::editorCaret);
    ed->setColour(juce::TextEditor::highlightColourId, Theme::editorHighlight);
    ed->setColour(juce::TextEditor::highlightedTextColourId, Theme::editorHighlightedText);
    ed->setColour(juce::TextEditor::outlineColourId, Theme::editorOutline);
    row.control = ed;
    addAndMakeVisible(ed);
    return ed;
}

juce::TextButton* SettingsCard::addButton(const juce::String& labelText)
{
    auto& row = rows.emplace_back();
    row.label = std::make_unique<juce::Label>(juce::String(), labelText);
    row.label->setColour(juce::Label::textColourId, Theme::textPrimary);
    row.label->setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(row.label.get());

    auto* btn = new juce::TextButton();
    btn->setColour(juce::TextButton::buttonColourId, Theme::bgButton);
    row.control = btn;
    addAndMakeVisible(btn);
    return btn;
}
