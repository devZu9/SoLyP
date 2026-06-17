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

    // ── card 1: General ──
    {
        auto* card = new SettingsCard(I18n::get("settings.title"));
        auto* langCb = card->addCombo(I18n::get("settings.language"));
        langCb->addItem(juce::String::fromUTF8("Русский"), 1);
        langCb->addItem("English", 2);
        langCb->onChange = [langCb, onLangChanged] {
            SettingsManager::language = langCb->getSelectedId() == 2 ? "en" : "ru";
            I18n::setLanguage(SettingsManager::language);
            SettingsManager::save();
            if (onLangChanged) onLangChanged();
        };
        langCb->setSelectedId(I18n::getCurrentLang().equalsIgnoreCase("en") ? 2 : 1, juce::dontSendNotification);

        // separator before BPM section
        card->addSeparator();

        // BPM controls
        auto* bpmTg = card->addToggle(I18n::get("settings.manualBpm"));
        bpmTg->setToggleState(SettingsManager::manualBpmEnabled, juce::dontSendNotification);

        auto* bpvSl = card->addSlider(I18n::get("settings.manualBpmValue"), 20.0, 300.0, 1.0);
        bpvSl->setValue((double)SettingsManager::manualBpmValue);
        bpvSl->setVelocityBasedMode(true);
        bpvSl->setDoubleClickReturnValue(true, 120.0);
        bpvSl->onValueChange = [bpvSl] {
            SettingsManager::manualBpmValue = (float)bpvSl->getValue();
            SettingsManager::save();
        };

        auto* bpmLabel = card->getLastLabel();

        // dimming
        float initAlpha = SettingsManager::manualBpmEnabled ? 1.0f : 0.4f;
        bpvSl->setEnabled(SettingsManager::manualBpmEnabled);
        bpvSl->setAlpha(initAlpha);
        if (bpmLabel != nullptr)
            bpmLabel->setAlpha(initAlpha);

        bpmTg->onStateChange = [bpmTg, bpvSl, bpmLabel] {
            bool on = bpmTg->getToggleState();
            float a = on ? 1.0f : 0.4f;
            SettingsManager::manualBpmEnabled = on;
            bpvSl->setEnabled(on);
            bpvSl->setAlpha(a);
            if (bpmLabel != nullptr)
                bpmLabel->setAlpha(a);
            SettingsManager::save();
        };

        container->addAndMakeVisible(card);
    }

    // ── card 2: Display ──
    {
        auto* card = new SettingsCard(I18n::get("settings.longLines"));
        auto* preSl = card->addSlider(I18n::get("settings.preLines"), 1.0, 5.0, 1.0);
        preSl->setValue((double)SettingsManager::preLinesOnPause);
        preSl->onValueChange = [preSl] {
            SettingsManager::preLinesOnPause = (int)preSl->getValue();
            SettingsManager::save();
        };
        auto* longCb = card->addCombo(I18n::get("settings.longLines"));
        longCb->addItem(I18n::get("settings.longLinesWrap"), 1);
        longCb->addItem(I18n::get("settings.longLinesShrink"), 2);
        longCb->setSelectedId(SettingsManager::longLineBehavior == 1 ? 2 : 1);
        longCb->onChange = [longCb] {
            SettingsManager::longLineBehavior = longCb->getSelectedId() - 1;
            SettingsManager::save();
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
        octCb->setSelectedId(SettingsManager::octaveSystem + 1);
        octCb->onChange = [octCb] {
            SettingsManager::octaveSystem = octCb->getSelectedId() - 1;
            SettingsManager::save();
        };

        auto* lmSl = card->addSlider(I18n::get("settings.landmarkOctave"), 0.0, 10.0, 1.0);
        lmSl->setValue((double)SettingsManager::landmarkOctave);
        lmSl->onValueChange = [lmSl] {
            SettingsManager::landmarkOctave = (int)lmSl->getValue();
            SettingsManager::save();
        };

        auto* trSl = card->addSlider(I18n::get("settings.triggerOctave"), 0.0, 10.0, 1.0);
        trSl->setValue((double)SettingsManager::triggerOctave);
        trSl->onValueChange = [trSl] {
            SettingsManager::triggerOctave = (int)trSl->getValue();
            SettingsManager::save();
        };

        auto* chSl = card->addSlider(I18n::get("settings.midiChannel"), 0.0, 16.0, 1.0);
        chSl->setValue((double)SettingsManager::midiChannel);
        chSl->onValueChange = [chSl] {
            SettingsManager::midiChannel = (int)chSl->getValue();
            SettingsManager::save();
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

        int* trigPtrs[] = {
            &SettingsManager::triggerPlay, &SettingsManager::triggerPause,
            &SettingsManager::triggerNextSection, &SettingsManager::triggerHybrid,
            &SettingsManager::triggerCountdown3, &SettingsManager::triggerCountdown5
        };
        const char* trigKeys[] = {
            "settings.triggerPlay", "settings.triggerPause", "settings.triggerNext",
            "settings.triggerHybrid", "settings.triggerCount3", "settings.triggerCount5"
        };

        for (int idx = 0; idx < 6; ++idx)
        {
            auto* cb = card->addCombo(I18n::get(trigKeys[idx]));
            for (int i = 0; i < 12; ++i)
                cb->addItem(I18n::get(noteKeys[i]), i + 1);
            cb->setSelectedId(*trigPtrs[idx] + 1);
            cb->onChange = [cb, idx, trigPtrs] {
                *trigPtrs[idx] = cb->getSelectedId() - 1;
                SettingsManager::save();
            };
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
