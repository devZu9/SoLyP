#include "SettingsComponent.h"
#include "Theme.h"
#include "../Data/SettingsManager.h"
#include "../Data/I18n.h"

static juce::ComboBox* createCombo()
{
    auto* cb = new juce::ComboBox();
    cb->setColour(juce::ComboBox::backgroundColourId, Theme::bgPanel);
    cb->setColour(juce::ComboBox::textColourId, Theme::textPrimary);
    cb->setColour(juce::ComboBox::arrowColourId, Theme::textPrimary);
    cb->setColour(juce::ComboBox::buttonColourId, Theme::bgButton);
    cb->setColour(juce::PopupMenu::backgroundColourId, Theme::bgPanel);
    cb->setColour(juce::PopupMenu::textColourId, Theme::textPrimary);
    cb->setColour(juce::PopupMenu::highlightedBackgroundColourId, Theme::bgButton);
    return cb;
}

static juce::Slider* createSlider(double min, double max, double step)
{
    auto* s = new juce::Slider();
    s->setSliderStyle(juce::Slider::LinearHorizontal);
    s->setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    s->setRange(min, max, step);
    s->setColour(juce::Slider::trackColourId, Theme::sliderTrack);
    s->setColour(juce::Slider::thumbColourId, Theme::sliderThumb);
    s->setColour(juce::Slider::backgroundColourId, Theme::bgPanel);
    s->setColour(juce::Slider::textBoxBackgroundColourId, Theme::bgPanel);
    s->setColour(juce::Slider::textBoxTextColourId, Theme::textPrimary);
    s->setColour(juce::Slider::textBoxOutlineColourId, Theme::bgPanel);
    return s;
}

static juce::ToggleButton* createToggle()
{
    auto* tb = new juce::ToggleButton();
    tb->setColour(juce::ToggleButton::tickColourId, Theme::textPrimary);
    tb->setColour(juce::ToggleButton::textColourId, Theme::textPrimary);
    return tb;
}

static juce::Label* addLabel(juce::Component* parent, const juce::String& key)
{
    auto* l = new juce::Label({}, I18n::get(key));
    l->setColour(juce::Label::textColourId, Theme::textPrimary);
    l->setFont(juce::FontOptions(14.0f));
    parent->addAndMakeVisible(l);
    return l;
}

// ── SettingsComponent ────────────────────────────────────────────────────────

