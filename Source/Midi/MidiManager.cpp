#include "MidiManager.h"
#include "../Data/SettingsManager.h"

MidiManager::MidiManager() {}

bool MidiManager::isInOctave(int midiNote, int octaveParam) const
{
    int noteOctave = midiNote / 12 - 1 - SettingsManager::octaveSystem;
    return noteOctave == octaveParam;
}

int MidiManager::getNoteInOctave(int midiNote, int octave) const
{
    if (!isInOctave(midiNote, octave))
        return -1;
    return midiNote % 12;
}

void MidiManager::processNote(int midiNote, std::function<void(Command)> callback)
{
    if (isInOctave(midiNote, SettingsManager::landmarkOctave))
    {
        auto noteInOctave = midiNote % 12;
        lastLandmarkSection = noteInOctave;
        callback(Command::Landmark);
        return;
    }

    if (isInOctave(midiNote, SettingsManager::triggerOctave))
    {
        auto noteInOctave = midiNote % 12;

        if (noteInOctave == SettingsManager::triggerPlay)
            callback(Command::Play);
        else if (noteInOctave == SettingsManager::triggerPause)
            callback(Command::Pause);
        else if (noteInOctave == SettingsManager::triggerNextSection)
            callback(Command::NextSection);
        else if (noteInOctave == SettingsManager::triggerHybrid)
            callback(Command::Hybrid);
        else if (noteInOctave == SettingsManager::triggerCountdown3)
            callback(Command::Countdown3);
        else if (noteInOctave == SettingsManager::triggerStop)
            callback(Command::Stop);
    }
}
