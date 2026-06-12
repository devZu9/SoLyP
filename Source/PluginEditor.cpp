#include "PluginEditor.h"

namespace
{
    juce::String getDefaultSongDir()
    {
        return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
            .getChildFile("SoLyP").getChildFile("songs").getFullPathName();
    }

    void ensureSongsDir()
    {
        juce::File(getDefaultSongDir()).createDirectory();
    }

    juce::String songToText(const Song& song)
    {
        juce::String result;
        for (auto& s : song.sections)
        {
            result += "[" + s.name + "]\n";
            for (auto& l : s.lines)
                result += l + "\n";
            result += "\n";
        }
        return result.trimEnd();
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
    textEditor->setVisible(false);
    addAndMakeVisible(textEditor.get());

    saveButton.setVisible(false);
    backButton.setVisible(false);
    editModeLoadButton.setVisible(false);
    addAndMakeVisible(saveButton);
    addAndMakeVisible(backButton);
    addAndMakeVisible(editModeLoadButton);
    saveButton.addListener(this);
    backButton.addListener(this);
    editModeLoadButton.addListener(this);

    // save dialog
    filenameField = std::make_unique<juce::TextEditor>();
    filenameField->setFont(juce::Font(juce::FontOptions(16.0f)));
    filenameField->setTextToShowWhenEmpty("filename (e.g. my_song)", juce::Colours::grey);
    filenameField->setVisible(false);
    addAndMakeVisible(filenameField.get());

    songnameField = std::make_unique<juce::TextEditor>();
    songnameField->setFont(juce::Font(juce::FontOptions(16.0f)));
    songnameField->setTextToShowWhenEmpty("song title (optional)", juce::Colours::grey);
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

    // controls panel (always there, manages its own hover internally)
    controlsPanel = std::make_unique<ControlsPanel>();
    controlsPanel->linesSlider.addListener(this);
    controlsPanel->fontSizeSlider.addListener(this);
    controlsPanel->setBounds(getWidth() - ControlsPanel::compWidth - 8, 8,
                             ControlsPanel::compWidth, ControlsPanel::compHeight);
    addAndMakeVisible(controlsPanel.get());

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
    // timer is only needed for countdown animation
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
    repaint();
}

// ── paint ───────────────────────────────────────────────────────────────────

void SoLyPAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1A1A2E));

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

void SoLyPAudioProcessorEditor::paintLyrics(juce::Graphics& g)
{
    const auto& song = processor.getCurrentSong();
    if (song.sections.isEmpty())
    {
        g.setColour(juce::Colours::grey);
        g.setFont(juce::FontOptions(24.0f));
        g.drawText("Load a song to begin\n(click Edit in top-left)",
                   getLocalBounds(), juce::Justification::centred);
        return;
    }

    const auto& section = song.sections[processor.getCurrentSectionIndex()];
    float lineHeight = fontSize * 1.4f;
    auto bounds = getLocalBounds().reduced(40, 20);
    int maxLines = juce::jmax(1, static_cast<int>((bounds.getHeight() - 20) / lineHeight));
    int linesToShow = juce::jmin(visibleLines, maxLines, static_cast<int>(section.lines.size()));

    if (linesToShow <= 0)
    {
        linesToShow = juce::jmin(visibleLines, static_cast<int>(section.lines.size()));
        if (linesToShow <= 0) return;
    }

    int startLine = processor.getCurrentLineIndex();
    if (startLine >= static_cast<int>(section.lines.size())) startLine = 0;
    if (startLine + linesToShow > static_cast<int>(section.lines.size()))
        linesToShow = static_cast<int>(section.lines.size() - startLine);

    float totalHeight = linesToShow * lineHeight;
    float y = bounds.getY() + (bounds.getHeight() - totalHeight) / 2.0f;

    for (int i = 0; i < linesToShow; ++i)
    {
        int idx = startLine + i;
        if (idx >= static_cast<int>(section.lines.size())) break;

        g.setColour(i == 0 ? juce::Colour(0xFFF0C040) : juce::Colours::white);
        g.setFont(juce::FontOptions(fontSize));
        auto lineBounds = bounds.withY(static_cast<int>(y)).withHeight(static_cast<int>(lineHeight));
        g.drawText(section.lines[idx], lineBounds, juce::Justification::centred, true);
        y += lineHeight;
    }

    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.setFont(14.0f);
    juce::String info = "Section: " + section.name
        + "  |  Bar: " + juce::String(processor.getCurrentBar())
        + "  |  " + (processor.getTransportState() == SoLyPAudioProcessor::TransportState::Playing ? "PLAYING" : "PAUSED");
    g.drawText(info, getLocalBounds().reduced(10, 5), juce::Justification::bottomLeft);
}

