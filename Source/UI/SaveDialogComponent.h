#pragma once

#include <JuceHeader.h>

class SaveDialogComponent : public juce::Component,
                            private juce::TextEditor::Listener
{
public:
    SaveDialogComponent(const juce::String& initialFilename,
                        const juce::String& initialSongTitle,
                        std::function<void(const juce::String&, const juce::String&)> onSave_,
                        std::function<void()> onCancel_);
    ~SaveDialogComponent() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;

    void activateModal(std::function<void()> onClose);
    void textEditorEscapeKeyPressed(juce::TextEditor&) override;

private:
    juce::TextEditor filenameField;
    juce::TextEditor songnameField;
    juce::TextButton saveBtn{ "" };
    juce::TextButton cancelBtn{ "" };
    juce::TextButton closeBtn{ "" };
};
