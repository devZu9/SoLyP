#pragma once

#include <JuceHeader.h>

class SettingsCard : public juce::Component
{
public:
    SettingsCard(const juce::String& titleText);
    void paint(juce::Graphics&) override;
    void resized() override;

    juce::ComboBox* addCombo(const juce::String& labelKey);
    juce::Slider*   addSlider(const juce::String& labelKey, double min, double max, double step);
    juce::ToggleButton* addToggle(const juce::String& labelKey);
    void addSeparator();
    juce::Label* getLastLabel();
    int getPreferredHeight() const;

private:
    juce::Label title;
    struct Row {
        std::unique_ptr<juce::Label> label;
        juce::Component* control = nullptr;
        bool isSeparator = false;
    };
    std::vector<Row> rows;
};
