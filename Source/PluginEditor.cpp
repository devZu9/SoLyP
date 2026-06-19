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

        // clean leftover temp files from JUCE atomic writes
        auto parent = juce::File(getDefaultSongDir()).getParentDirectory();
        for (auto& f : parent.findChildFiles(juce::File::findFiles, false, "*.tmp"))
            f.deleteFile();
    }
}

SoLyPAudioProcessorEditor::SoLyPAudioProcessorEditor(SoLyPAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    // single instance
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

    // load theme first — all Theme:: colours must be set before any widget is created
    auto themeDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("SoLyP").getChildFile("themes");
    Theme::loadFromFile(themeDir.getChildFile("dark.json"));

    // load settings once, apply language
    SettingsManager::load();
    I18n::setLanguage(SettingsManager::language);

    ensureSongsDir();

    setSize(800, 500);
    setResizable(true, false);
    setConstrainer(nullptr);

    // apply saved window size
    if (SettingsManager::windowWidth > 0 && SettingsManager::windowHeight > 0)
        setSize(SettingsManager::windowWidth, SettingsManager::windowHeight);

    // defer: restore position, register listener, allow saves
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
    startTimerHz(20);

    // load cursor SVGs
    {
        auto triXml = juce::XmlDocument::parse(juce::String(Icons::cursorTriangle));
        auto sqXml  = juce::XmlDocument::parse(juce::String(Icons::cursorSquare));
        if (triXml) cursorTri = juce::Drawable::createFromSVG(*triXml);
        if (sqXml)  cursorSq  = juce::Drawable::createFromSVG(*sqXml);
    }

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

    // settings button in editor (gear icon)
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
    addAndMakeVisible(settingsEditBtn);

    // left panel
    leftPanel = std::make_unique<LeftPanel>();
    leftPanel->editButton.addListener(this);
    leftPanel->loadButton.addListener(this);
    leftPanel->settingsBtn.addListener(this);
    leftPanel->setBounds(8, 8, leftPanel->getRequiredWidth(), LeftPanel::compHeight);
    addAndMakeVisible(leftPanel.get());

    // controls panel
    controlsPanel = std::make_unique<ControlsPanel>();
    controlsPanel->linesSlider.addListener(this);
    controlsPanel->fontSizeSlider.addListener(this);
    controlsPanel->setBounds(getWidth() - ControlsPanel::compWidth - 8, 8,
                             ControlsPanel::compWidth, ControlsPanel::compHeight);
    addAndMakeVisible(controlsPanel.get());

    // apply initial slider values from SettingsManager
    controlsPanel->linesSlider.setValue((double)SettingsManager::visibleLines, juce::dontSendNotification);
    controlsPanel->fontSizeSlider.setValue((double)SettingsManager::fontSize, juce::dontSendNotification);

    processor.onStateChanged = [this]() { repaint(); };

}

SoLyPAudioProcessorEditor::~SoLyPAudioProcessorEditor()
{
    processor.onStateChanged = nullptr;
    if (auto* top = getTopLevelComponent())
        top->removeComponentListener(this);
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
    return;
}

bool SoLyPAudioProcessorEditor::keyPressed(const juce::KeyPress&)
{
    return false;
}

void SoLyPAudioProcessorEditor::timerCallback()
{
    if (!SettingsManager::cursorEnabled) return;

    auto screenPos = juce::Desktop::getInstance().getMousePosition();
    mousePos = getLocalPoint(nullptr, screenPos);

    if (!getLocalBounds().contains(mousePos)) return;

    cursorAngle -= 0.005f * powf(1.3f, (float)(SettingsManager::cursorRotSpeed - 1));
    if (cursorAngle < 0.0f) cursorAngle += juce::MathConstants<float>::twoPi;
    repaint();
}

