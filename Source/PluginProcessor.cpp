#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        layout.add(std::make_unique<juce::AudioParameterFloat>("bpm", "BPM", 20.0f, 300.0f, 120.0f));
        layout.add(std::make_unique<juce::AudioParameterBool>("manualBpm", "Manual BPM", false));
        layout.add(std::make_unique<juce::AudioParameterInt>("midiChannel", "MIDI Channel", 0, 16, 0));
        layout.add(std::make_unique<juce::AudioParameterInt>("preLines", "Pre-lines on Pause", 1, 5, 1));
        return layout;
    }
}

SoLyPAudioProcessor::SoLyPAudioProcessor()
    :       AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    apvts.addParameterListener("preLines", this);
    preLinesOnPause = static_cast<int>(*apvts.getRawParameterValue("preLines"));
}

SoLyPAudioProcessor::~SoLyPAudioProcessor()
{
    apvts.removeParameterListener("preLines", this);
}

void SoLyPAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == "preLines")
        preLinesOnPause = static_cast<int>(newValue);
}

const juce::String SoLyPAudioProcessor::getName() const { return "SoLyP"; }

bool SoLyPAudioProcessor::acceptsMidi() const { return true; }
bool SoLyPAudioProcessor::producesMidi() const { return false; }
bool SoLyPAudioProcessor::isMidiEffect() const { return true; }
double SoLyPAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int SoLyPAudioProcessor::getNumPrograms() { return 1; }
int SoLyPAudioProcessor::getCurrentProgram() { return 0; }
void SoLyPAudioProcessor::setCurrentProgram(int) {}
const juce::String SoLyPAudioProcessor::getProgramName(int) { return {}; }
void SoLyPAudioProcessor::changeProgramName(int, const juce::String&) {}

void SoLyPAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void SoLyPAudioProcessor::releaseResources() {}

void SoLyPAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    updateBarPosition(getPlayHead());

    for (const auto& metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        if (message.isNoteOn())
            processMidiMessage(message);
    }
}

void SoLyPAudioProcessor::updateBarPosition(juce::AudioPlayHead* head)
{
    if (!head)
        return;

    auto opt = head->getPosition();
    if (!opt.hasValue())
        return;

    auto& pos = *opt;

    if (pos.getBpm().hasValue())
        currentBpm = *pos.getBpm();

    if (!pos.getTimeInSeconds().hasValue())
        return;

    if (!pos.getIsPlaying())
    {
        if (transportState == TransportState::Playing)
            setTransportState(TransportState::Paused);
        return;
    }

    if (transportState == TransportState::Paused)
        return;

    double ppqPosition = pos.getPpqPosition().orFallback(0.0);
    int currentBarInt = static_cast<int>(std::floor(ppqPosition));

    if (transportState == TransportState::Countdown)
    {
        double ppq = pos.getPpqPosition().orFallback(0.0);

        if (countdownFrom == 3)
        {
            double startBeat = std::floor(ppq) - 1.5;
            double elapsed = ppq - startBeat;
            int digit = 3 - static_cast<int>(elapsed / 0.5);
            if (digit < 0) digit = 0;
            if (digit != countdownValue)
            {
                countdownValue = digit;
                if (countdownValue == 0)
                {
                    transportState = TransportState::Playing;
                    countdownValue = 0;
                    currentLineIndex = 0;
                    lastBar = currentBarInt;
                    if (onStateChanged) onStateChanged();
                    return;
                }
            }
        }
        else if (countdownFrom == 5)
        {
            double startBeat = std::floor(ppq) - 2.5;
            double elapsed = ppq - startBeat;
            int digit = 5 - static_cast<int>(elapsed / 0.5);
            if (digit < 0) digit = 0;
            if (digit != countdownValue)
            {
                countdownValue = digit;
                if (countdownValue == 0)
                {
                    transportState = TransportState::Playing;
                    countdownValue = 0;
                    currentLineIndex = 0;
                    lastBar = currentBarInt;
                    if (onStateChanged) onStateChanged();
                    return;
                }
            }
        }

        if (onStateChanged) onStateChanged();
        return;
    }

    if (currentBarInt != lastBar)
    {
        if (transportState == TransportState::Playing)
        {
            advanceLine();
        }
        lastBar = currentBarInt;
    }
}

void SoLyPAudioProcessor::advanceLine()
{
    if (currentSong.sections.isEmpty())
        return;

    const auto& section = currentSong.sections[currentSectionIndex];
    currentLineIndex++;

    if (currentLineIndex >= static_cast<int>(section.lines.size()))
    {
        currentLineIndex = 0;
        switchToNextSection();
    }

    if (onStateChanged) onStateChanged();
}

void SoLyPAudioProcessor::switchToSection(int index)
{
    if (index < 0 || index >= static_cast<int>(currentSong.sections.size()))
        return;

    currentSectionIndex = index;
    currentLineIndex = 0;

    if (onStateChanged) onStateChanged();
}

void SoLyPAudioProcessor::switchToNextSection()
{
    int next = currentSectionIndex + 1;
    if (next >= static_cast<int>(currentSong.sections.size()))
        next = 0;
    switchToSection(next);
}

void SoLyPAudioProcessor::startCountdown(int fromValue)
{
    transportState = TransportState::Countdown;
    countdownFrom = fromValue;
    countdownValue = fromValue;
    if (onStateChanged) onStateChanged();
}

void SoLyPAudioProcessor::processMidiMessage(const juce::MidiMessage& msg)
{
    if (!msg.isNoteOn())
        return;

    int channel = static_cast<int>(*apvts.getRawParameterValue("midiChannel"));
    if (channel != 0 && msg.getChannel() != channel)
        return;

    midiManager.processNote(msg.getNoteNumber(), [this](MidiManager::Command cmd) {
        switch (cmd)
        {
        case MidiManager::Command::Play:
            if (transportState == TransportState::Paused || transportState == TransportState::Stopped)
                setTransportState(TransportState::Playing);
            break;
        case MidiManager::Command::Pause:
            if (transportState == TransportState::Playing || transportState == TransportState::Countdown)
                setTransportState(TransportState::Paused);
            break;
        case MidiManager::Command::NextSection:
            switchToNextSection();
            break;
        case MidiManager::Command::Hybrid:
            switchToNextSection();
            setTransportState(TransportState::Playing);
            break;
        case MidiManager::Command::Countdown3:
            startCountdown(3);
            break;
        case MidiManager::Command::Countdown5:
            startCountdown(5);
            break;
        case MidiManager::Command::Landmark:
        {
            int sectionIndex = midiManager.getLastLandmarkSection();
            if (sectionIndex >= 0)
                switchToSection(sectionIndex);
            break;
        }
        }
    });
}

void SoLyPAudioProcessor::setTransportState(TransportState state)
{
    transportState = state;
    if (state == TransportState::Playing)
    {
        auto opt = getPlayHead()->getPosition();
        if (opt.hasValue())
            lastBar = static_cast<int>(std::floor(opt->getPpqPosition().orFallback(0.0)));
    }
    if (onStateChanged) onStateChanged();
}

void SoLyPAudioProcessor::loadSong(const Song& song)
{
    currentSong = song;
    currentSectionIndex = 0;
    currentLineIndex = 0;
    transportState = TransportState::Stopped;
    if (onStateChanged) onStateChanged();
}

void SoLyPAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SoLyPAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr)
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* SoLyPAudioProcessor::createEditor()
{
    return new SoLyPAudioProcessorEditor(*this);
}

bool SoLyPAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SoLyPAudioProcessor();
}
