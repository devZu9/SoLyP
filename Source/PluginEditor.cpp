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

    // настройка тултипов
    class SoLyPTooltipLAF : public juce::LookAndFeel_V4 {
    public:
        void drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height) override
        {
            float corner = 5.0f;
            g.setColour(Theme::tooltipBg);
            g.fillRoundedRectangle(0.0f, 0.0f, (float)width, (float)height, corner);
            g.setColour(Theme::tooltipOutline);
            g.drawRoundedRectangle(0.5f, 0.5f, (float)width - 1.0f, (float)height - 1.0f, corner, 1.0f);
            g.setColour(Theme::tooltipText);
            g.setFont(juce::Font(juce::FontOptions(Theme::baseFontSize * Theme::tooltipSize)));
            g.drawFittedText(text, 5, 0, width - 10, height, juce::Justification::centred, 5);
        }
    };
    tooltipLAF = std::make_unique<SoLyPTooltipLAF>();
    setLookAndFeel(tooltipLAF.get());
    tooltipWindow.setColour(juce::TooltipWindow::backgroundColourId, Theme::tooltipBg);
    tooltipWindow.setColour(juce::TooltipWindow::textColourId, Theme::tooltipText);
    tooltipWindow.setColour(juce::TooltipWindow::outlineColourId, Theme::tooltipOutline);

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
    controlsPanel->gapSlider.addListener(this);
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
                    stopTimer(PauseId);
                    stopTimer(CountdownId);
                    if (slots.empty())
                        initSlots();
                    break;
                case SoLyPAudioProcessor::TransportState::Playing:
                    showPauseText = false;
                    lastScrollTime = juce::Time::getMillisecondCounterHiRes();
                    if (lastState == SoLyPAudioProcessor::TransportState::Stopped
                        && !processor.getCurrentSong().textSong.isEmpty())
                    {
                        slots.assign(slots.size(), Slot{});
                        nextLineIndex = 0;
                        setupPreLines();
                        nextLineIndex = std::min(SettingsManager::preLinesOnPause, (int)slots.size());
                    }
                    if (lastState == SoLyPAudioProcessor::TransportState::Paused)
                        scrollOffset = 0.0;
                    stopTimer(PauseId);
                    startTimer(ScrollId, 50);
                    break;
                case SoLyPAudioProcessor::TransportState::Paused:
                    showPauseText = true;
                    pauseMsgY = 0.0;
                    lastScrollTime = juce::Time::getMillisecondCounterHiRes();
                    stopTimer(ScrollId);
                    startTimer(PauseId, 50);
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
                    stopTimer(PauseId);
                    startTimer(ScrollId, 50);
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

void SoLyPAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(Theme::bgMain);
    if (settingsMode || editMode) return;
    rebuildDisplayLines();
    auto state = processor.getTransportState();
    if (useCenterY && state == SoLyPAudioProcessor::TransportState::Stopped)
        initPaint(g);
    else
        paintScroll(g);
    paintPauseText(g);
    if (state == SoLyPAudioProcessor::TransportState::Countdown) paintCountdown(g);
    paintStatusBar(g);
    if (!lastError.isEmpty()) paintError(g);
}

void SoLyPAudioProcessorEditor::paintOverChildren(juce::Graphics& g) { paintCursor(g); }

void SoLyPAudioProcessorEditor::resized()
{
    if (settingsMode) {
        auto area = getLocalBounds().reduced(10);
        auto buttonBar = area.removeFromTop(38);
        {
            juce::FlexBox fb;
            fb.flexDirection = juce::FlexBox::Direction::row;
            fb.items.add(juce::FlexItem(backButton).withWidth(28.0f));
            fb.performLayout(buttonBar.toFloat());
        }
        if (settingsComponent) settingsComponent->setBounds(area);
        return;
    }
    if (editMode) {
        auto area = getLocalBounds().reduced(10);
        auto buttonBar = area.removeFromTop(38);
        {
            juce::FlexBox fb;
            fb.flexDirection = juce::FlexBox::Direction::row;
            float w = 28.0f;
            fb.items.add(juce::FlexItem(backButton).withWidth(w));
            fb.items.add(juce::FlexItem(8.0f, 1.0f));
            fb.items.add(juce::FlexItem(editModeLoadButton).withWidth(w));
            fb.items.add(juce::FlexItem(8.0f, 1.0f));
            fb.items.add(juce::FlexItem(editModeNewButton).withWidth(w));
            fb.items.add(juce::FlexItem(8.0f, 1.0f));
            fb.items.add(juce::FlexItem(saveButton).withWidth(w));
            fb.items.add(juce::FlexItem().withFlex(1.0f));
            fb.items.add(juce::FlexItem(settingsEditBtn).withWidth(w));
            fb.performLayout(buttonBar.toFloat());
        }
        textEditor->setBounds(area);
        return;
    }
    auto st = processor.getTransportState();
    if (st != SoLyPAudioProcessor::TransportState::Playing)
    {
        rebuildDisplayLines();
        const auto& song = processor.getCurrentSong();
        if (!song.textSong.isEmpty()) initSlots();
    }
    repaint();
    if (leftPanel) leftPanel->setBounds(8, 8, leftPanel->getRequiredWidth(), LeftPanel::compHeight);
    if (controlsPanel) controlsPanel->setBounds(getWidth() - ControlsPanel::compWidth - 8, 8, ControlsPanel::compWidth, ControlsPanel::compHeight);
    updateLineCount();
    repaint();
}