void SoLyPAudioProcessorEditor::paintCountdown(juce::Graphics& g)
{
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(120.0f));
    juce::String digit = juce::String(processor.getCountdownValue());
    if (digit == "0")
    {
        g.setColour(juce::Colour(0xFF44FF44));
        digit = "GO!";
    }
    auto bounds = getLocalBounds();
    g.drawText(digit, bounds.removeFromTop(bounds.getHeight() / 2), juce::Justification::centred);

    const auto& song = processor.getCurrentSong();
    if (!song.sections.isEmpty())
    {
        int nextIdx = processor.getCurrentSectionIndex();
        if (nextIdx < static_cast<int>(song.sections.size()))
        {
            g.setFont(juce::FontOptions(36.0f));
            g.setColour(juce::Colour(0xFF88CCFF));
            g.drawText(song.sections[nextIdx].lines[0], bounds, juce::Justification::centred);
        }
    }
}

void SoLyPAudioProcessorEditor::paintPauseOverlay(juce::Graphics& g)
{
    g.setColour(juce::Colour(0x88000000));
    g.fillRect(getLocalBounds());
    g.setColour(juce::Colour(0xFFFFCC00));
    g.setFont(juce::FontOptions(28.0f));
    g.drawText("(waiting, not singing)", getLocalBounds(), juce::Justification::centred);

    const auto& song = processor.getCurrentSong();
    if (!song.sections.isEmpty() && processor.getPreLinesOnPause() > 0)
    {
        const auto& section = song.sections[processor.getCurrentSectionIndex()];
        int nextLine = processor.getCurrentLineIndex() + 1;
        g.setFont(juce::FontOptions(24.0f));
        g.setColour(juce::Colour(0x88AAAAAA));
        auto bounds = getLocalBounds().removeFromBottom(getHeight() / 3);
        auto y = static_cast<float>(bounds.getY() + 20);
        for (int i = 0; i < processor.getPreLinesOnPause(); ++i)
        {
            int idx = nextLine + i;
            if (idx >= static_cast<int>(section.lines.size())) break;
            g.drawText(section.lines[idx], bounds.withY(static_cast<int>(y)).withHeight(30),
                       juce::Justification::centred);
            y += 34;
        }
    }
}

void SoLyPAudioProcessorEditor::paintSaveDialog(juce::Graphics& g)
{
    // dim background
    g.setColour(juce::Colour(0xAA000000));
    g.fillRect(getLocalBounds());

    // dialog box
    auto dialogW = juce::jmin(400, getWidth() - 100);
    auto dialogH = 180;
    auto dialogArea = juce::Rectangle<int>(
        (getWidth() - dialogW) / 2,
        (getHeight() - dialogH) / 2,
        dialogW,
        dialogH);

    g.setColour(juce::Colour(0xFF2A2A4A));
    g.fillRoundedRectangle(dialogArea.toFloat(), 10.0f);
    g.setColour(juce::Colour(0xFF4444AA));
    g.drawRoundedRectangle(dialogArea.toFloat(), 10.0f, 2.0f);

    // × close button
    auto closeArea = dialogArea.removeFromTop(32).removeFromRight(32);
    g.setColour(juce::Colour(0xFF6666CC));
    g.drawText("×", closeArea, juce::Justification::centred);

    // inner area
    auto inner = dialogArea.reduced(14);
    int labelH = 18;
    int fieldH = 26;
    int gap = 6;

    // filename label
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(13.0f));
    g.drawText("filename", inner.removeFromTop(labelH), juce::Justification::centredLeft);
    inner.removeFromTop(4);
    inner.removeFromTop(fieldH); // skip field area (drawn by child component)
    inner.removeFromTop(gap);

    // song title label
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(13.0f));
    g.drawText("song title (optional)", inner.removeFromTop(labelH), juce::Justification::centredLeft);
    inner.removeFromTop(4);
    inner.removeFromTop(fieldH); // skip field area
    inner.removeFromTop(gap + 4);

    // Save button area hint
    g.setColour(juce::Colour(0xFF4444AA));
    g.drawRoundedRectangle(inner.removeFromLeft(80).reduced(2).toFloat(), 4.0f, 1.0f);
}

