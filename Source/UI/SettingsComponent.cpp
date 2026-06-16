#include "SettingsComponent.h"
#include "SettingsCard.h"
#include "Theme.h"
#include "../Data/SettingsManager.h"
#include "../Data/I18n.h"

namespace
{
    struct LabelForwarder : juce::MouseListener
    {
        std::function<void()> onClick;
        std::function<void()> onDoubleClick;
        LabelForwarder(std::function<void()> c, std::function<void()> dc)
            : onClick(c), onDoubleClick(dc) {}
        void mouseDown(const juce::MouseEvent&) override { if (onClick) onClick(); }
        void mouseDoubleClick(const juce::MouseEvent&) override { if (onDoubleClick) onDoubleClick(); }
    };
}

SettingsComponent::SettingsComponent(std::function<void()> onLangChanged)
{
    auto* container = new juce::Component();
    gridContainer.reset(container);
    viewport.setViewedComponent(container, false);
    viewport.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport);

    auto s = SettingsManager::load();

    // ── card 1: General ──
    {
        auto* card = new SettingsCard(I18n::get("settings.title"));
        auto* langCb = card->addCombo(I18n::get("settings.language"));
        langCb->addItem(juce::String::fromUTF8("Русский"), 1);
        langCb->addItem("English", 2);
        langCb->onChange = [langCb, onLangChanged] {
            auto st = SettingsManager::load();
            st.language = langCb->getSelectedId() == 2 ? "en" : "ru";
            SettingsManager::save(st);
            I18n::setLanguage(st.language);
            if (onLangChanged) onLangChanged();
        };
        langCb->setSelectedId(s.language == "en" ? 2 : 1, juce::dontSendNotification);

        // separator before BPM section
        card->addSeparator();

        // BPM controls on General card
        auto* bpmTg = card->addToggle(I18n::get("settings.manualBpm"));
        bpmTg->setToggleState(s.manualBpmEnabled, juce::dontSendNotification);
        auto* bpmLabel = card->getLastLabel();

        auto* bpvSl = card->addSlider(I18n::get("settings.manualBpmValue"), 20.0, 300.0, 1.0);
        bpvSl->setValue((double)s.manualBpmValue);
        bpvSl->setVelocityBasedMode(true);
        bpvSl->setDoubleClickReturnValue(true, 120.0);
        bpvSl->onValueChange = [bpvSl] {
            auto st = SettingsManager::load();
            st.manualBpmValue = (float)bpvSl->getValue();
            SettingsManager::save(st);
        };

        auto* bpvLabel = card->getLastLabel();

        // toggle label click → toggle the button
        if (bpmLabel != nullptr)
            bpmLabel->addMouseListener(new LabelForwarder(
                [bpmTg] { bpmTg->triggerClick(); },
                nullptr
            ), false);

        // BPM label double-click → reset slider to 120
        if (bpvLabel != nullptr)
            bpvLabel->addMouseListener(new LabelForwarder(
                nullptr,
                [bpvSl] { bpvSl->setValue(120.0); }
            ), false);

        // apply initial dim state
        float initAlpha = s.manualBpmEnabled ? 1.0f : 0.4f;
        bpvSl->setEnabled(s.manualBpmEnabled);
        bpvSl->setAlpha(initAlpha);
        if (bpvLabel != nullptr)
            bpvLabel->setAlpha(initAlpha);

        bpmTg->onStateChange = [bpmTg, bpvSl, bpvLabel] {
            bool on = bpmTg->getToggleState();
            float a = on ? 1.0f : 0.4f;
            bpvSl->setEnabled(on);
            bpvSl->setAlpha(a);
            if (bpvLabel != nullptr)
                bpvLabel->setAlpha(a);
            auto st = SettingsManager::load();
            st.manualBpmEnabled = on;
            SettingsManager::save(st);
        };
        container->addAndMakeVisible(card);
    }

    // ── card 2: Display ──
    {
        auto* card = new SettingsCard(I18n::get("settings.longLines"));
        auto* preSl = card->addSlider(I18n::get("settings.preLines"), 1.0, 5.0, 1.0);
        preSl->setValue((double)s.preLinesOnPause);
        preSl->onValueChange = [preSl] {
            auto st = SettingsManager::load();
            st.preLinesOnPause = (int)preSl->getValue();
            SettingsManager::save(st);
        };
        auto* longCb = card->addCombo(I18n::get("settings.longLines"));
        longCb->addItem(I18n::get("settings.longLinesWrap"), 1);
        longCb->addItem(I18n::get("settings.longLinesShrink"), 2);
        longCb->setSelectedId(s.longLineBehavior == 1 ? 2 : 1);
        longCb->onChange = [longCb] {
            auto st = SettingsManager::load();
            st.longLineBehavior = longCb->getSelectedId() - 1;
            SettingsManager::save(st);
        };
        container->addAndMakeVisible(card);
    }

    // ── card 3: MIDI ──
    {
        auto* card = new SettingsCard("MIDI");
        auto* octCb = card->addCombo(I18n::get("settings.octaveSystem"));
        octCb->addItem(I18n::get("settings.octaveYamaha"), 1);
        octCb->addItem(I18n::get("settings.octaveRoland"), 2);
        octCb->addItem(I18n::get("settings.octaveC3"), 3);
        octCb->addItem(I18n::get("settings.octaveC4"), 4);
        octCb->setSelectedId(s.octaveSystem + 1);
        octCb->onChange = [octCb] {
            auto st = SettingsManager::load();
            st.octaveSystem = octCb->getSelectedId() - 1;
            SettingsManager::save(st);
        };

        auto* lmSl = card->addSlider(I18n::get("settings.landmarkOctave"), 0.0, 10.0, 1.0);
        lmSl->setValue((double)s.landmarkOctave);
        lmSl->onValueChange = [lmSl] {
            auto st = SettingsManager::load();
            st.landmarkOctave = (int)lmSl->getValue();
            SettingsManager::save(st);
        };

        auto* trSl = card->addSlider(I18n::get("settings.triggerOctave"), 0.0, 10.0, 1.0);
        trSl->setValue((double)s.triggerOctave);
        trSl->onValueChange = [trSl] {
            auto st = SettingsManager::load();
            st.triggerOctave = (int)trSl->getValue();
            SettingsManager::save(st);
        };

        auto* chSl = card->addSlider(I18n::get("settings.midiChannel"), 0.0, 16.0, 1.0);
        chSl->setValue((double)s.midiChannel);
        chSl->onValueChange = [chSl] {
            auto st = SettingsManager::load();
            st.midiChannel = (int)chSl->getValue();
            SettingsManager::save(st);
        };
        container->addAndMakeVisible(card);
    }

    // ── card 4: Triggers ──
    {
        auto* card = new SettingsCard("Triggers");
        const char* noteKeys[] = {
            "settings.noteC", "settings.noteCs", "settings.noteD", "settings.noteDs",
            "settings.noteE", "settings.noteF", "settings.noteFs", "settings.noteG",
            "settings.noteGs", "settings.noteA", "settings.noteAs", "settings.noteB"
        };
        struct { const char* key; int* ptr; } trigs[] = {
            { "settings.triggerPlay", &s.triggerPlay },
            { "settings.triggerPause", &s.triggerPause },
            { "settings.triggerNext", &s.triggerNextSection },
            { "settings.triggerHybrid", &s.triggerHybrid },
            { "settings.triggerCount3", &s.triggerCountdown3 },
            { "settings.triggerCount5", &s.triggerCountdown5 }
        };
        int comboIdx = 0;
        for (auto& t : trigs)
        {
            auto* cb = card->addCombo(I18n::get(t.key));
            for (int i = 0; i < 12; ++i)
                cb->addItem(I18n::get(noteKeys[i]), i + 1);
            cb->setSelectedId(*t.ptr + 1);
            cb->onChange = [cb, comboIdx] {
                auto st = SettingsManager::load();
                int* targets[] = { &st.triggerPlay, &st.triggerPause, &st.triggerNextSection,
                                   &st.triggerHybrid, &st.triggerCountdown3, &st.triggerCountdown5 };
                *targets[comboIdx] = cb->getSelectedId() - 1;
                SettingsManager::save(st);
            };
            ++comboIdx;
        }
        container->addAndMakeVisible(card);
    }

    setSize(600, 400);
}

