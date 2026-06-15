#include "SettingsManager.h"

juce::File SettingsManager::getFile()
{
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("SoLyP")
        .getChildFile("settings.json");
}

Settings SettingsManager::load()
{
    Settings s;
    auto file = getFile();
    if (!file.existsAsFile())
        return s;

    auto parsed = juce::JSON::parse(file);
    if (!parsed.isObject())
        return s;

    auto* obj = parsed.getDynamicObject();
    if (obj == nullptr)
        return s;

    auto g = [&](const juce::String& key, auto fallback) -> decltype(fallback) {
        if (obj->hasProperty(key))
            return static_cast<decltype(fallback)>(obj->getProperty(key));
        return fallback;
    };

    s.visibleLines = g("visibleLines", 6);
    s.fontSize = g("fontSize", 80.0f);
    s.preLinesOnPause = g("preLinesOnPause", 1);
    s.pauseText = obj->hasProperty("pauseText") ? obj->getProperty("pauseText").toString() : juce::String();
    s.longLineBehavior = g("longLineBehavior", 0);
    s.language = obj->hasProperty("language") ? obj->getProperty("language").toString() : juce::String("ru");
    s.themeName = obj->hasProperty("themeName") ? obj->getProperty("themeName").toString() : juce::String("dark");
    s.octaveSystem = g("octaveSystem", 0);
    s.landmarkOctave = g("landmarkOctave", 1);
    s.triggerOctave = g("triggerOctave", 2);
    s.midiChannel = g("midiChannel", 0);
    s.triggerPlay = g("triggerPlay", 0);
    s.triggerPause = g("triggerPause", 2);
    s.triggerNextSection = g("triggerNextSection", 4);
    s.triggerHybrid = g("triggerHybrid", 5);
    s.triggerCountdown3 = g("triggerCountdown3", 7);
    s.triggerCountdown5 = g("triggerCountdown5", 9);
    s.manualBpmEnabled = g("manualBpmEnabled", false);
    s.manualBpmValue = g("manualBpmValue", 120.0f);
    s.windowWidth = g("windowWidth", 0);
    s.windowHeight = g("windowHeight", 0);
    s.windowX = g("windowX", 0);
    s.windowY = g("windowY", 0);

    return s;
}

void SettingsManager::save(const Settings& s)
{
    auto obj = std::make_unique<juce::DynamicObject>();
    obj->setProperty("visibleLines", s.visibleLines);
    obj->setProperty("fontSize", s.fontSize);
    obj->setProperty("preLinesOnPause", s.preLinesOnPause);
    obj->setProperty("pauseText", s.pauseText);
    obj->setProperty("longLineBehavior", s.longLineBehavior);
    obj->setProperty("language", s.language);
    obj->setProperty("themeName", s.themeName);
    obj->setProperty("octaveSystem", s.octaveSystem);
    obj->setProperty("landmarkOctave", s.landmarkOctave);
    obj->setProperty("triggerOctave", s.triggerOctave);
    obj->setProperty("midiChannel", s.midiChannel);
    obj->setProperty("triggerPlay", s.triggerPlay);
    obj->setProperty("triggerPause", s.triggerPause);
    obj->setProperty("triggerNextSection", s.triggerNextSection);
    obj->setProperty("triggerHybrid", s.triggerHybrid);
    obj->setProperty("triggerCountdown3", s.triggerCountdown3);
    obj->setProperty("triggerCountdown5", s.triggerCountdown5);
    obj->setProperty("manualBpmEnabled", s.manualBpmEnabled);
    obj->setProperty("manualBpmValue", s.manualBpmValue);
    obj->setProperty("windowWidth", s.windowWidth);
    obj->setProperty("windowHeight", s.windowHeight);
    obj->setProperty("windowX", s.windowX);
    obj->setProperty("windowY", s.windowY);

    auto json = juce::JSON::toString(juce::var(obj.release()), false);

    auto file = getFile();
    file.getParentDirectory().createDirectory();
    file.replaceWithText(json, false, false, "\n");
}
