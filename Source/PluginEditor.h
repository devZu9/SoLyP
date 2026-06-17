#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/ControlsPanel.h"
#include "UI/LeftPanel.h"

class SettingsComponent;

class SoLyPAudioProcessorEditor : public juce::AudioProcessorEditor,
                                  private juce::Button::Listener,
                                  private juce::Slider::Listener,
                                  private juce::ComponentListener
{
public:
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

private:
    void buttonClicked(juce::Button*) override;
    void sliderValueChanged(juce::Slider*) override;

    void paintLyrics(juce::Graphics& g);
    void paintPauseOverlay(juce::Graphics& g);
    void paintError(juce::Graphics& g);

    void showSaveDialog();
    void doSave(const juce::String& filename, const juce::String& songTitle);
    void loadSongFromFile();

    void enterEditMode();
    void exitEditMode();
    void enterSettingsMode();
    void exitSettingsMode();
    void onLanguageChanged();

    SoLyPAudioProcessor& processor;


    juce::String lastError;

    // edit mode
    bool editMode = false;
    std::unique_ptr<juce::TextEditor> textEditor;
    juce::TextButton saveButton{ "" };
    juce::TextButton backButton{ "" };
    juce::TextButton editModeLoadButton{ "" };

    // settings mode
    bool settingsMode = false;
    std::unique_ptr<SettingsComponent> settingsComponent;
    juce::DrawableButton settingsEditBtn{ "", juce::DrawableButton::ImageFitted };

    // save dialog data (persisted between invocations)
    juce::String lastFilename;
    juce::String lastSongTitle;

    // left panel
    std::unique_ptr<LeftPanel> leftPanel;

    // controls
    std::unique_ptr<ControlsPanel> controlsPanel;

    bool languageChangeGuard = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoLyPAudioProcessorEditor)
};
