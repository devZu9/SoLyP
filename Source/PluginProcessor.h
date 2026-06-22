#pragma once

#include <JuceHeader.h>
#include "Data/Song.h"
#include "Midi/MidiManager.h"

class SoLyPAudioProcessor : public juce::AudioProcessor,
                            public juce::AudioProcessorValueTreeState::Listener
{
public:
    SoLyPAudioProcessor();
    ~SoLyPAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    enum class TransportState { Stopped, Playing, Paused, Countdown };
    TransportState getTransportState() const { return transportState; }

    void enterPlay();
    void enterPause();
    void enterStop();
    void enterCountdown();

    const Song& getCurrentSong() const { return currentSong; }
    void loadSong(const Song& song);

    int getCurrentSectionIndex() const { return currentSectionIndex; }
    int getCurrentBar() const { return scrollHead >= 0.0 ? (int)scrollHead : 0; }
    int getLastMidiNote() const { return lastMidiNote; }
    double getLastMidiNoteTime() const { return lastMidiNoteTime; }
    double getScrollHead() const { return scrollHead; }
    int getPreLinesOnPause() const { return preLinesOnPause; }
    void setPreLinesOnPause(int count) { preLinesOnPause = count; }
    bool isPauseLineActive() const { return pauseLineActive; }
    int getPauseLineDisplayIdx() const { return pauseLineDisplayIdx; }
    int getResumeSkipIdx() const { return resumeSkipIdx; }
    bool getShowPauseText() const { return showPauseText; }

    void timerTick();

    std::function<void()> onStateChanged;

private:
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    void processMidiMessage(const juce::MidiMessage& msg);

    void switchToSection(int index);
    void switchToNextSection();

    juce::AudioProcessorValueTreeState apvts;

    TransportState transportState = TransportState::Stopped;
    Song currentSong;
    int currentSectionIndex = 0;

    double currentBpm = 120.0;
    double scrollHead = -1.0;
    double lastTimerUpdate = 0.0;

    int preLinesOnPause = 1;
    bool pauseLineActive = false;
    int pauseLineDisplayIdx = -1;
    bool showPauseText = true;
    int resumeSkipIdx = 0;

    int lastMidiNote = -1;
    double lastMidiNoteTime = 0.0;

    MidiManager midiManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoLyPAudioProcessor)
};
