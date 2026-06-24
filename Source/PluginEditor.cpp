#include "PluginEditor.h"
#include "PluginEditorHelpers.h"
#include "UI/Theme.h"
#include "UI/SettingsComponent.h"
#include "UI/SaveDialogComponent.h"
#include "UI/icons/Icons.h"
#include "Data/SettingsManager.h"
#include "Data/I18n.h"

namespace
{
    void ensureSongsDir()
    {
        juce::File(getDefaultSongDir()).createDirectory();
        auto parent = juce::File(getDefaultSongDir()).getParentDirectory();
        for (auto& f : parent.findChildFiles(juce::File::findFiles, false, "*.tmp"))
            f.deleteFile();
    }
}

SoLyPAudioProcessorEditor::SoLyPAudioProcessorEditor(SoLyPAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    {
        static juce::InterProcessLock lock("SoLyP_SingleInstance");
        if (!lock.enter(100))
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::MessageBoxIconType::WarningIcon,
                "SoLyP",
                juce::String::fromUTF8("Приложение уже запущено"),
                "OK");
            juce::MessageManager::callAsync([] { juce::JUCEApplication::getInstance()->quit(); });
            return;
        }
    }

    auto themeDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("SoLyP").getChildFile("themes");
    Theme::loadFromFile(themeDir.getChildFile("dark.json"));

    SettingsManager::load();
    I18n::setLanguage(SettingsManager::language);
    ensureSongsDir();

    setSize(800, 500);
    setResizable(true, false);
    setConstrainer(nullptr);

    if (SettingsManager::windowWidth > 0 && SettingsManager::windowHeight > 0)
        setSize(SettingsManager::windowWidth, SettingsManager::windowHeight);

    juce::MessageManager::callAsync([this] {
        if (auto* top = getTopLevelComponent())
        {
            top->addComponentListener(this);
            top->setAlwaysOnTop(SettingsManager::alwaysOnTop);
            if (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
            {
                if (SettingsManager::windowX != 0 || SettingsManager::windowY != 0)
                    top->setTopLeftPosition(SettingsManager::windowX, SettingsManager::windowY);
            }
        }
        SettingsManager::noStartSave = false;
    });

    setMouseClickGrabsKeyboardFocus(false);
    setWantsKeyboardFocus(true);

    lastScrollTime = juce::Time::getMillisecondCounterHiRes();

    {
        auto triXml = juce::XmlDocument::parse(juce::String(Icons::cursorTriangle));
        auto sqXml  = juce::XmlDocument::parse(juce::String(Icons::cursorSquare));
        if (triXml) cursorTri = juce::Drawable::createFromSVG(*triXml);
        if (sqXml)  cursorSq  = juce::Drawable::createFromSVG(*sqXml);
    }

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
    textEditor->onTextChange = [this] { if (editMode) { textModified = true; pendingChanges = true; } };
    addAndMakeVisible(textEditor.get());

    auto gearSvg = juce::String(Icons::gear).replace("#000000", "#" + Theme::iconPrimary.toDisplayString(false));
    auto gearHoverSvg = juce::String(Icons::gear).replace("#000000", "#" + Theme::iconHover.toDisplayString(false));
    auto gearXml = juce::XmlDocument::parse(gearSvg);
    auto gearHoverXml = juce::XmlDocument::parse(gearHoverSvg);
    if (gearXml != nullptr && gearHoverXml != nullptr)
    {
        auto gearNormal = juce::Drawable::createFromSVG(*gearXml);
        auto gearHover  = juce::Drawable::createFromSVG(*gearHoverXml);
        settingsEditBtn.setImages(gearNormal.get(), gearHover.get());
    }
    settingsEditBtn.setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
    settingsEditBtn.setColour(juce::DrawableButton::backgroundOnColourId, Theme::bgButtonHover);
    settingsEditBtn.setVisible(false);
    settingsEditBtn.addListener(this);
    settingsEditBtn.setTooltip(I18n::get("settings.go"));

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
        btn.addListener(this);
    };

    setupIconBtn(backButton, Icons::arrowCircleLeft, I18n::get("button.back"));
    setupIconBtn(editModeLoadButton, Icons::fileArrowDown, I18n::get("button.load"));
    setupIconBtn(editModeNewButton, Icons::fileDashed, I18n::get("button.new"));
    setupIconBtn(saveButton, Icons::uploadSimpleFill, I18n::get("button.save"));
    addAndMakeVisible(settingsEditBtn);

    leftPanel = std::make_unique<LeftPanel>();
    leftPanel->loadButton.addListener(this);
    leftPanel->newButton.addListener(this);
    leftPanel->editButton.addListener(this);
    leftPanel->settingsBtn.addListener(this);
    leftPanel->setBounds(8, 8, leftPanel->getRequiredWidth(), LeftPanel::compHeight);
    addAndMakeVisible(leftPanel.get());

    controlsPanel = std::make_unique<ControlsPanel>();
    controlsPanel->linesSlider.addListener(this);
    controlsPanel->fontSizeSlider.addListener(this);
    controlsPanel->setBounds(getWidth() - ControlsPanel::compWidth - 8, 8,
                             ControlsPanel::compWidth, ControlsPanel::compHeight);
    addAndMakeVisible(controlsPanel.get());

    controlsPanel->linesSlider.setValue((double)SettingsManager::visibleLines, juce::dontSendNotification);
    controlsPanel->fontSizeSlider.setValue((double)SettingsManager::fontSize, juce::dontSendNotification);

    stateChangeQueued = false;
    processor.onStateChanged = [this]() {
        if (!stateChangeQueued.exchange(true))
        {
            juce::MessageManager::callAsync([this]() {
                stateChangeQueued = false;
                auto state = processor.getTransportState();
                switch (state)
                {
                case SoLyPAudioProcessor::TransportState::Stopped:
                    stopTimer(ScrollId);
                    stopTimer(PauseMsgId);
                    stopTimer(CountdownId);
                    if (slots.empty())
                        initSlots();
                    break;
                case SoLyPAudioProcessor::TransportState::Playing:
                    showPauseText = false;
                    lastScrollTime = juce::Time::getMillisecondCounterHiRes();
                    if (lastState == SoLyPAudioProcessor::TransportState::Stopped && slots.empty())
                        initSlots();
                    startTimer(ScrollId, 50);
                    stopTimer(PauseMsgId);
                    break;
                case SoLyPAudioProcessor::TransportState::Paused:
                    showPauseText = true;
                    pauseMsgY = 0.0;
                    lastScrollTime = juce::Time::getMillisecondCounterHiRes();
                    startTimer(ScrollId, 50);
                    startTimer(PauseMsgId, 50);
                    stopTimer(CountdownId);
                    break;
                case SoLyPAudioProcessor::TransportState::Countdown:
                {
                    double bpm = SettingsManager::manualBpmEnabled
                        ? (double)SettingsManager::manualBpmValue
                        : processor.getCurrentBpm();
                    if (bpm <= 0.0) bpm = 120.0;
                    countdownPhaseDuration = 0.5 * (60000.0 / bpm * (double)SettingsManager::timeSignature);
                    countdownPhase = 1;
                    countdownPhaseStart = juce::Time::getMillisecondCounterHiRes();
                    startTimer(CountdownId, 50);
                    break;
                }
                }
                int st = processor.getSectionTarget();
                if (st >= 0) {
                    const auto& song = processor.getCurrentSong();
                    int idx = findSectionStart(song, st);
                    if (idx >= 0) {
                        nextLineIndex = idx;
                        setupPreLines();
                    }
                    processor.clearSectionTarget();
                } else if (st == -2) {
                    const auto& song = processor.getCurrentSong();
                    int curSid = 0;
                    if (nextLineIndex > 0 && nextLineIndex - 1 < song.displayLines.size())
                        curSid = song.displayLines[nextLineIndex - 1].sectionId;
                    int idx = findSectionStart(song, curSid + 1);
                    if (idx >= 0) {
                        nextLineIndex = idx;
                        setupPreLines();
                    }
                    processor.clearSectionTarget();
                }
                lastState = state;
                repaint();
            });
        }
    };

    if (SettingsManager::cursorEnabled)
        startTimer(CursorId, 16);
}

