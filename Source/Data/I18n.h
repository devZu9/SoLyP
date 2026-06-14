#pragma once

#include <JuceHeader.h>

namespace I18n
{
    void setLanguage(const juce::String& lang);
    juce::String get(const juce::String& key);
    juce::String getCurrentLang();
}
