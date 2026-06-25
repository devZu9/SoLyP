#include "SettingsComponent.h"
#include "SettingsCard.h"
#include "Theme.h"
#include "../Data/SettingsManager.h"
#include "../Data/I18n.h"
#include "icons/Icons.h"

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

    struct ColorPicker : juce::Component, juce::ChangeListener
    {
        juce::ColourSelector selector;
        juce::TextButton* target;

        ColorPicker()
            : selector(juce::ColourSelector::showColourAtTop
                 | juce::ColourSelector::showSliders | juce::ColourSelector::showColourspace)
        {
            setSize(300, 320);
            addAndMakeVisible(selector);
            selector.setBounds(getLocalBounds());
        }

        void changeListenerCallback(juce::ChangeBroadcaster*) override
        {
            if (target)
            {
                auto c = selector.getCurrentColour();
                target->setColour(juce::TextButton::buttonColourId, c);
            }
        }

        ~ColorPicker() override
        {
            selector.removeChangeListener(this);
            if (target)
            {
                auto c = selector.getCurrentColour();
                target->setColour(juce::TextButton::buttonColourId, c);
                SettingsManager::cursorColor = c.toDisplayString(false);
                SettingsManager::save();
            }
        }
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
        auto* bpmToggleLabel = card->getLastLabel();

        auto* bpvSl = card->addSlider(I18n::get("settings.manualBpmValue"), 20.0, 300.0, 1.0);
        bpvSl->setValue((double)SettingsManager::manualBpmValue);
        bpvSl->setVelocityBasedMode(true);
        bpvSl->setDoubleClickReturnValue(true, 120.0);
        bpvSl->onValueChange = [bpvSl] {
            SettingsManager::manualBpmValue = (float)bpvSl->getValue();
            SettingsManager::save();
        };

        auto* bpmLabel = card->getLastLabel();

        if (bpmToggleLabel != nullptr)
            bpmToggleLabel->addMouseListener(new LabelForwarder(
                [bpmTg] { bpmTg->triggerClick(); }, nullptr), false);

        if (bpmLabel != nullptr)
            bpmLabel->addMouseListener(new LabelForwarder(
                nullptr, [bpvSl] { bpvSl->setValue(120.0); }), false);

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

        card->addSeparator();

        // time signature
        auto* tsCb = card->addCombo(I18n::get("settings.timeSignature"));
        tsCb->addItem("4/4", 1);
        tsCb->addItem("3/4", 2);
        tsCb->addItem("2/4", 3);
        int tsId = (SettingsManager::timeSignature == 3) ? 2
                 : (SettingsManager::timeSignature == 2) ? 3 : 1;
        tsCb->setSelectedId(tsId, juce::dontSendNotification);
        tsCb->onChange = [tsCb] {
            int vals[] = { 4, 3, 2 };
            SettingsManager::timeSignature = vals[tsCb->getSelectedId() - 1];
            SettingsManager::save();
        };

        card->addSeparator();

        juce::ToggleButton* aotTg = card->addToggle(I18n::get("settings.alwaysOnTop"));
        aotTg->setToggleState(SettingsManager::alwaysOnTop, juce::dontSendNotification);
        juce::DrawableButton* aotInfo = card->addInfoIcon();

        auto svg = juce::String(Icons::question).replace("#000000", "#" + Theme::iconPrimary.toDisplayString(false));
        auto xml = juce::XmlDocument::parse(svg);
        if (xml)
        {
            auto drawable = juce::Drawable::createFromSVG(*xml);
            aotInfo->setImages(drawable.get());
            aotInfo->setTooltip(I18n::get("settings.alwaysOnTopTip"));
        }

        auto* aotLabel = card->getLastLabel();
        if (aotLabel)
        {
            aotLabel->addMouseListener(new LabelForwarder(
                [aotTg] { aotTg->triggerClick(); }, nullptr), false);
            aotLabel->setTooltip(I18n::get("settings.alwaysOnTopTip"));
        }

        aotTg->onStateChange = [aotTg] {
            SettingsManager::alwaysOnTop = aotTg->getToggleState();
            SettingsManager::save();
        };

        container->addAndMakeVisible(card);
    }

    // ── card 2: Display ──
    {
        auto* card = new SettingsCard(I18n::get("settings.textLines"));
        auto* preSl = card->addSlider(I18n::get("settings.preLines"), 1.0, 5.0, 1.0);
        preSl->setValue((double)SettingsManager::preLinesOnPause);
        preSl->onValueChange = [preSl] {
            SettingsManager::preLinesOnPause = (int)preSl->getValue();
            SettingsManager::save();
        };

        auto* linesPerBarSl = card->addSlider(I18n::get("settings.barsPerLine"), 1.0, 4.0, 1.0);
        linesPerBarSl->setValue((double)SettingsManager::barsPerLine);
        linesPerBarSl->setDoubleClickReturnValue(true, 1.0);
        linesPerBarSl->onValueChange = [linesPerBarSl] {
            SettingsManager::barsPerLine = (float)linesPerBarSl->getValue();
            SettingsManager::save();
        };
        {
            auto* info = card->addInfoIcon();
            if (info) {
                auto svg = juce::String(Icons::question).replace("#000000", "#" + Theme::iconPrimary.toDisplayString(false));
                auto xml = juce::XmlDocument::parse(svg);
                if (xml) {
                    auto drawable = juce::Drawable::createFromSVG(*xml);
                    info->setImages(drawable.get());
                    info->setTooltip(I18n::get("settings.barsPerLineTip"));
                }
            }
        }

        card->addSeparator();
        auto* pauseEditor = card->addEditor(I18n::get("settings.pauseText"), 60);
        pauseEditor->setText(SettingsManager::pauseText);
        pauseEditor->onTextChange = [pauseEditor] {
            SettingsManager::pauseText = pauseEditor->getText();
            SettingsManager::save();
        };

        container->addAndMakeVisible(card);
    }

    // ── card 3: MIDI ──
    {
        auto* card = new SettingsCard("MIDI");
        auto* octCb = card->addCombo(I18n::get("settings.octaveSystem"));
        octCb->addItem(I18n::get("settings.octaveStd"), 1);
        octCb->addItem(I18n::get("settings.octaveAbleton"), 2);
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
            &SettingsManager::triggerCountdown3, &SettingsManager::triggerStop
        };
        const char* trigKeys[] = {
            "settings.triggerPlay", "settings.triggerPause", "settings.triggerNext",
            "settings.triggerHybrid",             "settings.triggerCount3", "settings.triggerStop"
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

    // ── card 5: Cursor ──
    {
        auto* card = new SettingsCard(I18n::get("cursor.title"));

        // SVG images for cursor shapes
        auto svgTri = juce::String(Icons::cursorTriangle).replace("#000", "#" + Theme::iconPrimary.toDisplayString(false));
        auto svgSq  = juce::String(Icons::cursorSquare).replace("#000", "#" + Theme::iconPrimary.toDisplayString(false));
        auto triXml = juce::XmlDocument::parse(svgTri);
        auto sqXml  = juce::XmlDocument::parse(svgSq);
        auto triDrawable = juce::Drawable::createFromSVG(*triXml);
        auto sqDrawable  = juce::Drawable::createFromSVG(*sqXml);

        // Row 1: Enable/disable custom cursor
        auto* cursorTg = card->addToggle(I18n::get("cursor.enable"));
        cursorTg->setToggleState(SettingsManager::cursorEnabled, juce::dontSendNotification);
        auto* cursorTgLabel = card->getLastLabel();
        if (cursorTgLabel)
            cursorTgLabel->addMouseListener(new LabelForwarder(
                [cursorTg] { cursorTg->triggerClick(); }, nullptr), false);

        // Row 2: Shape selector (image pair)
        juce::DrawableButton* shapeBtn1 = nullptr;
        juce::DrawableButton* shapeBtn2 = nullptr;
        card->addImagePair(I18n::get("cursor.shape"), triDrawable.get(), sqDrawable.get(), shapeBtn1, shapeBtn2);
        shapeBtn1->setToggleState(SettingsManager::cursorShape == 0, juce::dontSendNotification);
        shapeBtn2->setToggleState(SettingsManager::cursorShape == 1, juce::dontSendNotification);
        shapeBtn1->setColour(juce::DrawableButton::backgroundOnColourId, Theme::accentBorder);
        shapeBtn2->setColour(juce::DrawableButton::backgroundOnColourId, Theme::accentBorder);
        if (SettingsManager::cursorShape == 0)
            shapeBtn1->setColour(juce::DrawableButton::backgroundColourId, Theme::accentBorder);
        else
            shapeBtn2->setColour(juce::DrawableButton::backgroundColourId, Theme::accentBorder);

        auto* shapeLabel = card->getLastLabel();

        // Row 3: Size slider
        auto* sizeSl = card->addSlider(I18n::get("cursor.size"), 8.0, 64.0, 1.0);
        sizeSl->setValue((double)SettingsManager::cursorSize);
        sizeSl->onValueChange = [sizeSl] {
            SettingsManager::cursorSize = (int)sizeSl->getValue();
            SettingsManager::save();
        };
        auto* sizeLabel = card->getLastLabel();

        // Row 4: Rotation toggle
        auto* rotTg = card->addToggle(I18n::get("cursor.rotation"));
        rotTg->setToggleState(SettingsManager::cursorRotation, juce::dontSendNotification);
        auto* rotTgLabel = card->getLastLabel();

        // Row 5: Direction radio pair
        juce::ToggleButton *dirCCW = nullptr, *dirCW = nullptr;
        card->addRadioPair(I18n::get("cursor.direction"), I18n::get("cursor.dirCCW"), I18n::get("cursor.dirCW"), dirCCW, dirCW);
        dirCCW->setToggleState(SettingsManager::cursorRotDir == 0, juce::dontSendNotification);
        dirCW->setToggleState(SettingsManager::cursorRotDir == 1, juce::dontSendNotification);

        auto* dirLabel = card->getLastLabel();

        // Row 6: Speed slider
        auto* speedSl = card->addSlider(I18n::get("cursor.speed"), 1.0, 20.0, 1.0);
        speedSl->setValue((double)SettingsManager::cursorRotSpeed);
        speedSl->onValueChange = [speedSl] {
            SettingsManager::cursorRotSpeed = (int)speedSl->getValue();
            SettingsManager::save();
        };
        auto* speedLabel = card->getLastLabel();

        // Row 7: Color picker
        auto* colorBtn = card->addButton(I18n::get("cursor.color"));
        juce::Colour curColor = juce::Colour::fromString("ff" + SettingsManager::cursorColor);
        colorBtn->setButtonText("");
        colorBtn->setColour(juce::TextButton::buttonColourId, curColor);
        colorBtn->onClick = [colorBtn] {
            auto* picker = new ColorPicker();
            picker->target = colorBtn;
            picker->selector.setCurrentColour(juce::Colour::fromString("ff" + SettingsManager::cursorColor));
            picker->selector.addChangeListener(picker);
            auto* box = new juce::CallOutBox(*picker, colorBtn->getScreenBounds(), nullptr);
            box->enterModalState(true, juce::ModalCallbackFunction::create([picker, box, colorBtn](int) {
                auto c = picker->selector.getCurrentColour();
                colorBtn->setColour(juce::TextButton::buttonColourId, c);
                SettingsManager::cursorColor = c.toDisplayString(false);
                SettingsManager::save();
                delete picker;
                delete box;
            }));
        };

        // Callbacks
        cursorTg->onStateChange = [cursorTg, shapeBtn1, shapeBtn2, shapeLabel, sizeSl, sizeLabel, rotTg, rotTgLabel, dirCCW, dirCW, dirLabel, speedSl, speedLabel, colorBtn] {
            bool on = cursorTg->getToggleState();
            float a = on ? 1.0f : 0.4f;
            SettingsManager::cursorEnabled = on;
            shapeBtn1->setEnabled(on); shapeBtn1->setAlpha(a);
            shapeBtn2->setEnabled(on); shapeBtn2->setAlpha(a);
            if (shapeLabel) shapeLabel->setAlpha(a);
            sizeSl->setEnabled(on); sizeSl->setAlpha(a);
            if (sizeLabel) sizeLabel->setAlpha(a);
            rotTg->setEnabled(on); rotTg->setAlpha(a);
            if (rotTgLabel) rotTgLabel->setAlpha(a);
            colorBtn->setEnabled(on); colorBtn->setAlpha(a);
            if (!on)
            {
                dirCCW->setEnabled(false); dirCCW->setAlpha(0.4f);
                dirCW->setEnabled(false);  dirCW->setAlpha(0.4f);
                if (dirLabel) dirLabel->setAlpha(0.4f);
                speedSl->setEnabled(false); speedSl->setAlpha(0.4f);
                if (speedLabel) speedLabel->setAlpha(0.4f);
            }
            else if (rotTg->getToggleState())
            {
                dirCCW->setEnabled(true); dirCCW->setAlpha(1.0f);
                dirCW->setEnabled(true);  dirCW->setAlpha(1.0f);
                if (dirLabel) dirLabel->setAlpha(1.0f);
                speedSl->setEnabled(true); speedSl->setAlpha(1.0f);
                if (speedLabel) speedLabel->setAlpha(1.0f);
            }
            SettingsManager::save();
        };

        shapeBtn1->onClick = [shapeBtn1, shapeBtn2] {
            if (shapeBtn1->getToggleState()) return;
            shapeBtn1->setToggleState(true, juce::dontSendNotification);
            shapeBtn2->setToggleState(false, juce::dontSendNotification);
            shapeBtn1->setColour(juce::DrawableButton::backgroundColourId, Theme::accentBorder);
            shapeBtn2->setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
            SettingsManager::cursorShape = 0;
            SettingsManager::save();
        };
        shapeBtn2->onClick = [shapeBtn1, shapeBtn2] {
            if (shapeBtn2->getToggleState()) return;
            shapeBtn2->setToggleState(true, juce::dontSendNotification);
            shapeBtn1->setToggleState(false, juce::dontSendNotification);
            shapeBtn2->setColour(juce::DrawableButton::backgroundColourId, Theme::accentBorder);
            shapeBtn1->setColour(juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
            SettingsManager::cursorShape = 1;
            SettingsManager::save();
        };

        rotTg->onStateChange = [rotTg, dirCCW, dirCW, dirLabel, speedSl, speedLabel] {
            bool on = rotTg->getToggleState();
            float a = on ? 1.0f : 0.4f;
            SettingsManager::cursorRotation = on;
            dirCCW->setEnabled(on); dirCCW->setAlpha(a);
            dirCW->setEnabled(on);  dirCW->setAlpha(a);
            if (dirLabel) dirLabel->setAlpha(a);
            speedSl->setEnabled(on); speedSl->setAlpha(a);
            if (speedLabel) speedLabel->setAlpha(a);
            SettingsManager::save();
        };

        dirCCW->onStateChange = [dirCCW] {
            if (dirCCW->getToggleState()) {
                SettingsManager::cursorRotDir = 0;
                SettingsManager::save();
            }
        };
        dirCW->onStateChange = [dirCW] {
            if (dirCW->getToggleState()) {
                SettingsManager::cursorRotDir = 1;
                SettingsManager::save();
            }
        };

        // Apply initial dim state
        {
            bool curOn = SettingsManager::cursorEnabled;
            float curA = curOn ? 1.0f : 0.4f;
            bool rotOn = curOn && SettingsManager::cursorRotation;
            float rotA = rotOn ? 1.0f : 0.4f;

            shapeBtn1->setEnabled(curOn); shapeBtn1->setAlpha(curA);
            shapeBtn2->setEnabled(curOn); shapeBtn2->setAlpha(curA);
            if (shapeLabel) shapeLabel->setAlpha(curA);
            sizeSl->setEnabled(curOn); sizeSl->setAlpha(curA);
            if (sizeLabel) sizeLabel->setAlpha(curA);
            rotTg->setEnabled(curOn); rotTg->setAlpha(curA);
            if (rotTgLabel) rotTgLabel->setAlpha(curA);
            dirCCW->setEnabled(rotOn); dirCCW->setAlpha(rotA);
            dirCW->setEnabled(rotOn);  dirCW->setAlpha(rotA);
            if (dirLabel) dirLabel->setAlpha(rotA);
            speedSl->setEnabled(rotOn); speedSl->setAlpha(rotA);
            if (speedLabel) speedLabel->setAlpha(rotA);
            colorBtn->setEnabled(curOn); colorBtn->setAlpha(curA);
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