SoLyPAudioProcessorEditor::~SoLyPAudioProcessorEditor()
{
    processor.onStateChanged = nullptr;
    if (auto* top = getTopLevelComponent())
        top->removeComponentListener(this);
}

void SoLyPAudioProcessorEditor::timerCallback(int timerId)
{
    auto state = processor.getTransportState();
    if (timerId == ScrollId && (state == SoLyPAudioProcessor::TransportState::Playing
        || state == SoLyPAudioProcessor::TransportState::Paused
        || state == SoLyPAudioProcessor::TransportState::Countdown))
    {
        double now = juce::Time::getMillisecondCounterHiRes();
        double elapsed = now - lastScrollTime;
        lastScrollTime = now;
        double timePerLine = getTimePerLine();
        if (timePerLine <= 0.0 || slots.empty()) { repaint(); return; }
        double step = elapsed / (timePerLine * 1000.0);
        if (state == SoLyPAudioProcessor::TransportState::Paused) step *= 5.0;
        if (state == SoLyPAudioProcessor::TransportState::Countdown && countdownPhase > 0) step *= 5.0;
        scrollOffset += step;
        int N = (int)slots.size();
        if (state == SoLyPAudioProcessor::TransportState::Playing
            || state == SoLyPAudioProcessor::TransportState::Countdown)
        {
            while (scrollOffset >= 1.0)
            {
                scrollOffset -= 1.0;
                const auto& song = processor.getCurrentSong();
                if (song.displayLines.isEmpty()) { repaint(); return; }
                for (int i = 0; i < N - 1; ++i)
                    slots[i].text = slots[i + 1].text;
                if (nextLineIndex < song.displayLines.size())
                    slots[N - 1].text = song.displayLines[nextLineIndex++].text;
                else
                    processor.enterStop();
            }
        }
        if (state == SoLyPAudioProcessor::TransportState::Paused && showPauseText)
        {
            double lh = SettingsManager::fontSize * 1.4f;
            pauseMsgY -= step * lh;
        }
        repaint();
        return;
    }
    if (timerId == PauseMsgId)
    {
        if (state != SoLyPAudioProcessor::TransportState::Paused) stopTimer(PauseMsgId);
        return;
    }
    if (timerId == CountdownId)
    {
        double now = juce::Time::getMillisecondCounterHiRes();
        double elapsed = now - countdownPhaseStart;
        if (elapsed >= countdownPhaseDuration)
        {
            countdownPhase++;
            if (countdownPhase > 3)
            {
                countdownPhase = 0;
                stopTimer(CountdownId);
                stopTimer(ScrollId);
                stopTimer(PauseMsgId);
                processor.enterPlay();
                return;
            }
            countdownPhaseStart = now;
        }
        repaint();
        return;
    }
    if (timerId == CursorId)
    {
        if (!SettingsManager::cursorEnabled) { stopTimer(CursorId); return; }
        auto screenPos = juce::Desktop::getInstance().getMousePosition();
        mousePos = getLocalPoint(nullptr, screenPos);
        cursorAngle -= 0.005f * powf(1.3f, (float)(SettingsManager::cursorRotSpeed - 1));
        if (cursorAngle < 0.0f) cursorAngle += juce::MathConstants<float>::twoPi;
        int sz = SettingsManager::cursorSize + 4;
        static juce::Point<int> prevPos;
        repaint(juce::Rectangle<int>(prevPos.x - sz, prevPos.y - sz, sz * 2, sz * 2));
        repaint(juce::Rectangle<int>(mousePos.x - sz, mousePos.y - sz, sz * 2, sz * 2));
        prevPos = mousePos;
    }
}

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

