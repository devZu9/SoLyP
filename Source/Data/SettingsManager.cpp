#include "SettingsManager.h"

juce::File SettingsManager::getFile()
{
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("SoLyP")
        .getChildFile("settings.json");
}

void SettingsManager::load()
{
    auto file = getFile();
    if (!file.existsAsFile())
        return;

    auto parsed = juce::JSON::parse(file);
    if (!parsed.isObject())
        return;

    auto* obj = parsed.getDynamicObject();
    if (obj == nullptr)
        return;

    auto g = [&](const juce::String& key, auto fallback) -> decltype(fallback) {
        if (obj->hasProperty(key))
            return static_cast<decltype(fallback)>(obj->getProperty(key));
        return fallback;
    };

    visibleLines       = g("visibleLines", 6);
    fontSize           = g("fontSize", 80.0f);
    preLinesOnPause    = g("preLinesOnPause", 1);
    pauseText          = obj->hasProperty("pauseText") ? obj->getProperty("pauseText").toString() : juce::String();
    longLineBehavior   = g("longLineBehavior", 0);
    language           = obj->hasProperty("language") ? obj->getProperty("language").toString() : juce::String("ru");
    themeName          = obj->hasProperty("themeName") ? obj->getProperty("themeName").toString() : juce::String("dark");
    octaveSystem       = g("octaveSystem", 0);
    landmarkOctave     = g("landmarkOctave", 1);
    triggerOctave      = g("triggerOctave", 2);
    midiChannel        = g("midiChannel", 0);
    triggerPlay        = g("triggerPlay", 0);
    triggerPause       = g("triggerPause", 2);
    triggerNextSection = g("triggerNextSection", 4);
    triggerHybrid      = g("triggerHybrid", 5);
    triggerCountdown3  = g("triggerCountdown3", 7);
    triggerCountdown5  = g("triggerCountdown5", 9);
    manualBpmEnabled   = g("manualBpmEnabled", false);
    manualBpmValue     = g("manualBpmValue", 120.0f);
    windowWidth        = g("windowWidth", 0);
    windowHeight       = g("windowHeight", 0);
    windowX            = g("windowX", 0);
    windowY            = g("windowY", 0);
}

void SettingsManager::save()
{
    if (noStartSave) return;
    auto obj = std::make_unique<juce::DynamicObject>();
    obj->setProperty("visibleLines",       visibleLines);
    obj->setProperty("fontSize",           fontSize);
    obj->setProperty("preLinesOnPause",    preLinesOnPause);
    obj->setProperty("pauseText",          pauseText);
    obj->setProperty("longLineBehavior",   longLineBehavior);
    obj->setProperty("language",           language);
    obj->setProperty("themeName",          themeName);
    obj->setProperty("octaveSystem",       octaveSystem);
    obj->setProperty("landmarkOctave",     landmarkOctave);
    obj->setProperty("triggerOctave",      triggerOctave);
    obj->setProperty("midiChannel",        midiChannel);
    obj->setProperty("triggerPlay",        triggerPlay);
    obj->setProperty("triggerPause",       triggerPause);
    obj->setProperty("triggerNextSection", triggerNextSection);
    obj->setProperty("triggerHybrid",      triggerHybrid);
    obj->setProperty("triggerCountdown3",  triggerCountdown3);
    obj->setProperty("triggerCountdown5",  triggerCountdown5);
    obj->setProperty("manualBpmEnabled",   manualBpmEnabled);
    obj->setProperty("manualBpmValue",     manualBpmValue);
    obj->setProperty("windowWidth",        windowWidth);
    obj->setProperty("windowHeight",       windowHeight);
    obj->setProperty("windowX",            windowX);
    obj->setProperty("windowY",            windowY);

    auto json = juce::JSON::toString(juce::var(obj.release()), false);
    auto file = getFile();
    file.getParentDirectory().createDirectory();
    file.replaceWithText(json, false, false, "\n");
}
