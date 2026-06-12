#include "MidiManager.h"

MidiManager::MidiManager() {}

bool MidiManager::isInOctave(int midiNote, int octave) const
{
    int noteOctave = midiNote / 12;
    return noteOctave == octave;
}

int MidiManager::getNoteInOctave(int midiNote, int octave) const
{
    if (!isInOctave(midiNote, octave))
        return -1;
    return midiNote % 12;
}

void MidiManager::processNote(int midiNote, std::function<void(Command)> callback)
{
    if (isInOctave(midiNote, config.landmarkOctave))
    {
        auto noteInOctave = midiNote % 12;
        lastLandmarkSection = noteInOctave;
        callback(Command::Landmark);
        return;
    }

    if (isInOctave(midiNote, config.triggerOctave))
    {
        auto noteInOctave = midiNote % 12;

        if (noteInOctave == config.playNote)
            callback(Command::Play);
        else if (noteInOctave == config.pauseNote)
            callback(Command::Pause);
        else if (noteInOctave == config.nextSectionNote)
            callback(Command::NextSection);
        else if (noteInOctave == config.hybridNote)
            callback(Command::Hybrid);
        else if (noteInOctave == config.countdown3Note)
            callback(Command::Countdown3);
        else if (noteInOctave == config.countdown5Note)
            callback(Command::Countdown5);
    }
}
