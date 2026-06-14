#include "PluginEditor.h"
#include "PluginEditorHelpers.h"
#include "UI/Theme.h"
#include "Data/SettingsManager.h"

namespace
{
    void ensureSongsDir()
    {
        juce::File(getDefaultSongDir()).createDirectory();
    }
}

SoLyPAudioProcessorEditor::SoLyPAudioProcessorEditor(SoLyPAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    ensureSongsDir();

    setSize(800, 500);
    setResizable(true, false);
    setConstrainer(nullptr);
    setAlwaysOnTop(true);
    setMouseClickGrabsKeyboardFocus(false);
    setWantsKeyboardFocus(true);
    startTimerHz(30);

    // edit mode widgets
    textEditor = std::make_unique<juce::TextEditor>();
    textEditor->setMultiLine(true, true);
    textEditor->setReturnKeyStartsNewLine(true);
    textEditor->setFont(juce::Font(juce::FontOptions(18.0f)));
    textEditor->setScrollbarsShown(true);
    textEditor->setColour(juce::TextEditor::backgroundColourId,      Theme::editorBg);
    textEditor->setColour(juce::TextEditor::textColourId,            Theme::editorText);
    textEditor->setColour(juce::CaretComponent::caretColourId,       Theme::editorCaret);
    textEditor->setColour(juce::TextEditor::highlightColourId,       Theme::editorHighlight);
    textEditor->setColour(juce::TextEditor::highlightedTextColourId, Theme::editorHighlightedText);
    textEditor->setColour(juce::TextEditor::outlineColourId,         Theme::editorOutline);
    textEditor->setVisible(false);
    addAndMakeVisible(textEditor.get());

    auto styleBtn = [](juce::TextButton& btn) {
        btn.setColour(juce::TextButton::buttonColourId, Theme::bgButton);
        btn.setColour(juce::TextButton::textColourOffId, Theme::textPrimary);
        btn.setColour(juce::TextButton::textColourOnId, Theme::textOnButton);
    };

    saveButton.setVisible(false);
    backButton.setVisible(false);
    editModeLoadButton.setVisible(false);
    styleBtn(saveButton);
    styleBtn(backButton);
    styleBtn(editModeLoadButton);
    addAndMakeVisible(saveButton);
    addAndMakeVisible(backButton);
    addAndMakeVisible(editModeLoadButton);
    saveButton.addListener(this);
    backButton.addListener(this);
    editModeLoadButton.addListener(this);

    // save dialog
    filenameField = std::make_unique<juce::TextEditor>();
    filenameField->setFont(juce::Font(juce::FontOptions(16.0f)));
    filenameField->setTextToShowWhenEmpty("filename (e.g. my_song)", Theme::textHint);
    filenameField->setVisible(false);
    addAndMakeVisible(filenameField.get());

    songnameField = std::make_unique<juce::TextEditor>();
    songnameField->setFont(juce::Font(juce::FontOptions(16.0f)));
    songnameField->setTextToShowWhenEmpty("song title (optional)", Theme::textHint);
    songnameField->setVisible(false);
    addAndMakeVisible(songnameField.get());

    confirmSaveBtn.setVisible(false);
    cancelSaveBtn.setVisible(false);
    addAndMakeVisible(confirmSaveBtn);
    addAndMakeVisible(cancelSaveBtn);
    confirmSaveBtn.addListener(this);
    cancelSaveBtn.addListener(this);

    // left panel
    leftPanel = std::make_unique<LeftPanel>();
    leftPanel->editButton.addListener(this);
    leftPanel->loadButton.addListener(this);
    leftPanel->setBounds(8, 8, LeftPanel::compWidth, LeftPanel::compHeight);
    addAndMakeVisible(leftPanel.get());

    // controls panel
    controlsPanel = std::make_unique<ControlsPanel>();
    controlsPanel->linesSlider.addListener(this);
    controlsPanel->fontSizeSlider.addListener(this);
    controlsPanel->setBounds(getWidth() - ControlsPanel::compWidth - 8, 8,
                             ControlsPanel::compWidth, ControlsPanel::compHeight);
    addAndMakeVisible(controlsPanel.get());

    // load saved settings
    auto settings = SettingsManager::load();
    visibleLines = settings.visibleLines;
    fontSize = settings.fontSize;
    controlsPanel->linesSlider.setValue(visibleLines);
    controlsPanel->fontSizeSlider.setValue(fontSize);

    processor.onStateChanged = [this]() { repaint(); };
}

SoLyPAudioProcessorEditor::~SoLyPAudioProcessorEditor()
{
    processor.onStateChanged = nullptr;
}

bool SoLyPAudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
    if (saveDialogVisible && key == juce::KeyPress::escapeKey)
    {
        hideSaveDialog();
        return true;
    }
    return false;
}

void SoLyPAudioProcessorEditor::timerCallback()
{
    if (editMode) return;
    if (processor.getTransportState() == SoLyPAudioProcessor::TransportState::Countdown)
        repaint();
}

void SoLyPAudioProcessorEditor::mouseMove(const juce::MouseEvent& e)
{
    if (editMode) return;
    juce::ignoreUnused(e);
}
void SoLyPAudioProcessorEditor::mouseExit(const juce::MouseEvent&) {}

