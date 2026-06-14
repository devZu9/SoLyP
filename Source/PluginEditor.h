#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/ControlsPanel.h"
#include "UI/LeftPanel.h"

class SoLyPAudioProcessorEditor : public juce::AudioProcessorEditor,
                                  private juce::Timer,
                                  private juce::Button::Listener,
                                  private juce::Slider::Listener
{
public:
    SoLyPAudioProcessorEditor(SoLyPAudioProcessor&);
    ~SoLyPAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void buttonClicked(juce::Button*) override;
    void sliderValueChanged(juce::Slider*) override;

    void paintLyrics(juce::Graphics& g);
    void paintCountdown(juce::Graphics& g);
    void paintPauseOverlay(juce::Graphics& g);
    void paintError(juce::Graphics& g);

    void showSaveDialog();
    void doSave(const juce::String& filename, const juce::String& songTitle);
    void loadSongFromFile();

    void enterEditMode();
    void exitEditMode();

    SoLyPAudioProcessor& processor;

    int visibleLines = 6;
    float fontSize = 80.0f;

    juce::String lastError;

    // edit mode
    bool editMode = false;
    std::unique_ptr<juce::TextEditor> textEditor;
    juce::TextButton saveButton{ "" };
    juce::TextButton backButton{ "" };
    juce::TextButton editModeLoadButton{ "" };

    // save dialog data (persisted between invocations)
    juce::String lastFilename;
    juce::String lastSongTitle;

    // left panel
    std::unique_ptr<LeftPanel> leftPanel;

    // controls
    std::unique_ptr<ControlsPanel> controlsPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoLyPAudioProcessorEditor)
};
