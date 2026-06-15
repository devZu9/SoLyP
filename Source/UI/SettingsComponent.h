#pragma once

#include <JuceHeader.h>

class SettingsComponent : public juce::Component
{
public:
    SettingsComponent();
    ~SettingsComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    juce::Viewport viewport;
    std::unique_ptr<juce::Component> content;
};
