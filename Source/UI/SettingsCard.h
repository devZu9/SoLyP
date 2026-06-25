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
    juce::DrawableButton* addInfoIcon();
    juce::TextButton* addButton(const juce::String& labelKey);
    juce::TextEditor* addEditor(const juce::String& labelKey, int height = 60);
    void addRadioPair(const juce::String& labelKey, const juce::String& opt1, const juce::String& opt2,
                      juce::ToggleButton*& out1, juce::ToggleButton*& out2);
    void addImagePair(const juce::String& labelKey, const juce::Drawable* img1, const juce::Drawable* img2,
                      juce::DrawableButton*& out1, juce::DrawableButton*& out2);
    void addSeparator();
    juce::Label* getLastLabel();
    int getPreferredHeight() const;

private:
    juce::Label title;
    struct Row {
        std::unique_ptr<juce::Label> label;
        juce::Component* control = nullptr;
        juce::Component* control2 = nullptr;
        bool isPair = false;
        bool isInfoPair = false;
        bool isSeparator = false;
        int rowHeight = 28;
    };
    std::vector<Row> rows;
    std::unique_ptr<juce::LookAndFeel> editorBorderLAF;
};
