#include "../../Display.h"
#include "../../Data/SettingsManager.h"
#include "../../Data/I18n.h"
#include "../Theme.h"
#include "../SettingsComponent.h"
#include "../SaveDialogComponent.h"
#include "../icons/Icons.h"

void SoLyPAudioProcessorEditor::componentMovedOrResized(
    juce::Component& comp, bool wasMoved, bool wasResized)
{
    if (wasMoved || wasResized)
    {
        auto pos = comp.getScreenPosition();
        SettingsManager::windowWidth = getWidth();
        SettingsManager::windowHeight = getHeight();
        SettingsManager::windowX = pos.x;
        SettingsManager::windowY = pos.y;
        SettingsManager::save();
    }
}

bool SoLyPAudioProcessorEditor::keyPressed(const juce::KeyPress&) { return false; }

void SoLyPAudioProcessorEditor::mouseMove(const juce::MouseEvent& e)
{
    if (editMode) return;
    mousePos = e.getPosition();
}

void SoLyPAudioProcessorEditor::mouseExit(const juce::MouseEvent&) {}
void SoLyPAudioProcessorEditor::mouseDown(const juce::MouseEvent&) {}

void SoLyPAudioProcessorEditor::buttonClicked(juce::Button* btn)
{
    if (btn == &saveButton) { showSaveDialog(); return; }
    if (btn == &backButton) { if (settingsMode) exitSettingsMode(); else exitEditMode(); return; }
    if (btn == &editModeLoadButton) { loadSongFromFile(); return; }
    if (btn == &editModeNewButton) { enterEditMode(true); return; }
    if (btn == &leftPanel->editButton) { leftPanel->setHovered(false); enterEditMode(); return; }
    if (btn == &leftPanel->loadButton) { leftPanel->setHovered(false); loadSongFromFile(); return; }
    if (btn == &leftPanel->newButton) { leftPanel->setHovered(false); enterEditMode(true); return; }
    if (btn == &leftPanel->settingsBtn) { leftPanel->setHovered(false); enterSettingsMode(); return; }
    if (btn == &settingsEditBtn) { enterSettingsMode(); return; }
}

void SoLyPAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    if (!controlsPanel) return;

    auto bounds = getLocalBounds().reduced(40, 20);
    float availH = (float)bounds.getHeight();
    float lhMult = SettingsManager::lineSpacing;

    if (slider == &controlsPanel->linesSlider)
    {
        int newLines = static_cast<int>(controlsPanel->linesSlider.getValue());
        if (newLines > 0)
        {
            SettingsManager::visibleLines = newLines;
            SettingsManager::fontSize = availH / ((float)newLines * lhMult);
            controlsPanel->fontSizeSlider.setValue((double)SettingsManager::fontSize, juce::dontSendNotification);
        }
    }
    else if (slider == &controlsPanel->fontSizeSlider)
    {
        float newSize = static_cast<float>(controlsPanel->fontSizeSlider.getValue());
        if (newSize > 0.0f)
        {
            SettingsManager::fontSize = newSize;
            int newLines = (int)(availH / (newSize * lhMult));
            if (newLines < 1) newLines = 1;
            SettingsManager::visibleLines = newLines;
            controlsPanel->linesSlider.setValue((double)newLines, juce::dontSendNotification);
        }
    }
    else if (slider == &controlsPanel->gapSlider)
    {
        SettingsManager::lineSpacing = (float)controlsPanel->gapSlider.getValue();
        int newLines = (int)(availH / (SettingsManager::fontSize * SettingsManager::lineSpacing));
        if (newLines < 1) newLines = 1;
        SettingsManager::visibleLines = newLines;
        controlsPanel->linesSlider.setValue((double)newLines, juce::dontSendNotification);
    }

    updateLineCount();
    rebuildDisplayLines();
    const auto& song = processor.getCurrentSong();
    if (!song.textSong.isEmpty())
        initSlots();
    SettingsManager::save();
    repaint();
}