SettingsComponent::SettingsComponent()
{
    setSize(600, 700);

    auto* c = new juce::Component();
    content.reset(c);

    auto s = SettingsManager::load();

    int y = 10;
    int pad = 12;
    int colX = 220;
    int ctrlW = 200;
    int rowH = 28;

    // language
    addLabel(c, "settings.language")->setBounds(pad, y, colX - pad, rowH);
    {
        auto* cb = createCombo();
        cb->addItem("Русский", 1);
        cb->addItem("English", 2);
        cb->setSelectedId(s.language == "en" ? 2 : 1);
        cb->onChange = [cb, this] {
            auto st = SettingsManager::load();
            st.language = cb->getSelectedId() == 2 ? "en" : "ru";
            SettingsManager::save(st);
        };
        c->addAndMakeVisible(cb);
        cb->setBounds(colX, y, ctrlW, rowH);
    }
    y += rowH + 6;

    // pre-lines on pause
    addLabel(c, "settings.preLines")->setBounds(pad, y, colX - pad, rowH);
    {
        auto* sl = createSlider(1.0, 5.0, 1.0);
        sl->setValue(s.preLinesOnPause);
        sl->onValueChange = [sl, this] {
            auto st = SettingsManager::load();
            st.preLinesOnPause = (int)sl->getValue();
            SettingsManager::save(st);
        };
        c->addAndMakeVisible(sl);
        sl->setBounds(colX, y, ctrlW, rowH);
    }
    y += rowH + 6;

    // pause text
    addLabel(c, "settings.pauseText")->setBounds(pad, y, colX - pad, rowH);
    {
        auto* te = new juce::TextEditor();
        te->setText(s.pauseText, false);
        te->setColour(juce::TextEditor::backgroundColourId, Theme::bgPanel);
        te->setColour(juce::TextEditor::textColourId, Theme::textPrimary);
        te->setColour(juce::TextEditor::outlineColourId, Theme::bgPanel);
        te->onTextChange = [te, this] {
            auto st = SettingsManager::load();
            st.pauseText = te->getText();
            SettingsManager::save(st);
        };
        c->addAndMakeVisible(te);
        te->setBounds(colX, y, ctrlW, rowH);
    }
    y += rowH + 6;

    // long lines
    addLabel(c, "settings.longLines")->setBounds(pad, y, colX - pad, rowH);
    {
        auto* cb = createCombo();
        cb->addItem(I18n::get("settings.longLinesWrap"), 1);
        cb->addItem(I18n::get("settings.longLinesShrink"), 2);
        cb->setSelectedId(s.longLineBehavior == 1 ? 2 : 1);
        cb->onChange = [cb, this] {
            auto st = SettingsManager::load();
            st.longLineBehavior = cb->getSelectedId() - 1;
            SettingsManager::save(st);
        };
        c->addAndMakeVisible(cb);
        cb->setBounds(colX, y, ctrlW, rowH);
    }
    y += rowH + 10;

    // octave system
    addLabel(c, "settings.octaveSystem")->setBounds(pad, y, colX - pad, rowH);
    {
        auto* cb = createCombo();
        cb->addItem(I18n::get("settings.octaveYamaha"), 1);
        cb->addItem(I18n::get("settings.octaveRoland"), 2);
        cb->addItem(I18n::get("settings.octaveC3"), 3);
        cb->addItem(I18n::get("settings.octaveC4"), 4);
        cb->setSelectedId(s.octaveSystem + 1);
        cb->onChange = [cb, this] {
            auto st = SettingsManager::load();
            st.octaveSystem = cb->getSelectedId() - 1;
            SettingsManager::save(st);
        };
        c->addAndMakeVisible(cb);
        cb->setBounds(colX, y, ctrlW, rowH);
    }
    y += rowH + 6;

    // landmark octave
    addLabel(c, "settings.landmarkOctave")->setBounds(pad, y, colX - pad, rowH);
    {
        auto* sl = createSlider(0.0, 10.0, 1.0);
        sl->setValue(s.landmarkOctave);
        sl->onValueChange = [sl, this] {
            auto st = SettingsManager::load();
            st.landmarkOctave = (int)sl->getValue();
            SettingsManager::save(st);
        };
        c->addAndMakeVisible(sl);
        sl->setBounds(colX, y, ctrlW, rowH);
    }
    y += rowH + 6;

    // trigger octave
    addLabel(c, "settings.triggerOctave")->setBounds(pad, y, colX - pad, rowH);
    {
        auto* sl = createSlider(0.0, 10.0, 1.0);
        sl->setValue(s.triggerOctave);
        sl->onValueChange = [sl, this] {
            auto st = SettingsManager::load();
            st.triggerOctave = (int)sl->getValue();
            SettingsManager::save(st);
        };
        c->addAndMakeVisible(sl);
        sl->setBounds(colX, y, ctrlW, rowH);
    }
    y += rowH + 6;

    // MIDI channel
    addLabel(c, "settings.midiChannel")->setBounds(pad, y, colX - pad, rowH);
    {
        auto* sl = createSlider(0.0, 16.0, 1.0);
        sl->setValue(s.midiChannel);
        sl->onValueChange = [sl, this] {
            auto st = SettingsManager::load();
            st.midiChannel = (int)sl->getValue();
            SettingsManager::save(st);
        };
        c->addAndMakeVisible(sl);
        sl->setBounds(colX, y, ctrlW, rowH);
    }
    y += rowH + 10;

    // manual BPM toggle
    addLabel(c, "settings.manualBpm")->setBounds(pad, y, colX - pad, rowH);
    {
        auto* tg = createToggle();
        tg->setToggleState(s.manualBpmEnabled, juce::dontSendNotification);
        tg->onStateChange = [tg, this] {
            auto st = SettingsManager::load();
            st.manualBpmEnabled = tg->getToggleState();
            SettingsManager::save(st);
        };
        c->addAndMakeVisible(tg);
        tg->setBounds(colX, y, ctrlW, rowH);
    }
    y += rowH + 6;

    // BPM value
    addLabel(c, "settings.manualBpmValue")->setBounds(pad, y, colX - pad, rowH);
    {
        auto* sl = createSlider(20.0, 300.0, 1.0);
        sl->setValue(s.manualBpmValue);
        sl->onValueChange = [sl, this] {
            auto st = SettingsManager::load();
            st.manualBpmValue = (float)sl->getValue();
            SettingsManager::save(st);
        };
        c->addAndMakeVisible(sl);
        sl->setBounds(colX, y, ctrlW, rowH);
    }
    y += rowH + 10;

    // trigger notes
    const char* noteKeys[] = {
        "settings.noteC", "settings.noteCs", "settings.noteD", "settings.noteDs",
        "settings.noteE", "settings.noteF", "settings.noteFs", "settings.noteG",
        "settings.noteGs", "settings.noteA", "settings.noteAs", "settings.noteB"
    };
    struct TrigInfo { const char* labelKey; int* ptr; };
    TrigInfo trigs[] = {
        {"settings.triggerPlay", &s.triggerPlay},
        {"settings.triggerPause", &s.triggerPause},
        {"settings.triggerNext", &s.triggerNextSection},
        {"settings.triggerHybrid", &s.triggerHybrid},
        {"settings.triggerCount3", &s.triggerCountdown3},
        {"settings.triggerCount5", &s.triggerCountdown5}
    };

    for (auto& t : trigs)
    {
        addLabel(c, t.labelKey)->setBounds(pad, y, colX - pad, rowH);
        auto* cb = createCombo();
        for (int i = 0; i < 12; ++i)
            cb->addItem(I18n::get(noteKeys[i]), i + 1);
        cb->setSelectedId(*t.ptr + 1);
        int trigIdx = (int)(&t - trigs);
        cb->onChange = [cb, trigIdx, this] {
            auto st = SettingsManager::load();
            int* targets[] = { &st.triggerPlay, &st.triggerPause, &st.triggerNextSection,
                               &st.triggerHybrid, &st.triggerCountdown3, &st.triggerCountdown5 };
            *targets[trigIdx] = cb->getSelectedId() - 1;
            SettingsManager::save(st);
        };
        c->addAndMakeVisible(cb);
        cb->setBounds(colX, y, ctrlW, rowH);
        y += rowH + 6;
    }

    y += 10;
    content->setSize(getWidth(), y);
    viewport.setViewedComponent(content.get(), false);
    addAndMakeVisible(viewport);

    setSize(500, 400);
}

SettingsComponent::~SettingsComponent() = default;

void SettingsComponent::paint(juce::Graphics& g)
{
    g.fillAll(Theme::bgMain);
}

void SettingsComponent::resized()
{
    viewport.setBounds(getLocalBounds());
}
