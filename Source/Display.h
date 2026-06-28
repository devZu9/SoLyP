#pragma once

#include <JuceHeader.h>
#include "Logic.h"
#include "Slot.h"
#include "UI/Display/DisplayHelpers.h"
#include "UI/ControlsPanel.h"
#include "UI/LeftPanel.h"

class SettingsComponent;

class SoLyPAudioProcessorEditor : public juce::AudioProcessorEditor,
                                  private juce::MultiTimer,
                                  private juce::Button::Listener,
                                  private juce::Slider::Listener,
                                  private juce::ComponentListener
{
public:
    static constexpr double boost = 15.0;

    SoLyPAudioProcessorEditor(SoLyPAudioProcessor&);
    ~SoLyPAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    void componentMovedOrResized(juce::Component&, bool wasMoved, bool wasResized) override;
    bool keyPressed(const juce::KeyPress&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    bool isTextModified() const { return pendingChanges; }

private:
    enum TimerType { TimerScroll = 1, TimerPause = 2, TimerCountdown = 3, TimerCursor = 4, TimerPreLines = 5 };

    void timerCallback(int timerId) override;
    void scrollCallback();
    void pauseCallback();
    void preLinesCallback();
    void countdownCallback();
    void cursorCallback();
    void buttonClicked(juce::Button*) override;
    void sliderValueChanged(juce::Slider*) override;

    void initPaint(juce::Graphics& g);
    void paintScroll(juce::Graphics& g);
    void paintPauseText(juce::Graphics& g);
    void paintStatusBar(juce::Graphics& g);
    void paintCountdown(juce::Graphics& g);
    void paintError(juce::Graphics& g);
    void paintCursor(juce::Graphics& g);
    void rebuildDisplayLines();
    void initView(juce::Graphics& g);
    void resetCursorState();
    void updateLineCount();

    void showSaveDialog();
    void doSave(const juce::String& filename, const juce::String& songTitle);
    void loadSongFromFile();
    void applySongLoad(Song& song, const juce::File& file);

    void enterEditMode(bool blank = false);
    void exitEditMode();
    void enterSettingsMode();
    void exitSettingsMode();
    void onLanguageChanged();

    // slot management
    void initSlots();
    void setupPreLines();
    double getTimePerLine();

    SoLyPAudioProcessor& processor;

    // slots
    std::vector<Slot> slots;
    int nextLineIndex = 0;
    double lastScrollTime = 0.0;
    bool useCenterY = true;
    SoLyPAudioProcessor::TransportState lastState = SoLyPAudioProcessor::TransportState::Stopped;
    std::atomic<bool> stateChangeQueued{false};
    bool sectionJumped = false;

    // pause
    bool showPauseText = false;
    double pauseMsgY = 0.0;
    juce::Rectangle<int> lyricsViewArea;
    double realLineHeight = 0.0;
    double topYlastLine = 0.0;
    double topLimit = 0.0;
    juce::Image fastImage;
    double fastImageY = 0.0;

    // countdown
    int countdownPhase = 0;
    double countdownPhaseStart = 0.0;
    double countdownPhaseDuration = 0.0;

    int pauseShiftCount = 0;
    double lastPreLineTime = 0.0;
    double lastPauseTime = 0.0;
    double pauseMsgSpeed = 0.0;

    juce::String lastError;

    // edit mode
    bool editMode = false;
    std::unique_ptr<juce::TextEditor> textEditor;
    juce::DrawableButton saveButton{ "", juce::DrawableButton::ImageFitted };
    juce::DrawableButton backButton{ "", juce::DrawableButton::ImageFitted };
    juce::DrawableButton editModeLoadButton{ "", juce::DrawableButton::ImageFitted };
    juce::DrawableButton editModeNewButton{ "", juce::DrawableButton::ImageFitted };

    // settings mode
    bool settingsMode = false;
    std::unique_ptr<SettingsComponent> settingsComponent;
    juce::DrawableButton settingsEditBtn{ "", juce::DrawableButton::ImageFitted };

    juce::String lastFilename;
    juce::String lastSongTitle;
    std::unique_ptr<LeftPanel> leftPanel;
    std::unique_ptr<ControlsPanel> controlsPanel;

    bool languageChangeGuard = false;
    bool textModified = false;
    bool pendingChanges = false;
    juce::String originalText;
    juce::TooltipWindow tooltipWindow{ this, 500 };
    std::unique_ptr<juce::LookAndFeel> tooltipLAF;

    std::unique_ptr<juce::Drawable> cursorTri, cursorSq;
    float cursorAngle = 0.0f;
    juce::Point<int> mousePos;
    juce::String cssCursorColor;
    int cssCursorShape = -1;
    bool lastCursorEnabled = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoLyPAudioProcessorEditor)
};