void SoLyPAudioProcessorEditor::enterEditMode(bool blank)
{
    editMode = true;
    if (leftPanel) leftPanel->setVisible(false);
    if (controlsPanel) controlsPanel->setVisible(false);
    const auto& song = processor.getCurrentSong();
    textEditor->setVisible(true);
    saveButton.setVisible(true);
    backButton.setVisible(true);
    editModeLoadButton.setVisible(true);
    editModeNewButton.setVisible(true);
    settingsEditBtn.setVisible(true);
    resized();
    if (blank || song.textSong.isEmpty())
        textEditor->setText(I18n::get("editor.placeholder"), juce::dontSendNotification);
    else if (!pendingChanges)
        textEditor->setText(songToText(song), juce::dontSendNotification);
    textEditor->grabKeyboardFocus();
    resetCursorState();
}

void SoLyPAudioProcessorEditor::exitEditMode()
{
    editMode = false;
    if (textModified) {
        Song song = Song::fromText(textEditor->getText());
        { auto b = getLocalBounds().reduced(40, 20); song.rebuildDisplayLines((float)b.getWidth(), SettingsManager::fontSize); }
        processor.loadSong(song);
    }
    if (leftPanel) leftPanel->setVisible(true);
    if (controlsPanel) controlsPanel->setVisible(true);
    textEditor->setVisible(false);
    saveButton.setVisible(false);
    backButton.setVisible(false);
    editModeLoadButton.setVisible(false);
    editModeNewButton.setVisible(false);
    settingsEditBtn.setVisible(false);
    resized();
    grabKeyboardFocus();
    resetCursorState();
}

void SoLyPAudioProcessorEditor::enterSettingsMode()
{
    settingsMode = true;
    if (editMode) {
        editMode = false;
        if (textModified) {
            Song song = Song::fromText(textEditor->getText());
            { auto b = getLocalBounds().reduced(40, 20); song.rebuildDisplayLines((float)b.getWidth(), SettingsManager::fontSize); }
            processor.loadSong(song);
        }
        textEditor->setVisible(false);
        saveButton.setVisible(false);
        backButton.setVisible(false);
        editModeLoadButton.setVisible(false);
        editModeNewButton.setVisible(false);
        settingsEditBtn.setVisible(false);
    } else {
        if (leftPanel) leftPanel->setVisible(false);
        if (controlsPanel) controlsPanel->setVisible(false);
    }
    backButton.setVisible(true);
    settingsComponent = std::make_unique<SettingsComponent>([this] { onLanguageChanged(); });
    addAndMakeVisible(settingsComponent.get());
    resized();
    resetCursorState();
}

void SoLyPAudioProcessorEditor::exitSettingsMode()
{
    languageChangeGuard = false;
    settingsMode = false;
    settingsComponent = nullptr;
    backButton.setVisible(false);
    if (leftPanel) leftPanel->setVisible(true);
    if (controlsPanel) controlsPanel->setVisible(true);
    if (auto* top = getTopLevelComponent()) top->setAlwaysOnTop(SettingsManager::alwaysOnTop);
    resized();
    resetCursorState();
}

void SoLyPAudioProcessorEditor::onLanguageChanged()
{
    if (languageChangeGuard) return;
    languageChangeGuard = true;
    if (leftPanel) leftPanel->refreshText();
    backButton.setTooltip(I18n::get("button.back"));
    saveButton.setTooltip(I18n::get("button.save"));
    editModeLoadButton.setTooltip(I18n::get("button.load"));
    editModeNewButton.setTooltip(I18n::get("button.new"));
    settingsEditBtn.setTooltip(I18n::get("settings.go"));
    repaint();
    auto* alert = new juce::AlertWindow(I18n::get("settings.language"),
        I18n::get("settings.languageChanged"), juce::AlertWindow::InfoIcon);
    alert->setColour(juce::AlertWindow::backgroundColourId, Theme::bgPanel);
    alert->setColour(juce::AlertWindow::textColourId, Theme::textPrimary);
    alert->setColour(juce::AlertWindow::outlineColourId, Theme::accentBorder);
    alert->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
    alert->enterModalState(true, juce::ModalCallbackFunction::create([this, alert](int) {
        languageChangeGuard = false;
        if (settingsMode) exitSettingsMode();
        delete alert;
    }));
}

