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

    if (obj->hasProperty("visibleLines"))
        s.visibleLines = static_cast<int>(obj->getProperty("visibleLines"));

    if (obj->hasProperty("fontSize"))
        s.fontSize = static_cast<float>(obj->getProperty("fontSize"));

    return s;
}

void SettingsManager::save(const Settings& s)
{
    auto obj = std::make_unique<juce::DynamicObject>();
    obj->setProperty("visibleLines", s.visibleLines);
    obj->setProperty("fontSize", s.fontSize);

    auto json = juce::JSON::toString(juce::var(obj.release()), false);

    auto file = getFile();
    file.getParentDirectory().createDirectory();
    file.replaceWithText(json, false, false, "\n");
}
