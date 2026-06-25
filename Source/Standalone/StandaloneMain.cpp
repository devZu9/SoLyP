#include <JuceHeader.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>
#include "../Display.h"
#include "../Logic.h"
#include "../Data/SettingsManager.h"
#include "../Data/I18n.h"
#include "../UI/Theme.h"

class SoLyPStandaloneWindow : public StandaloneFilterWindow
{
public:
    using StandaloneFilterWindow::StandaloneFilterWindow;

    void closeButtonPressed() override
    {
        auto* proc = dynamic_cast<SoLyPAudioProcessor*>(getPluginHolder()->processor.get());
        auto* editor = proc ? dynamic_cast<SoLyPAudioProcessorEditor*>(proc->getActiveEditor()) : nullptr;

        if (editor && editor->isTextModified())
        {
            auto* alert = new juce::AlertWindow(
                I18n::get("confirm.unsaved"),
                I18n::get("confirm.closeApp"),
                juce::AlertWindow::QuestionIcon);
            alert->setColour(juce::AlertWindow::backgroundColourId, Theme::bgPanel);
            alert->setColour(juce::AlertWindow::textColourId, Theme::textPrimary);
            alert->setColour(juce::AlertWindow::outlineColourId, Theme::accentBorder);
            alert->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
            alert->addButton(juce::String::fromUTF8("Отмена"), 0, juce::KeyPress(juce::KeyPress::escapeKey));
            alert->enterModalState(true, juce::ModalCallbackFunction::create([alert](int r) {
                if (r == 1)
                    juce::JUCEApplication::getInstance()->quit();
                delete alert;
            }));
        }
        else
        {
            juce::JUCEApplication::getInstance()->quit();
        }
    }
};

class SoLyPStandaloneApp : public juce::JUCEApplication
{
public:
    SoLyPStandaloneApp()
    {
        juce::PropertiesFile::Options options;
        options.applicationName     = JucePlugin_Name;
        options.filenameSuffix      = ".settings";
        options.osxLibrarySubFolder = "Application Support";
       #if JUCE_LINUX || JUCE_BSD
        options.folderName          = "~/.config";
       #else
        options.folderName          = "";
       #endif
        appProperties.setStorageParameters(options);
    }

    const juce::String getApplicationName() override    { return JucePlugin_Name; }
    const juce::String getApplicationVersion() override { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override          { return true; }
    void anotherInstanceStarted(const juce::String&) override {}

    void initialise(const juce::String&) override
    {
        auto holder = createPluginHolder();
        if (juce::Desktop::getInstance().getDisplays().displays.isEmpty())
        {
            pluginHolder = std::move(holder);
            return;
        }

        mainWindow.reset(new SoLyPStandaloneWindow(
            getApplicationName(),
            juce::LookAndFeel::getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId),
            std::move(holder)));
        mainWindow->setVisible(true);
    }

    void shutdown() override
    {
        pluginHolder = nullptr;
        mainWindow = nullptr;
        appProperties.saveIfNeeded();
    }

    void systemRequestedQuit() override
    {
        if (pluginHolder != nullptr)
            pluginHolder->savePluginState();
        else if (mainWindow != nullptr)
            mainWindow->pluginHolder->savePluginState();

        if (juce::ModalComponentManager::getInstance()->cancelAllModalComponents())
        {
            juce::Timer::callAfterDelay(100, []() {
                if (auto* app = juce::JUCEApplicationBase::getInstance())
                    app->systemRequestedQuit();
            });
        }
        else
        {
            quit();
        }
    }

private:
    std::unique_ptr<StandalonePluginHolder> createPluginHolder()
    {
        constexpr auto autoOpenMidiDevices =
       #if (JUCE_ANDROID || JUCE_IOS) && ! JUCE_DONT_AUTO_OPEN_MIDI_DEVICES_ON_MOBILE
                true;
       #else
                false;
       #endif

       #ifdef JucePlugin_PreferredChannelConfigurations
        constexpr StandalonePluginHolder::PluginInOuts channels[] { JucePlugin_PreferredChannelConfigurations };
        const juce::Array<StandalonePluginHolder::PluginInOuts> channelConfig(channels, juce::numElementsInArray(channels));
       #else
        const juce::Array<StandalonePluginHolder::PluginInOuts> channelConfig;
       #endif

        return std::make_unique<StandalonePluginHolder>(appProperties.getUserSettings(),
                                                         false,
                                                         juce::String{},
                                                         nullptr,
                                                         channelConfig,
                                                         autoOpenMidiDevices);
    }

    juce::ApplicationProperties appProperties;
    std::unique_ptr<StandalonePluginHolder> pluginHolder;
    std::unique_ptr<SoLyPStandaloneWindow> mainWindow;
};

START_JUCE_APPLICATION(SoLyPStandaloneApp)