void SoLyPAudioProcessorEditor::showSaveDialog()
{
    lastError = {};
    auto text = textEditor->getText();
    if (text.trim().isEmpty()) { lastError = I18n::get("error.empty"); repaint(); return; }
    Song song = Song::fromText(text);
    if (!song.validationError.isEmpty()) { lastError = song.validationError; repaint(); return; }
    if (lastFilename.isEmpty() && !song.textSong.isEmpty())
        lastFilename = juce::File::createLegalFileName(song.textSong[0].sectionName);
    auto* dialog = new SaveDialogComponent(lastFilename, lastSongTitle,
        [this](const juce::String& name, const juce::String& title) { doSave(name, title); },
        []() {});
    addAndMakeVisible(dialog);
    dialog->setBounds(getLocalBounds());
    if (SettingsManager::cursorEnabled) dialog->setMouseCursor(juce::MouseCursor::NoCursor);
    dialog->activateModal([this, dialog] {
        juce::MessageManager::callAsync([this, dialog] { removeChildComponent(dialog); delete dialog; });
    });
}

void SoLyPAudioProcessorEditor::paintCursor(juce::Graphics& g)
{
    bool curEnabled = SettingsManager::cursorEnabled;
    if (curEnabled != lastCursorEnabled) {
        lastCursorEnabled = curEnabled;
        std::function<void(juce::Component*)> applyCursor;
        applyCursor = [&](juce::Component* c) {
            if (dynamic_cast<juce::TextEditor*>(c) != nullptr) return;
            c->setMouseCursor(curEnabled ? juce::MouseCursor::NoCursor : juce::MouseCursor::NormalCursor);
            for (auto* child : c->getChildren()) applyCursor(child);
        };
        applyCursor(this);
        if (curEnabled)
            for (auto* child : getChildren())
                if (dynamic_cast<SaveDialogComponent*>(child) || dynamic_cast<juce::AlertWindow*>(child))
                    applyCursor(child);
    }
    if (!curEnabled) return;
    {
        bool hasDialog = false;
        for (auto* child : getChildren())
            if (dynamic_cast<SaveDialogComponent*>(child) || dynamic_cast<juce::AlertWindow*>(child)) { hasDialog = true; break; }
        if (!hasDialog && textEditor && textEditor->isVisible()
            && textEditor->getScreenBounds().contains(juce::Desktop::getInstance().getMousePosition()))
            return;
    }
    if (SettingsManager::cursorColor != cssCursorColor || SettingsManager::cursorShape != cssCursorShape) {
        cssCursorColor = SettingsManager::cursorColor;
        cssCursorShape = SettingsManager::cursorShape;
        auto triXml = juce::XmlDocument::parse(juce::String(Icons::cursorTriangle).replace("#000", "#" + cssCursorColor));
        auto sqXml  = juce::XmlDocument::parse(juce::String(Icons::cursorSquare).replace("#000", "#" + cssCursorColor));
        if (triXml) cursorTri = juce::Drawable::createFromSVG(*triXml);
        if (sqXml)  cursorSq  = juce::Drawable::createFromSVG(*sqXml);
    }
    auto* drawable = (SettingsManager::cursorShape == 0) ? cursorTri.get() : cursorSq.get();
    if (!drawable) return;
    auto screenPos = juce::Desktop::getInstance().getMousePosition();
    auto pos = getLocalPoint(nullptr, screenPos);
    if (!getLocalBounds().contains(pos)) return;
    float sz = (float)SettingsManager::cursorSize;
    float scale = sz / 100.0f;
    float angle = SettingsManager::cursorRotation ? cursorAngle : 0.0f;
    if (SettingsManager::cursorRotDir == 1) angle = -angle;
    g.saveState();
    g.addTransform(juce::AffineTransform::translation((float)pos.x, (float)pos.y));
    g.addTransform(juce::AffineTransform::rotation(angle));
    g.addTransform(juce::AffineTransform::scale(scale));
    g.addTransform(juce::AffineTransform::translation(-50.0f, -50.0f));
    g.setOpacity(0.7f);
    drawable->draw(g, 1.0f);
    g.restoreState();
}

void SoLyPAudioProcessorEditor::resetCursorState() { lastCursorEnabled = false; }

void SoLyPAudioProcessorEditor::updateLineCount()
{
    if (!controlsPanel) return;
    const auto& song = processor.getCurrentSong();
    if (song.textSong.isEmpty()) return;
    int fitting = calcFittingLines(getHeight(), SettingsManager::fontSize, song);
    int newVal = juce::jmin((int)controlsPanel->linesSlider.getValue(), fitting);
    controlsPanel->linesSlider.setValue((double)newVal, juce::dontSendNotification);
}