void SoLyPAudioProcessorEditor::paintError(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xCCFF0000));
    g.setFont(18.0f);
    g.drawText(lastError, getLocalBounds().removeFromBottom(60), juce::Justification::centred);
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
            inner.removeFromTop(32); // close button row
            int labelH = 18;
            int fieldH = 26;
            int gap = 6;

            // filename label space
            inner.removeFromTop(labelH);
            inner.removeFromTop(4);
            filenameField->setBounds(inner.removeFromTop(fieldH));
            inner.removeFromTop(gap);

            // song title label space
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

    // force paint so icons appear immediately
    repaint();

    // left panel
    if (leftPanel != nullptr)
    {
        leftPanel->setBounds(8, 8, LeftPanel::compWidth, LeftPanel::compHeight);
    }

    if (controlsPanel != nullptr)
        controlsPanel->setBounds(getWidth() - ControlsPanel::compWidth - 8, 8,
                                 ControlsPanel::compWidth, ControlsPanel::compHeight);
    repaint();
}

void SoLyPAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    if (saveDialogVisible)
    {
        // check close button hit
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

// ── save ────────────────────────────────────────────────────────────────────

void SoLyPAudioProcessorEditor::showSaveDialog()
{
    lastError = {};
    auto text = textEditor->getText();
    if (text.trim().isEmpty())
    {
        lastError = "Text is empty!";
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

    filenameField->setText(lastFilename, false);
    songnameField->setText(lastSongTitle, false);

    // hide editor widgets, show dialog widgets
    textEditor->setVisible(false);
    saveButton.setVisible(false);
    backButton.setVisible(false);
    editModeLoadButton.setVisible(false);

    saveDialogVisible = true;
    filenameField->setVisible(true);
    songnameField->setVisible(true);
    confirmSaveBtn.setVisible(true);
    cancelSaveBtn.setVisible(true);

    resized();
    filenameField->grabKeyboardFocus();
    filenameField->selectAll();
}

void SoLyPAudioProcessorEditor::hideSaveDialog()
{
    saveDialogVisible = false;
    filenameField->setVisible(false);
    songnameField->setVisible(false);
    confirmSaveBtn.setVisible(false);
    cancelSaveBtn.setVisible(false);

    if (editMode)
    {
        textEditor->setVisible(true);
        saveButton.setVisible(true);
        backButton.setVisible(true);
        editModeLoadButton.setVisible(true);
    }

    resized();
    grabKeyboardFocus();
}

void SoLyPAudioProcessorEditor::doSave()
{
    auto text = textEditor->getText();
    Song song = Song::fromText(text);
    if (!song.validationError.isEmpty())
    {
        lastError = song.validationError;
        repaint();
        return;
    }

    auto name = filenameField->getText().trim();
    if (name.isEmpty()) name = "untitled";
    if (!name.endsWith(".json")) name += ".json";
    lastFilename = name;

    auto title = songnameField->getText().trim();
    lastSongTitle = title;

    auto jsonText = song.toJson();

    if (title.isNotEmpty())
    {
        auto parsed = juce::JSON::parse(jsonText);
        if (auto* obj = parsed.getDynamicObject())
        {
            obj->setProperty("title", title);
            jsonText = juce::JSON::toString(parsed, false);
        }
    }

    auto dir = juce::File(getDefaultSongDir());
    dir.createDirectory();
    auto file = dir.getChildFile(name);

    if (file.existsAsFile())
    {
        juce::NativeMessageBox::showOkCancelBox(
            juce::MessageBoxIconType::WarningIcon,
            "Overwrite?",
            "File \"" + name + "\" already exists.\nOverwrite?",
            this,
            juce::ModalCallbackFunction::create([this, file, jsonText, song](int result) mutable
            {
                if (result != 1) return;
                file.replaceWithText(jsonText, false, false, "\n");
                processor.loadSong(song);
                exitEditMode();
            }));
        return;
    }

    file.replaceWithText(jsonText, false, false, "\n");
    processor.loadSong(song);
    exitEditMode();
}

// ── load ────────────────────────────────────────────────────────────────────

void SoLyPAudioProcessorEditor::loadSongFromFile()
{
    auto dir = juce::File(getDefaultSongDir());
    dir.createDirectory();

    auto chooser = std::make_shared<juce::FileChooser>("Load song JSON", dir, "*.json");
    chooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser&)
        {
            auto file = chooser->getResult();
            if (!file.existsAsFile()) return;
            auto jsonText = file.loadFileAsString();
            Song song = Song::fromJson(jsonText);
            if (!song.validationError.isEmpty())
            {
                lastError = song.validationError;
                repaint();
                return;
            }
            processor.loadSong(song);
            if (editMode) exitEditMode();
        });
}
