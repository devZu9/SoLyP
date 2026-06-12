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
    void setTransportState(TransportState state);

    const Song& getCurrentSong() const { return currentSong; }
    void loadSong(const Song& song);

    int getCurrentSectionIndex() const { return currentSectionIndex; }
    int getCurrentLineIndex() const { return currentLineIndex; }
    int getCurrentBar() const { return lastBar; }
    int getCountdownValue() const { return countdownValue; }

    int getPreLinesOnPause() const { return preLinesOnPause; }
    void setPreLinesOnPause(int count) { preLinesOnPause = count; }

    std::function<void()> onStateChanged;

private:
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    void advanceLine();
    void switchToSection(int index);
    void switchToNextSection();
    void startCountdown(int fromValue);
    void processMidiMessage(const juce::MidiMessage& msg);
    void updateBarPosition(juce::AudioPlayHead* playHead);

    juce::AudioProcessorValueTreeState apvts;

    TransportState transportState = TransportState::Stopped;
    Song currentSong;
    int currentSectionIndex = 0;
    int currentLineIndex = 0;

    int lastBar = 0;
    bool barAdvanced = false;
    double currentBpm = 120.0;

    int countdownValue = 0;
    int countdownFrom = 0;
    double countdownFraction = 0.0;

    int preLinesOnPause = 1;

    MidiManager midiManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoLyPAudioProcessor)
};
