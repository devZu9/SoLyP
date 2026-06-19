#include "PluginEditor.h"
#include "PluginEditorHelpers.h"
#include "UI/Theme.h"
#include "Data/SettingsManager.h"
#include "Data/I18n.h"

// ── shared helpers ─────────────────────────────────────────────────────────

juce::String getDefaultSongDir()
{
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("SoLyP").getChildFile("songs").getFullPathName();
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

// ── paint helpers ───────────────────────────────────────────────────────────

void SoLyPAudioProcessorEditor::paintLyrics(juce::Graphics& g)
{
    const auto& song = processor.getCurrentSong();
    if (song.sections.isEmpty())
    {
        g.setColour(Theme::textHint);
        g.setFont(juce::FontOptions(24.0f));
        g.drawText(I18n::get("empty.title") + "\n" + I18n::get("empty.hint"),
                   getLocalBounds(), juce::Justification::centred);
        return;
    }

    const auto& section = song.sections[processor.getCurrentSectionIndex()];
    float lineHeight = SettingsManager::fontSize * 1.4f;
    auto bounds = getLocalBounds().reduced(40, 20);
    int maxLines = juce::jmax(1, static_cast<int>((bounds.getHeight() - 20) / lineHeight));

    bool wrap = (SettingsManager::longLineBehavior == 0);

    // use pre-processed display lines (word-wrapped), or original lines with ellipsis
    if (wrap)
    {
        // ensure display lines are up to date with current width and font size
        float availW = (float)bounds.getWidth();
        if (song.displayLines.isEmpty() || song.lastBuildWidth != availW || song.lastBuildFontSize != SettingsManager::fontSize)
            processor.getCurrentSong().rebuildDisplayLines(availW, SettingsManager::fontSize);

        // find start index in displayLines for current section/line
        int dlIdx = 0;
        int currentSection = processor.getCurrentSectionIndex();
        int currentLine = processor.getCurrentLineIndex();
        for (int si = 0; si < song.lineToSection.size() && si < song.lineToLine.size(); ++si)
        {
            if (song.lineToSection[si] == currentSection && song.lineToLine[si] == currentLine)
            {
                dlIdx = si;
                break;
            }
            if (si == song.lineToSection.size() - 1 || si == song.lineToLine.size() - 1)
                dlIdx = 0;
        }

        int linesToShow = juce::jmin(SettingsManager::visibleLines, maxLines, song.displayLines.size() - dlIdx);
        if (linesToShow <= 0) return;

        float totalHeight = linesToShow * lineHeight;
        float y = bounds.getY() + (bounds.getHeight() - totalHeight) / 2.0f;

        for (int i = 0; i < linesToShow; ++i)
        {
            int idx = dlIdx + i;
            if (idx >= song.displayLines.size()) break;

            g.setColour(i == 0 ? Theme::textActiveLine : Theme::textOnButton);
            g.setFont(juce::FontOptions(SettingsManager::fontSize));
            auto lineBounds = bounds.withY(static_cast<int>(y)).withHeight(static_cast<int>(lineHeight));
            g.drawText(song.displayLines[idx], lineBounds, juce::Justification::centred, false);
            y += lineHeight;
        }
    }
    else
    {
        int linesToShow = juce::jmin(SettingsManager::visibleLines, maxLines, static_cast<int>(section.lines.size()));
        if (linesToShow <= 0)
        {
            linesToShow = juce::jmin(SettingsManager::visibleLines, static_cast<int>(section.lines.size()));
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

            g.setColour(i == 0 ? Theme::textActiveLine : Theme::textOnButton);
            g.setFont(juce::FontOptions(SettingsManager::fontSize));
            auto lineBounds = bounds.withY(static_cast<int>(y)).withHeight(static_cast<int>(lineHeight));
            g.drawText(section.lines[idx], lineBounds, juce::Justification::centred, true);
            y += lineHeight;
        }
    }

    // status bar
    g.setColour(Theme::textStatusBar);
    g.setFont(14.0f);

    const auto& currentSong = processor.getCurrentSong();
    juce::String leftInfo;
    if (currentSong.fileTitle.isNotEmpty())
        leftInfo = currentSong.fileTitle + "  |  ";

    leftInfo += I18n::get("status.section") + " " + section.name
        + "  |  " + I18n::get("status.bar") + " " + juce::String(processor.getCurrentBar())
        + "  |  " + (processor.getTransportState() == SoLyPAudioProcessor::TransportState::Playing ? I18n::get("status.playing") : I18n::get("status.paused"));

    auto note = processor.getLastMidiNote();
    if (note >= 0)
    {
        auto elapsed = juce::Time::getMillisecondCounterHiRes() - processor.getLastMidiNoteTime();
        if (elapsed < 2000.0)
        {
            static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
            int octave = (note / 12) - 1;
            leftInfo += "  |  " + juce::String(noteNames[note % 12]) + juce::String(octave) + " (" + juce::String(note) + ")";
        }
    }

    g.drawText(leftInfo, getLocalBounds().reduced(10, 5), juce::Justification::bottomLeft);

    // version — bottom-right
    g.drawText("v" + juce::String(SOLYP_VERSION), getLocalBounds().reduced(10, 5), juce::Justification::bottomRight);
}

void SoLyPAudioProcessorEditor::paintPauseOverlay(juce::Graphics& g)
{
    g.setColour(Theme::bgOverlay);
    g.fillRect(getLocalBounds());
    g.setColour(Theme::textPause);
    g.setFont(juce::FontOptions(28.0f));
    g.drawText(I18n::get("pause.text"), getLocalBounds(), juce::Justification::centred);

    const auto& song = processor.getCurrentSong();
    if (!song.sections.isEmpty() && processor.getPreLinesOnPause() > 0)
    {
        const auto& section = song.sections[processor.getCurrentSectionIndex()];
        int nextLine = processor.getCurrentLineIndex() + 1;
        g.setFont(juce::FontOptions(24.0f));
        g.setColour(Theme::textPause);
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

void SoLyPAudioProcessorEditor::paintError(juce::Graphics& g)
{
    g.setColour(Theme::textError);
    g.setFont(18.0f);
    g.drawText(lastError, getLocalBounds().removeFromBottom(60), juce::Justification::centred);
}

// ── save ────────────────────────────────────────────────────────────────────

void SoLyPAudioProcessorEditor::doSave(const juce::String& name, const juce::String& title)
{
    auto text = textEditor->getText();
    Song song = Song::fromText(text);
    if (!song.validationError.isEmpty())
    {
        lastError = song.validationError;
        repaint();
        return;
    }

    auto filename = name;
    if (filename.isEmpty()) filename = "untitled";
    if (!filename.endsWith(".json")) filename += ".json";
    lastFilename = filename;

    lastSongTitle = title;
    song.fileTitle = title;

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
    auto file = dir.getChildFile(filename);

    if (file.existsAsFile())
    {
        juce::NativeMessageBox::showOkCancelBox(
            juce::MessageBoxIconType::WarningIcon,
            I18n::get("overwrite.title"),
            I18n::get("overwrite.message").replace("{name}", filename),
            this,
            juce::ModalCallbackFunction::create([this, file, jsonText, song](int result) mutable
            {
                if (result != 1) return;
                file.replaceWithText(jsonText, false, false, "\n");
                processor.loadSong(song);
                textModified = false;
                pendingChanges = false;
                exitEditMode();
            }));
        return;
    }

    file.replaceWithText(jsonText, false, false, "\n");
    processor.loadSong(song);
    textModified = false;
    pendingChanges = false;
    exitEditMode();
}

// ── load ────────────────────────────────────────────────────────────────────

void SoLyPAudioProcessorEditor::loadSongFromFile()
{
    auto dir = juce::File(getDefaultSongDir());
    dir.createDirectory();

    auto chooser = std::make_shared<juce::FileChooser>(I18n::get("load.title"), dir, "*.json");
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

            if (textModified)
            {
                auto* alert = new juce::AlertWindow(
                    I18n::get("confirm.unsaved"),
                    I18n::get("confirm.loadFile"),
                    juce::AlertWindow::QuestionIcon);
                alert->setColour(juce::AlertWindow::backgroundColourId, Theme::bgPanel);
                alert->setColour(juce::AlertWindow::textColourId, Theme::textPrimary);
                alert->setColour(juce::AlertWindow::outlineColourId, Theme::accentBorder);
                alert->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
                alert->addButton(juce::String::fromUTF8("Отмена"), 0,
                    juce::KeyPress(juce::KeyPress::escapeKey));
                alert->enterModalState(true,
                    juce::ModalCallbackFunction::create(
                        [this, alert, song, file](int r) mutable {
                            if (r != 1) { delete alert; return; }
                            delete alert;
                            applySongLoad(song, file);
                        }));
            }
            else
            {
                applySongLoad(song, file);
            }
        });
}

void SoLyPAudioProcessorEditor::applySongLoad(Song& song, const juce::File& file)
{
    if (song.fileTitle.isEmpty())
        song.fileTitle = file.getFileNameWithoutExtension();
    processor.loadSong(song);
    pendingChanges = false;
    if (editMode)
        textEditor->setText(songToText(song), juce::dontSendNotification);
}
