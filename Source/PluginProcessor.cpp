#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Data/SettingsManager.h"

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

    // read BPM and play state from DAW playhead
    auto head = getPlayHead();
    if (head)
    {
        auto opt = head->getPosition();
        if (opt.hasValue())
        {
            auto& pos = *opt;
            if (pos.getBpm().hasValue())
                currentBpm = *pos.getBpm();
            if (pos.getIsPlaying())
            {
                if (transportState == TransportState::Stopped || transportState == TransportState::Paused)
                    setTransportState(TransportState::Playing);
            }
        }
    }

    for (const auto& metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        if (message.isNoteOn())
            processMidiMessage(message);
    }
}

void SoLyPAudioProcessor::timerTick()
{
    double now = juce::Time::getMillisecondCounterHiRes();

    if (scrollHead >= 0.0 && transportState == TransportState::Playing)
    {
        if (currentSong.displayLines.isEmpty())
        {
            lastTimerUpdate = now;
            return;
        }

        double elapsed = now - lastTimerUpdate;
        int idx = (int)scrollHead;

        if (idx >= currentSong.displayLines.size())
        {
            transportState = TransportState::Stopped;
            if (onStateChanged) onStateChanged();
            lastTimerUpdate = now;
            return;
        }

        double bpm = SettingsManager::manualBpmEnabled
            ? (double)SettingsManager::manualBpmValue
            : currentBpm;
        if (bpm <= 0.0) bpm = 120.0;

        int parts = currentSong.displayLines[idx].parts;
        double barDuration = 60.0 / bpm * (double)SettingsManager::timeSignature;
        double timePerLine = barDuration / ((double)SettingsManager::linesPerBar * (double)parts);

        if (timePerLine > 0.0)
        {
            double advance = elapsed / (timePerLine * 1000.0);
            double newHead = scrollHead + advance;

            int newIdx = (int)newHead;
            if (newIdx >= currentSong.displayLines.size())
            {
                transportState = TransportState::Stopped;
                scrollHead = (double)(currentSong.displayLines.size() - 1);
                if (onStateChanged) onStateChanged();
                lastTimerUpdate = now;
                return;
            }

            int oldSection = currentSong.displayLines[idx].sectionIndex;
            int newSection = currentSong.displayLines[newIdx].sectionIndex;
            if (newSection != oldSection)
                currentSectionIndex = newSection;

            scrollHead = newHead;
            if (onStateChanged) onStateChanged();
        }
    }

    lastTimerUpdate = now;
}

void SoLyPAudioProcessor::setTransportState(TransportState state)
{
    transportState = state;

    if (state == TransportState::Playing)
    {
        lastTimerUpdate = juce::Time::getMillisecondCounterHiRes();
        if (scrollHead < 0.0 && !currentSong.displayLines.isEmpty())
        {
            // start from current section's first display line
            bool found = false;
            for (int i = 0; i < currentSong.displayLines.size(); ++i)
            {
                if (currentSong.displayLines[i].sectionIndex == currentSectionIndex)
                {
                    scrollHead = (double)i;
                    found = true;
                    break;
                }
            }
            if (!found)
                scrollHead = 0.0;
        }
    }

    if (onStateChanged) onStateChanged();
}

void SoLyPAudioProcessor::loadSong(const Song& song)
{
    currentSong = song;
    currentSectionIndex = 0;
    scrollHead = -1.0;
    lastTimerUpdate = 0.0;
    transportState = TransportState::Stopped;
    if (onStateChanged) onStateChanged();
}

void SoLyPAudioProcessor::switchToSection(int index)
{
    if (index < 0 || index >= static_cast<int>(currentSong.sections.size()))
        return;

    currentSectionIndex = index;

    if (currentSong.displayLines.isEmpty())
    {
        scrollHead = -1.0;
    }
    else
    {
        bool found = false;
        for (int i = 0; i < currentSong.displayLines.size(); ++i)
        {
            if (currentSong.displayLines[i].sectionIndex == index)
            {
                scrollHead = (double)i;
                found = true;
                break;
            }
        }
        if (!found)
            scrollHead = 0.0;
    }

    if (onStateChanged) onStateChanged();
}

void SoLyPAudioProcessor::switchToNextSection()
{
    int next = currentSectionIndex + 1;
    if (next >= static_cast<int>(currentSong.sections.size()))
    {
        transportState = TransportState::Stopped;
        if (onStateChanged) onStateChanged();
        return;
    }
    switchToSection(next);
}

void SoLyPAudioProcessor::processMidiMessage(const juce::MidiMessage& msg)
{
    if (!msg.isNoteOn())
        return;

    int channel = static_cast<int>(*apvts.getRawParameterValue("midiChannel"));
    if (channel != 0 && msg.getChannel() != channel)
        return;

    lastMidiNote = msg.getNoteNumber();
    lastMidiNoteTime = juce::Time::getMillisecondCounterHiRes();
    if (onStateChanged) onStateChanged();

    midiManager.processNote(msg.getNoteNumber(), [this](MidiManager::Command cmd) {
        switch (cmd)
        {
        case MidiManager::Command::Play:
        case MidiManager::Command::Countdown3:
        case MidiManager::Command::Countdown5:
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
