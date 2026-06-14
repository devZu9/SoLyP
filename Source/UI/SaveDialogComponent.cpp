#include "SaveDialogComponent.h"
#include "Theme.h"
#include "../Data/I18n.h"

SaveDialogComponent::SaveDialogComponent(const juce::String& initialFilename,
                                         const juce::String& initialSongTitle,
                                         std::function<void(const juce::String&, const juce::String&)> onSave_,
                                         std::function<void()> onCancel_)
{
    saveBtn.setButtonText(I18n::get("button.save"));
    cancelBtn.setButtonText(I18n::get("button.cancel"));

    saveBtn.setColour(juce::TextButton::buttonColourId, Theme::bgButton);
    saveBtn.setColour(juce::TextButton::textColourOffId, Theme::textPrimary);
    saveBtn.setColour(juce::TextButton::textColourOnId, Theme::textOnButton);
    cancelBtn.setColour(juce::TextButton::buttonColourId, Theme::bgButton);
    cancelBtn.setColour(juce::TextButton::textColourOffId, Theme::textPrimary);
    cancelBtn.setColour(juce::TextButton::textColourOnId, Theme::textOnButton);

    // close button — transparent, over the X area (20x20, centered in top-right)
    closeBtn.setSize(20, 20);
    closeBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    closeBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    closeBtn.onClick = [this] { exitModalState(0); };
    addAndMakeVisible(closeBtn);

    filenameField.setFont(juce::Font(juce::FontOptions(16.0f)));
    filenameField.setText(initialFilename, false);
    filenameField.setColour(juce::TextEditor::backgroundColourId, Theme::bgMain);
    filenameField.setColour(juce::TextEditor::textColourId, Theme::editorText);
    filenameField.setColour(juce::TextEditor::highlightColourId, Theme::editorHighlight);
    filenameField.setColour(juce::TextEditor::highlightedTextColourId, Theme::editorHighlightedText);
    filenameField.setColour(juce::TextEditor::outlineColourId, Theme::bgMain);
    filenameField.addListener(this);

    songnameField.setFont(juce::Font(juce::FontOptions(16.0f)));
    songnameField.setText(initialSongTitle, false);
    songnameField.setColour(juce::TextEditor::backgroundColourId, Theme::bgMain);
    songnameField.setColour(juce::TextEditor::textColourId, Theme::editorText);
    songnameField.setColour(juce::TextEditor::highlightColourId, Theme::editorHighlight);
    songnameField.setColour(juce::TextEditor::highlightedTextColourId, Theme::editorHighlightedText);
    songnameField.setColour(juce::TextEditor::outlineColourId, Theme::bgMain);

    addAndMakeVisible(filenameField);
    addAndMakeVisible(songnameField);
    addAndMakeVisible(saveBtn);
    addAndMakeVisible(cancelBtn);

    saveBtn.onClick = [this, onSave_] {
        auto name = filenameField.getText().trim();
        auto title = songnameField.getText().trim();
        onSave_(name, title);
        exitModalState(0);
    };
    cancelBtn.onClick = [this, onCancel_] {
        onCancel_();
        exitModalState(0);
    };

    setAlwaysOnTop(true);
    filenameField.grabKeyboardFocus();
    filenameField.selectAll();
}

void SaveDialogComponent::activateModal(std::function<void()> onClose)
{
    enterModalState(true, juce::ModalCallbackFunction::create([onClose](int) {
        if (onClose) onClose();
    }));
}

void SaveDialogComponent::textEditorEscapeKeyPressed(juce::TextEditor&)
{
    exitModalState(0);
}

void SaveDialogComponent::paint(juce::Graphics& g)
{
    g.setColour(Theme::bgMain.withAlpha(0.9f));
    g.fillRect(getLocalBounds());

    auto dialogW = juce::jmin(400, getWidth() - 100);
    auto dialogH = 220;
    auto dialogArea = juce::Rectangle<int>(
        (getWidth() - dialogW) / 2,
        (getHeight() - dialogH) / 2,
        dialogW,
        dialogH);

    g.setColour(Theme::editorBg);
    g.fillRoundedRectangle(dialogArea.toFloat(), 10.0f);
    g.setColour(Theme::bgMain);
    g.drawRoundedRectangle(dialogArea.toFloat(), 10.0f, 2.0f);

    // header
    auto headerArea = dialogArea.removeFromTop(32);
    auto closeArea = headerArea.removeFromRight(32);

    // draw small X
    g.setColour(Theme::accentClose);
    int cx = closeArea.getCentreX();
    int cy = closeArea.getCentreY();
    g.drawLine((float)(cx - 4), (float)(cy - 4), (float)(cx + 4), (float)(cy + 4), 2.0f);
    g.drawLine((float)(cx + 4), (float)(cy - 4), (float)(cx - 4), (float)(cy + 4), 2.0f);

    g.setColour(Theme::textOnButton);
    g.setFont(juce::FontOptions(16.0f));
    g.drawText(I18n::get("saveDialog.title"), headerArea.reduced(10, 0), juce::Justification::centredLeft);

    auto inner = dialogArea.reduced(14);
    int labelH = 18;
    int fieldH = 26;
    int gap = 6;

    g.setColour(Theme::textOnButton);
    g.setFont(juce::FontOptions(13.0f));
    g.drawText(I18n::get("saveDialog.filename"), inner.removeFromTop(labelH), juce::Justification::centredLeft);
    inner.removeFromTop(4);
    inner.removeFromTop(fieldH);
    inner.removeFromTop(gap);

    g.setColour(Theme::textOnButton);
    g.setFont(juce::FontOptions(13.0f));
    g.drawText(I18n::get("saveDialog.songTitle"), inner.removeFromTop(labelH), juce::Justification::centredLeft);
    inner.removeFromTop(4);
    inner.removeFromTop(fieldH);
    inner.removeFromTop(gap + 4);

    g.setColour(Theme::accentBorder);
    g.drawRoundedRectangle(inner.removeFromLeft(96).reduced(2, 4).toFloat(), 4.0f, 1.0f);
}

void SaveDialogComponent::resized()
{
    auto dialogW = juce::jmin(400, getWidth() - 100);
    auto dialogH = 220;
    auto dialogArea = juce::Rectangle<int>(
        (getWidth() - dialogW) / 2,
        (getHeight() - dialogH) / 2,
        dialogW,
        dialogH);

    // close button — 20x20 centered in the top-right corner of the dialog
    auto closeArea = dialogArea.removeFromTop(32).removeFromRight(32);
    closeBtn.setBounds(closeArea.getCentreX() - 10, closeArea.getCentreY() - 10, 20, 20);

    auto inner = dialogArea.reduced(14);
    int labelH = 18;
    int fieldH = 26;
    int gap = 6;

    inner.removeFromTop(labelH);
    inner.removeFromTop(4);
    filenameField.setBounds(inner.removeFromTop(fieldH));
    inner.removeFromTop(gap);

    inner.removeFromTop(labelH);
    inner.removeFromTop(4);
    songnameField.setBounds(inner.removeFromTop(fieldH));
    inner.removeFromTop(gap + 4);

    saveBtn.setBounds(inner.removeFromLeft(96).reduced(2, 4));
    inner.removeFromLeft(8);
    cancelBtn.setBounds(inner.removeFromLeft(96).reduced(2, 4));
}

bool SaveDialogComponent::keyPressed(const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        exitModalState(0);
        return true;
    }
    return false;
}
