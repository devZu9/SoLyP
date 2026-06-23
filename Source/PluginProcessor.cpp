#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Data/SettingsManager.h"

namespace
{
    void logToFile(const juce::String& msg)
    {
        auto file = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
            .getChildFile("SoLyP").getChildFile("debug.log");
        file.appendText(msg + "\n", false, false);
    }
}

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

// обновление настроек из APVTS при изменении параметра (напр. preLines из слайдера)
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

// подготовка аудио (плагин не обрабатывает звук — ничего не делаем)
void SoLyPAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void SoLyPAudioProcessor::releaseResources() {}

// главный аудио-цикл: читает BPM из DAW, автостарт при Play, обрабатывает MIDI-сообщения
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
        }
    }

    for (const auto& metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        if (message.isNoteOn())
        {
            logToFile("[SoLyP] processBlock: note=" + juce::String(message.getNoteNumber())
                + " ch=" + juce::String(message.getChannel()));
            processMidiMessage(message);
        }
    }
}

// таймер GUI (20 Гц): продвигает scrollHead по BPM, останавливает в конце песни
void SoLyPAudioProcessor::timerTick()
{
    double now = juce::Time::getMillisecondCounterHiRes();
    bool isPlaying = (transportState == TransportState::Playing);
    bool isPaused = (transportState == TransportState::Paused);

    if (transportState == TransportState::Countdown && countdownPhase > 0)
    {
        double elapsed = now - countdownPhaseStart;

        if (elapsed >= countdownPhaseDuration)
        {
            countdownPhase++;
            if (countdownPhase > 3)
            {
                countdownPhase = 0;
                pauseLineActive = false;
                showPauseText = true;
                if (pauseLineDisplayIdx >= 0)
                    enterPlay();
                else
                {
                    transportState = TransportState::Stopped;
                    if (onStateChanged) onStateChanged();
                }
                lastTimerUpdate = now;
                return;
            }
            countdownPhaseStart = now;
            if (onStateChanged) onStateChanged();
        }
        if (onStateChanged) onStateChanged();
        // не return — scrollHead продолжает двигаться
    }

    if (scrollHead >= 0.0 && (isPlaying || isPaused || transportState == TransportState::Countdown))
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
            if (isPlaying)
            {
                scrollHead = (double)(currentSong.displayLines.size() - 1);
                transportState = TransportState::Stopped;
                if (onStateChanged) onStateChanged();
            }
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
            if (transportState == TransportState::Countdown && countdownPhase > 0)
                advance *= 3.0;
            double newHead = scrollHead + advance;

            int newIdx = (int)newHead;
            if (newIdx >= currentSong.displayLines.size())
            {
                if (isPlaying)
                {
                    scrollHead = (double)(currentSong.displayLines.size() - 1);
                    transportState = TransportState::Stopped;
                    if (onStateChanged) onStateChanged();
                }
                else
                {
                    scrollHead = (double)(currentSong.displayLines.size() - 1);
                }
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

// переход в режим воспроизведения: из паузы — возобновление с пред-строк, иначе — старт с начала секции
void SoLyPAudioProcessor::enterPlay()
{
    logToFile("[SoLyP] enterPlay() called from state=" + juce::String((int)transportState)
        + " pauseLineDisplayIdx=" + juce::String(pauseLineDisplayIdx));

    pauseLineActive = false;

    if ((transportState == TransportState::Paused || transportState == TransportState::Countdown)
        && pauseLineDisplayIdx >= 0)
    {
        resumeSkipIdx = pauseLineDisplayIdx;
        scrollHead = (double)pauseLineDisplayIdx;
        lastTimerUpdate = juce::Time::getMillisecondCounterHiRes();
    }
    else
    {
        resumeSkipIdx = 0;
        lastTimerUpdate = juce::Time::getMillisecondCounterHiRes();
        if (scrollHead < 0.0 && !currentSong.displayLines.isEmpty())
        {
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

    transportState = TransportState::Playing;
    if (onStateChanged) onStateChanged();
}

// переход в режим паузы: из Stopped — показывает пред-строки начала секции; из Playing — плавная анимация пауза-сообщения
void SoLyPAudioProcessor::enterPause()
{
    if (transportState == TransportState::Paused)
        return;

    pauseLineActive = true;

    if (transportState == TransportState::Stopped)
    {
        showPauseText = false;
        if (scrollHead < 0.0)
        {
            int firstIdx = 0;
            for (int i = 0; i < currentSong.displayLines.size(); ++i)
            {
                if (currentSong.displayLines[i].sectionIndex == currentSectionIndex)
                {
                    firstIdx = i;
                    break;
                }
            }
            pauseLineDisplayIdx = firstIdx;
            scrollHead = (double)(firstIdx + SettingsManager::visibleLines + preLinesOnPause);
        }
        else
        {
            pauseLineDisplayIdx = (int)scrollHead;
            scrollHead = (double)((int)scrollHead + SettingsManager::visibleLines + preLinesOnPause);
        }
    }
    else
    {
        showPauseText = true;
        int offset = preLinesOnPause;
        if (scrollHead >= 0.0 && (int)(scrollHead + offset) < currentSong.displayLines.size())
            pauseLineDisplayIdx = (int)(scrollHead + offset) + 1;
        else
            pauseLineDisplayIdx = -1;
    }

    transportState = TransportState::Paused;
    if (onStateChanged) onStateChanged();
}

// переход в режим останова: замораживает экран, не меняет scrollHead и pauseLineActive
void SoLyPAudioProcessor::enterStop()
{
    if (transportState == TransportState::Stopped)
        return;

    transportState = TransportState::Stopped;
    if (onStateChanged) onStateChanged();
}

// переход в режим обратного отсчёта (из Stopped/Paused): готовит pre-lines, запускает 3-2-1
void SoLyPAudioProcessor::enterCountdown()
{
    if (transportState == TransportState::Playing
        || transportState == TransportState::Countdown)
        return;

    if (transportState == TransportState::Stopped)
    {
        showPauseText = false;
        if (scrollHead < 0.0)
        {
            int firstIdx = 0;
            for (int i = 0; i < currentSong.displayLines.size(); ++i)
            {
                if (currentSong.displayLines[i].sectionIndex == currentSectionIndex)
                {
                    firstIdx = i;
                    break;
                }
            }
            pauseLineDisplayIdx = firstIdx;
            scrollHead = (double)(firstIdx + SettingsManager::visibleLines + preLinesOnPause);
        }
        else
        {
            pauseLineDisplayIdx = (int)scrollHead;
            scrollHead = (double)((int)scrollHead + SettingsManager::visibleLines + preLinesOnPause);
        }
    }
    else // Paused — keep showPauseText, pauseLineDisplayIdx and scrollHead unchanged
    {
    }
    pauseLineActive = true;

    double bpm = SettingsManager::manualBpmEnabled
        ? (double)SettingsManager::manualBpmValue : currentBpm;
    if (bpm <= 0.0) bpm = 120.0;
    countdownPhaseDuration = 0.5 * (60000.0 / bpm * (double)SettingsManager::timeSignature);

    countdownPhase = 1;
    countdownPhaseStart = juce::Time::getMillisecondCounterHiRes();

    transportState = TransportState::Countdown;
    if (onStateChanged) onStateChanged();
}

void SoLyPAudioProcessor::loadSong(const Song& song)
{
    currentSong = song;
    resumeSkipIdx = 0;
    pauseLineActive = false;
    scrollHead = -1.0;
    currentSectionIndex = 0;
    lastTimerUpdate = 0.0;
    transportState = TransportState::Stopped;
    if (onStateChanged) onStateChanged();
}

// переключение на секцию по индексу: ставит scrollHead на её начало, во время паузы — обновляет pre-lines
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

    if (pauseLineActive && transportState == TransportState::Paused)
    {
        int sectionStart = (int)scrollHead;
        pauseLineDisplayIdx = sectionStart;
        scrollHead = (double)(sectionStart + SettingsManager::visibleLines + preLinesOnPause);
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

// обработка входящей MIDI-ноты: проверка октавы/канала, вызов MidiManager, переходы enter*()
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
        logToFile("[SoLyP] cmd=" + juce::String((int)cmd) + " state=" + juce::String((int)transportState));
        switch (cmd)
        {
        case MidiManager::Command::Play:
            if (transportState != TransportState::Countdown)
                enterPlay();
            break;
        case MidiManager::Command::Countdown3:
            if (transportState != TransportState::Countdown)
                enterCountdown();
            break;
        case MidiManager::Command::Stop:
            if (transportState != TransportState::Countdown)
                enterStop();
            break;
        case MidiManager::Command::Pause:
            if (transportState != TransportState::Countdown)
                enterPause();
            break;
        case MidiManager::Command::NextSection:
            switchToNextSection();
            break;
        case MidiManager::Command::Hybrid:
            switchToNextSection();
            enterPlay();
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

// сохранение состояния APVTS (BPM, канал, preLines) в бинарный блок для DAW
void SoLyPAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

// загрузка состояния APVTS из бинарного блока (восстановление после перезапуска DAW)
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