void SoLyPAudioProcessorEditor::buttonClicked(juce::Button* btn)
{
    if (btn == &saveButton)
        showSaveDialog();
    else if (btn == &backButton)
        exitEditMode();
    else if (btn == &editModeLoadButton)
        loadSongFromFile();
    else if (btn == &leftPanel->editButton)
    {
        leftPanel->setHovered(false);
        enterEditMode();
    }
    else if (btn == &leftPanel->loadButton)
    {
        leftPanel->setHovered(false);
        loadSongFromFile();
    }
    else if (btn == &leftPanel->settingsBtn)
    {
        leftPanel->setHovered(false);
    }
    else if (btn == &confirmSaveBtn)
        doSave();
    else if (btn == &cancelSaveBtn)
        hideSaveDialog();
}

void SoLyPAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    if (controlsPanel != nullptr)
    {
        if (slider == &controlsPanel->linesSlider)
            visibleLines = static_cast<int>(controlsPanel->linesSlider.getValue());
        else if (slider == &controlsPanel->fontSizeSlider)
            fontSize = static_cast<float>(controlsPanel->fontSizeSlider.getValue());
    }

    Settings s;
    s.visibleLines = visibleLines;
    s.fontSize = fontSize;
    SettingsManager::save(s);
    repaint();
}

// ── paint ───────────────────────────────────────────────────────────────────

void SoLyPAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(Theme::bgMain);

    if (editMode)
    {
        if (saveDialogVisible) { paintSaveDialog(g); }
        return;
    }

    auto state = processor.getTransportState();
    if (state == SoLyPAudioProcessor::TransportState::Countdown)
        paintCountdown(g);
    else
        paintLyrics(g);

    if (state == SoLyPAudioProcessor::TransportState::Paused)
        paintPauseOverlay(g);

    if (!lastError.isEmpty())
        paintError(g);
}

// ── resized ─────────────────────────────────────────────────────────────────

void SoLyPAudioProcessorEditor::resized()
{
    if (editMode)
    {
        if (saveDialogVisible)
        {
            auto dialogW = juce::jmin(400, getWidth() - 100);
            auto dialogH = 180;
            auto dialogArea = juce::Rectangle<int>(
                (getWidth() - dialogW) / 2,
                (getHeight() - dialogH) / 2,
                dialogW,
                dialogH);

            auto inner = dialogArea.reduced(14);
            inner.removeFromTop(32);
            int labelH = 18;
            int fieldH = 26;
            int gap = 6;

            inner.removeFromTop(labelH);
            inner.removeFromTop(4);
            filenameField->setBounds(inner.removeFromTop(fieldH));
            inner.removeFromTop(gap);

            inner.removeFromTop(labelH);
            inner.removeFromTop(4);
            songnameField->setBounds(inner.removeFromTop(fieldH));
            inner.removeFromTop(gap + 4);

            confirmSaveBtn.setBounds(inner.removeFromLeft(80).reduced(2));
            cancelSaveBtn.setBounds(inner.removeFromLeft(80).reduced(2));
            return;
        }

        auto area = getLocalBounds().reduced(10);
        auto buttonBar = area.removeFromBottom(40);
        saveButton.setBounds(buttonBar.removeFromLeft(100).reduced(4));
        backButton.setBounds(buttonBar.removeFromLeft(100).reduced(4));
        editModeLoadButton.setBounds(buttonBar.removeFromLeft(100).reduced(4));
        textEditor->setBounds(area);
        return;
    }

    repaint();

    if (leftPanel != nullptr)
        leftPanel->setBounds(8, 8, LeftPanel::compWidth, LeftPanel::compHeight);

    if (controlsPanel != nullptr)
        controlsPanel->setBounds(getWidth() - ControlsPanel::compWidth - 8, 8,
                                 ControlsPanel::compWidth, ControlsPanel::compHeight);
    repaint();
}

void SoLyPAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    if (saveDialogVisible)
    {
        auto dialogW = juce::jmin(400, getWidth() - 100);
        auto dialogH = 180;
        auto dialogArea = juce::Rectangle<int>(
            (getWidth() - dialogW) / 2,
            (getHeight() - dialogH) / 2,
            dialogW,
            dialogH);
        auto closeArea = dialogArea.removeFromTop(32).removeFromRight(32);
        if (closeArea.contains(e.getPosition()))
        {
            hideSaveDialog();
            return;
        }
    }
}

// ── edit mode ───────────────────────────────────────────────────────────────

void SoLyPAudioProcessorEditor::enterEditMode()
{
    editMode = true;
    if (leftPanel != nullptr) leftPanel->setVisible(false);
    if (controlsPanel != nullptr) controlsPanel->setVisible(false);

    const auto& song = processor.getCurrentSong();
    if (song.sections.isEmpty())
        textEditor->setText("[Verse 1]\nType your\nlyrics here\n\n[Chorus]\nAnd here\n");
    else
        textEditor->setText(songToText(song));

    textEditor->setVisible(true);
    saveButton.setVisible(true);
    backButton.setVisible(true);
    editModeLoadButton.setVisible(true);
    resized();
    textEditor->grabKeyboardFocus();
}

void SoLyPAudioProcessorEditor::exitEditMode()
{
    hideSaveDialog();
    editMode = false;
    if (leftPanel != nullptr) leftPanel->setVisible(true);
    if (controlsPanel != nullptr) controlsPanel->setVisible(true);
    textEditor->setVisible(false);
    saveButton.setVisible(false);
    backButton.setVisible(false);
    editModeLoadButton.setVisible(false);
    resized();
    grabKeyboardFocus();
}