// отрисовка: каждая функция = один элемент
void SoLyPAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(Theme::bgMain);
    if (settingsMode || editMode) return;
    ensureReady();
    paintLines(g);
    paintPauseText(g);
    auto state = processor.getTransportState();
    if (state == SoLyPAudioProcessor::TransportState::Countdown) paintCountdown(g);
    paintStatusBar(g);
    if (!lastError.isEmpty()) paintError(g);
}

void SoLyPAudioProcessorEditor::paintOverChildren(juce::Graphics& g) { paintCursor(g); }

void SoLyPAudioProcessorEditor::resized()
{
    if (settingsMode) {
        auto area = getLocalBounds().reduced(10);
        auto buttonBar = area.removeFromTop(40);
        int gs = 28;
        backButton.setBounds(buttonBar.removeFromLeft(gs).withSizeKeepingCentre(gs, gs));
        if (settingsComponent) settingsComponent->setBounds(area);
        return;
    }
    if (editMode) {
        auto area = getLocalBounds().reduced(10);
        auto buttonBar = area.removeFromTop(40);
        int gs = 28, gap = 8;
        backButton.setBounds(buttonBar.removeFromLeft(gs).withSizeKeepingCentre(gs, gs));
        buttonBar.removeFromLeft(gap);
        editModeLoadButton.setBounds(buttonBar.removeFromLeft(gs).withSizeKeepingCentre(gs, gs));
        buttonBar.removeFromLeft(gap);
        editModeNewButton.setBounds(buttonBar.removeFromLeft(gs).withSizeKeepingCentre(gs, gs));
        buttonBar.removeFromLeft(gap);
        saveButton.setBounds(buttonBar.removeFromLeft(gs).withSizeKeepingCentre(gs, gs));
        settingsEditBtn.setBounds(buttonBar.getRight() - gs - 4, buttonBar.getY() + (buttonBar.getHeight() - gs) / 2, gs, gs);
        textEditor->setBounds(area);
        return;
    }
    repaint();
    if (leftPanel) leftPanel->setBounds(8, 8, leftPanel->getRequiredWidth(), LeftPanel::compHeight);
    if (controlsPanel) controlsPanel->setBounds(getWidth() - ControlsPanel::compWidth - 8, 8, ControlsPanel::compWidth, ControlsPanel::compHeight);
    updateLineCount();
    repaint();
}

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
    if (controlsPanel) {
        if (slider == &controlsPanel->linesSlider)
            SettingsManager::visibleLines = static_cast<int>(controlsPanel->linesSlider.getValue());
        else if (slider == &controlsPanel->fontSizeSlider) {
            SettingsManager::fontSize = static_cast<float>(controlsPanel->fontSizeSlider.getValue());
            updateLineCount();
            SettingsManager::visibleLines = static_cast<int>(controlsPanel->linesSlider.getValue());
        }
    }
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
    if (textModified) processor.loadSong(Song::fromText(textEditor->getText()));
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
        if (textModified) processor.loadSong(Song::fromText(textEditor->getText()));
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