void SoLyPAudioProcessorEditor::mouseMove(const juce::MouseEvent& e)
{
    if (editMode) return;
    mousePos = e.getPosition();
}
void SoLyPAudioProcessorEditor::mouseExit(const juce::MouseEvent&)
{
    mousePos = {};
}

void SoLyPAudioProcessorEditor::buttonClicked(juce::Button* btn)
{
    if (btn == &saveButton)
        showSaveDialog();
    else if (btn == &backButton)
    {
        if (settingsMode)
            exitSettingsMode();
        else
            exitEditMode();
    }
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
        enterSettingsMode();
    }
    else if (btn == &settingsEditBtn)
    {
        enterSettingsMode();
    }
}

void SoLyPAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    if (controlsPanel != nullptr)
    {
        if (slider == &controlsPanel->linesSlider)
            SettingsManager::visibleLines = static_cast<int>(controlsPanel->linesSlider.getValue());
        else if (slider == &controlsPanel->fontSizeSlider)
            SettingsManager::fontSize = static_cast<float>(controlsPanel->fontSizeSlider.getValue());
    }

    SettingsManager::save();
    repaint();
}

// ── paint ───────────────────────────────────────────────────────────────────

void SoLyPAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(Theme::bgMain);

    if (settingsMode || editMode)
        return;

    auto state = processor.getTransportState();
    paintLyrics(g);

    if (state == SoLyPAudioProcessor::TransportState::Paused)
        paintPauseOverlay(g);

    if (!lastError.isEmpty())
        paintError(g);
}

void SoLyPAudioProcessorEditor::paintOverChildren(juce::Graphics& g)
{
    paintCursor(g);
}

// ── resized ─────────────────────────────────────────────────────────────────