void SettingsComponent::paint(juce::Graphics& g)
{
    g.fillAll(Theme::bgMain);
}

void SettingsComponent::resized()
{
    auto area = getLocalBounds();
    viewport.setBounds(area);

    auto children = gridContainer->getChildren();
    if (children.isEmpty())
        return;

    int cols = 1;
    if (area.getWidth() >= 750) cols = 2;
    if (area.getWidth() >= 1050) cols = 3;

    juce::Grid grid;
    for (int i = 0; i < cols; ++i)
        grid.templateColumns.add(juce::Grid::TrackInfo(1_fr));
    grid.setGap(24_px);

    for (auto* child : children)
        grid.items.add(juce::GridItem(*child));

    int pad = 16;
    auto gridBounds = area.reduced(pad, pad).withY(pad);

    // auto-rows: use the tallest card's preferred height
    int cardH = 200;
    for (auto* child : children)
        if (auto* card = dynamic_cast<SettingsCard*>(child))
            cardH = juce::jmax(cardH, card->getPreferredHeight());
    grid.autoRows = juce::Grid::TrackInfo(juce::Grid::Px((float)cardH));

    grid.performLayout(gridBounds);

    float maxBottom = 0.0f;
    for (auto* child : children)
        maxBottom = juce::jmax(maxBottom, (float)child->getBottom());
    gridContainer->setSize(area.getWidth(), (int)maxBottom + pad);
}
