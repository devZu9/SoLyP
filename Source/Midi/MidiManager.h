#pragma once

#include <JuceHeader.h>

class MidiManager
{
public:
    enum class Command
    {
        None,
        Play,
        Pause,
        NextSection,
        Hybrid,
        Countdown3,
        Stop,
        Landmark
    };

    struct Configuration
    {
        int landmarkOctave = 1;
        int triggerOctave = 2;
        int playNote = 0;     // C within trigger octave
        int pauseNote = 2;    // D
        int nextSectionNote = 4; // E
        int hybridNote = 5;   // F
        int countdown3Note = 7; // G
    };

    MidiManager();
    ~MidiManager() = default;

    void processNote(int midiNote, std::function<void(Command)> callback);
    int getLastLandmarkSection() const { return lastLandmarkSection; }

    Configuration& getConfig() { return config; }
    void setConfig(const Configuration& cfg) { config = cfg; }

private:
    bool isInOctave(int midiNote, int octave) const;
    int getNoteInOctave(int midiNote, int octave) const;

    Configuration config;
    int lastLandmarkSection = -1;
};
