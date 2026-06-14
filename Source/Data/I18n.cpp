#include "I18n.h"
#include <map>

namespace
{
    std::map<juce::String, juce::String> strings;
    juce::String currentLang = "ru";

    juce::File getFile(const juce::String& lang)
    {
        return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
            .getChildFile("SoLyP").getChildFile("langs")
            .getChildFile(lang + ".json");
    }

    void loadFromJson(const juce::String& jsonText)
    {
        strings.clear();
        auto parsed = juce::JSON::parse(jsonText);
        if (!parsed.isObject()) return;
        auto* obj = parsed.getDynamicObject();
        if (obj == nullptr) return;

        for (auto& [key, value] : obj->getProperties())
            strings[key.toString()] = value.toString();
    }

    void tryLoad(const juce::String& lang)
    {
        auto file = getFile(lang);
        if (file.existsAsFile())
        {
            loadFromJson(file.loadFileAsString());
            return;
        }

        // try to copy from langs/ next to working directory (dev)
        auto src = juce::File::getCurrentWorkingDirectory()
            .getChildFile("langs").getChildFile(lang + ".json");
        if (src.existsAsFile())
        {
            file.getParentDirectory().createDirectory();
            src.copyFileTo(file);
            loadFromJson(file.loadFileAsString());
        }
    }
}

void I18n::setLanguage(const juce::String& lang)
{
    currentLang = lang;
    tryLoad(lang);
}

juce::String I18n::get(const juce::String& key)
{
    if (strings.empty())
        setLanguage(currentLang);

    auto it = strings.find(key);
    if (it != strings.end())
        return it->second;

    return "[" + key + "]";
}

juce::String I18n::getCurrentLang()
{
    return currentLang;
}
