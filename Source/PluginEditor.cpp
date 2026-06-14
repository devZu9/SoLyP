#include "PluginEditor.h"
#include "PluginEditorHelpers.h"
#include "UI/Theme.h"
#include "UI/SaveDialogComponent.h"
#include "Data/SettingsManager.h"
#include "Data/I18n.h"

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
    // load theme first — all Theme:: colours must be set before any widget is created
    auto themeDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("SoLyP").getChildFile("themes");
    Theme::loadFromFile(themeDir.getChildFile("dark.json"));

    // load settings and language
    auto settings = SettingsManager::load();
    I18n::setLanguage(settings.language);

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

    saveButton.setButtonText(I18n::get("button.save"));
    backButton.setButtonText(I18n::get("button.back"));
    editModeLoadButton.setButtonText(I18n::get("button.load"));

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

    // left panel
    leftPanel = std::make_unique<LeftPanel>();
    leftPanel->editButton.addListener(this);
    leftPanel->loadButton.addListener(this);
    leftPanel->setBounds(8, 8, leftPanel->getRequiredWidth(), LeftPanel::compHeight);
    addAndMakeVisible(leftPanel.get());

    // controls panel
    controlsPanel = std::make_unique<ControlsPanel>();
    controlsPanel->linesSlider.addListener(this);
    controlsPanel->fontSizeSlider.addListener(this);
    controlsPanel->setBounds(getWidth() - ControlsPanel::compWidth - 8, 8,
                             ControlsPanel::compWidth, ControlsPanel::compHeight);
    addAndMakeVisible(controlsPanel.get());

    // load saved settings
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

bool SoLyPAudioProcessorEditor::keyPressed(const juce::KeyPress&)
{
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
        return;

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

void SoLyPAudioProcessorEditor::paintOverChildren(juce::Graphics&)
{
}

// ── resized ─────────────────────────────────────────────────────────────────

void SoLyPAudioProcessorEditor::resized()
{
    if (editMode)
    {
        auto area = getLocalBounds().reduced(10);
        auto buttonBar = area.removeFromTop(40);
        backButton.setBounds(buttonBar.removeFromLeft(100).reduced(4));
        editModeLoadButton.setBounds(buttonBar.removeFromLeft(100).reduced(4));
        saveButton.setBounds(buttonBar.removeFromLeft(100).reduced(4));
        textEditor->setBounds(area);
        return;
    }

    repaint();

    if (leftPanel != nullptr)
        leftPanel->setBounds(8, 8, leftPanel->getRequiredWidth(), LeftPanel::compHeight);

    if (controlsPanel != nullptr)
        controlsPanel->setBounds(getWidth() - ControlsPanel::compWidth - 8, 8,
                                 ControlsPanel::compWidth, ControlsPanel::compHeight);
    repaint();
}

void SoLyPAudioProcessorEditor::mouseDown(const juce::MouseEvent&)
{
}

// ── edit mode ───────────────────────────────────────────────────────────────

void SoLyPAudioProcessorEditor::enterEditMode()
{
    editMode = true;
    if (leftPanel != nullptr) leftPanel->setVisible(false);
    if (controlsPanel != nullptr) controlsPanel->setVisible(false);

    const auto& song = processor.getCurrentSong();
    if (song.sections.isEmpty())
        textEditor->setText(I18n::get("editor.placeholder"));
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

// ── save dialog ─────────────────────────────────────────────────────────────

void SoLyPAudioProcessorEditor::showSaveDialog()
{
    lastError = {};
    auto text = textEditor->getText();
    if (text.trim().isEmpty())
    {
        lastError = I18n::get("error.empty");
        repaint();
        return;
    }

    Song song = Song::fromText(text);
    if (!song.validationError.isEmpty())
    {
        lastError = song.validationError;
        repaint();
        return;
    }

    if (lastFilename.isEmpty() && !song.sections.isEmpty())
        lastFilename = juce::File::createLegalFileName(song.sections[0].name);

    auto* dialog = new SaveDialogComponent(
        lastFilename,
        lastSongTitle,
        [this](const juce::String& name, const juce::String& title) {
            doSave(name, title);
        },
        []() {}
    );
    addAndMakeVisible(dialog);
    dialog->setBounds(getLocalBounds());
    dialog->activateModal([this, dialog] {
        juce::MessageManager::callAsync([this, dialog] {
            removeChildComponent(dialog);
            delete dialog;
        });
    });
}
