#include "Logic.h"
#include "Display.h"
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
    :       AudioProcessor(BusesProperties()
                    .withInput("Input", juce::AudioChannelSet::stereo(), true)
                    .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
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
bool SoLyPAudioProcessor::producesMidi() const { return true; }
bool SoLyPAudioProcessor::isMidiEffect() const { return false; }
double SoLyPAudioProcessor::getTailLengthSeconds() const { return 0.0; }

bool SoLyPAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

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

    auto head = getPlayHead();
    if (head)
    {
        auto opt = head->getPosition();
        if (opt.hasValue())
        {
            auto& pos = *opt;
            if (pos.getBpm().hasValue())
                currentBpm = *pos.getBpm();
        }
    }

    for (const auto& metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        if (message.isNoteOn())
            processMidiMessage(message);
    }
}

void SoLyPAudioProcessor::switchPlay()
{
    if (transportState == TransportState::Countdown) return;
    transportState = TransportState::Playing;
    if (onStateChanged) onStateChanged();
}

void SoLyPAudioProcessor::switchPause()
{
    if (transportState == TransportState::Paused
        || transportState == TransportState::Stopped) return;
    transportState = TransportState::Paused;
    if (onStateChanged) onStateChanged();
}

void SoLyPAudioProcessor::switchStop()
{
    if (transportState == TransportState::Stopped) return;
    transportState = TransportState::Stopped;
    if (onStateChanged) onStateChanged();
}

void SoLyPAudioProcessor::switchCountdown()
{
    if (transportState == TransportState::Playing
        || transportState == TransportState::Countdown) return;
    transportState = TransportState::Countdown;
    if (onStateChanged) onStateChanged();
}

void SoLyPAudioProcessor::switchHybrid()
{
    if (transportState == TransportState::Countdown) return;
    switchToNextSection();
    transportState = TransportState::Playing;
    if (onStateChanged) onStateChanged();
}

void SoLyPAudioProcessor::switchLandmark()
{
    if (transportState == TransportState::Countdown) return;
    int sectionIndex = midiManager.getLastLandmarkSection();
    if (sectionIndex >= 0) switchToSection(sectionIndex);
}

void SoLyPAudioProcessor::loadSong(const Song& song)
{
    currentSong = song;
    transportState = TransportState::Stopped;
    if (onStateChanged) onStateChanged();
}

void SoLyPAudioProcessor::switchToSection(int index)
{
    sectionTarget = index;
    if (onStateChanged) onStateChanged();
}

void SoLyPAudioProcessor::switchToNextSection()
{
    sectionTarget = -2;  // magic: +1 from current
    if (onStateChanged) onStateChanged();
}

void SoLyPAudioProcessor::processMidiMessage(const juce::MidiMessage& msg)
{
    if (!msg.isNoteOn()) return;

    int channel = static_cast<int>(*apvts.getRawParameterValue("midiChannel"));
    if (channel != 0 && msg.getChannel() != channel) return;

    lastMidiNote = msg.getNoteNumber();
    lastMidiNoteTime = juce::Time::getMillisecondCounterHiRes();
    if (onStateChanged) onStateChanged();

    midiManager.processNote(msg.getNoteNumber(), [this](MidiManager::Command cmd) {
        switch (cmd)
        {
        case MidiManager::Command::Play:
            switchPlay();
            break;
        case MidiManager::Command::Countdown3:
            switchCountdown();
            break;
        case MidiManager::Command::Stop:
            switchStop();
            break;
        case MidiManager::Command::Pause:
            switchPause();
            break;
        case MidiManager::Command::NextSection:
            switchToNextSection();
            break;
        case MidiManager::Command::Hybrid:
            switchHybrid();
            break;
        case MidiManager::Command::Landmark:
            switchLandmark();
            break;
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