void SoLyPAudioProcessorEditor::resized()
{
    if (settingsMode)
    {
        auto area = getLocalBounds().reduced(10);
        auto buttonBar = area.removeFromTop(40);
        backButton.setBounds(buttonBar.removeFromLeft(100).reduced(4));
        if (settingsComponent != nullptr)
            settingsComponent->setBounds(area);
        return;
    }

    if (editMode)
    {
        auto area = getLocalBounds().reduced(10);
        auto buttonBar = area.removeFromTop(40);
        backButton.setBounds(buttonBar.removeFromLeft(100).reduced(4));
        editModeLoadButton.setBounds(buttonBar.removeFromLeft(100).reduced(4));
        saveButton.setBounds(buttonBar.removeFromLeft(100).reduced(4));
        // gear button — right-aligned
        int gs = 28;
        settingsEditBtn.setBounds(buttonBar.getRight() - gs - 4,
                                  buttonBar.getY() + (buttonBar.getHeight() - gs) / 2,
                                  gs, gs);
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
    settingsEditBtn.setVisible(true);
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
    settingsEditBtn.setVisible(false);
    resized();
    grabKeyboardFocus();
}

// ── settings mode ───────────────────────────────────────────────────────────

void SoLyPAudioProcessorEditor::enterSettingsMode()
{
    settingsMode = true;

    if (editMode)
    {
        editMode = false;
        textEditor->setVisible(false);
        saveButton.setVisible(false);
        backButton.setVisible(false);
        editModeLoadButton.setVisible(false);
        settingsEditBtn.setVisible(false);
    }
    else
    {
        if (leftPanel != nullptr) leftPanel->setVisible(false);
        if (controlsPanel != nullptr) controlsPanel->setVisible(false);
    }

    // show back button for returning from settings
    backButton.setButtonText(I18n::get("button.back"));
    backButton.setVisible(true);

    settingsComponent = std::make_unique<SettingsComponent>([this] { onLanguageChanged(); });
    addAndMakeVisible(settingsComponent.get());
    resized();
}

void SoLyPAudioProcessorEditor::exitSettingsMode()
{
    languageChangeGuard = false;
    settingsMode = false;
    settingsComponent = nullptr;
    backButton.setVisible(false);

    if (leftPanel != nullptr) leftPanel->setVisible(true);
    if (controlsPanel != nullptr) controlsPanel->setVisible(true);
    if (auto* top = getTopLevelComponent())
        top->setAlwaysOnTop(SettingsManager::alwaysOnTop);
    resized();
}

void SoLyPAudioProcessorEditor::onLanguageChanged()
{
    if (languageChangeGuard) return;
    languageChangeGuard = true;

    saveButton.setButtonText(I18n::get("button.save"));
    backButton.setButtonText(I18n::get("button.back"));
    editModeLoadButton.setButtonText(I18n::get("button.load"));

    if (leftPanel != nullptr)
        leftPanel->refreshText();

    repaint();

    auto* alert = new juce::AlertWindow(
        I18n::get("settings.language"),
        I18n::get("settings.languageChanged"),
        juce::AlertWindow::InfoIcon);
    alert->setColour(juce::AlertWindow::backgroundColourId, Theme::bgPanel);
    alert->setColour(juce::AlertWindow::textColourId, Theme::textPrimary);
    alert->setColour(juce::AlertWindow::outlineColourId, Theme::accentBorder);
    alert->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
    alert->enterModalState(true, juce::ModalCallbackFunction::create([this, alert](int) {
        languageChangeGuard = false;
        if (settingsMode)
            exitSettingsMode();
        delete alert;
    }));
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

void SoLyPAudioProcessorEditor::paintCursor(juce::Graphics& g)
{
    if (!SettingsManager::cursorEnabled)
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        std::function<void(juce::Component*)> restore;
        restore = [&](juce::Component* c) {
            c->setMouseCursor(juce::MouseCursor::NormalCursor);
            for (auto* child : c->getChildren())
                restore(child);
        };
        restore(this);
        return;
    }

    std::function<void(juce::Component*)> setAll;
    setAll = [&](juce::Component* c) {
        if (dynamic_cast<juce::TextEditor*>(c) == nullptr)
            c->setMouseCursor(juce::MouseCursor::NoCursor);
        for (auto* child : c->getChildren())
            setAll(child);
    };
    setAll(this);

    // don't draw over text editor
    if (textEditor && textEditor->isVisible() && textEditor->getScreenBounds().contains(
        juce::Desktop::getInstance().getMousePosition()))
        return;

    // reload drawables if color changed
    if (SettingsManager::cursorColor != cssCursorColor || SettingsManager::cursorShape != cssCursorShape)
    {
        cssCursorColor = SettingsManager::cursorColor;
        cssCursorShape = SettingsManager::cursorShape;
        auto triXml = juce::XmlDocument::parse(juce::String(Icons::cursorTriangle)
            .replace("#000", "#" + cssCursorColor));
        auto sqXml  = juce::XmlDocument::parse(juce::String(Icons::cursorSquare)
            .replace("#000", "#" + cssCursorColor));
        if (triXml) cursorTri = juce::Drawable::createFromSVG(*triXml);
        if (sqXml)  cursorSq  = juce::Drawable::createFromSVG(*sqXml);
    }

    auto* drawable = (SettingsManager::cursorShape == 0) ? cursorTri.get() : cursorSq.get();
    if (!drawable) return;
    if (!getLocalBounds().contains(mousePos)) return;

    float sz = (float)SettingsManager::cursorSize;
    float scale = sz / 100.0f;

    float angle = SettingsManager::cursorRotation ? cursorAngle : 0.0f;
    if (SettingsManager::cursorRotDir == 1)
        angle = -angle;

    g.saveState();
    g.addTransform(juce::AffineTransform::translation((float)mousePos.x, (float)mousePos.y));
    g.addTransform(juce::AffineTransform::rotation(angle));
    g.addTransform(juce::AffineTransform::scale(scale));
    g.addTransform(juce::AffineTransform::translation(-50.0f, -50.0f));
    g.setOpacity(0.7f);
    drawable->draw(g, 1.0f);
    g.restoreState();
}
