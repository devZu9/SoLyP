#include "../../Display.h"
#include "../../Data/SettingsManager.h"

void SoLyPAudioProcessorEditor::timerCallback(int timerId)
{
    switch (timerId) {
        case ScrollId:     scrollCallback();     break;
        case PauseId:      pauseCallback();       break;
        case CountdownId:  countdownCallback();   break;
        case CursorId:     cursorCallback();      break;
    }
}

void SoLyPAudioProcessorEditor::scrollCallback()
{
    auto state = processor.getTransportState();
    if (state != SoLyPAudioProcessor::TransportState::Playing
        && state != SoLyPAudioProcessor::TransportState::Countdown) return;

    double now = juce::Time::getMillisecondCounterHiRes();
    double elapsed = now - lastScrollTime;
    lastScrollTime = now;
    double timePerLine = getTimePerLine();
    if (timePerLine <= 0.0 || slots.empty()) { repaint(); return; }

    double step = elapsed / (timePerLine * 1000.0);
    if (state == SoLyPAudioProcessor::TransportState::Countdown && countdownPhase > 0)
        step *= 5.0;

    scrollOffset += step;
    int N = (int)slots.size();

    if (state == SoLyPAudioProcessor::TransportState::Countdown && showPauseText)
    {
        double lh = SettingsManager::fontSize * SettingsManager::lineSpacing;
        pauseMsgY -= step * lh;
    }

    while (scrollOffset >= 1.0)
    {
        scrollOffset -= 1.0;
        const auto& song = processor.getCurrentSong();
        if (song.displayLines.isEmpty()) { repaint(); return; }
        for (int i = 0; i < N - 1; ++i)
            slots[i].text = slots[i + 1].text;
        if (nextLineIndex < song.displayLines.size())
            slots[N - 1].text = song.displayLines[nextLineIndex++].text;
        else
            processor.enterStop();
    }
    repaint();
}

void SoLyPAudioProcessorEditor::pauseCallback()
{
    auto state = processor.getTransportState();
    if (state != SoLyPAudioProcessor::TransportState::Paused) return;

    double now = juce::Time::getMillisecondCounterHiRes();
    double elapsed = now - lastScrollTime;
    lastScrollTime = now;
    double timePerLine = getTimePerLine();
    if (timePerLine <= 0.0) { repaint(); return; }

    double step = elapsed / (timePerLine * 1000.0) * 5.0;

    if (showPauseText)
    {
        double lh = SettingsManager::fontSize * SettingsManager::lineSpacing;
        pauseMsgY -= step * lh;
    }
    repaint();
}

void SoLyPAudioProcessorEditor::countdownCallback()
{
    double now = juce::Time::getMillisecondCounterHiRes();
    double elapsed = now - countdownPhaseStart;

    if (elapsed >= countdownPhaseDuration)
    {
        countdownPhase++;
        if (countdownPhase > 3)
        {
            countdownPhase = 0;
            stopTimer(CountdownId);
            stopTimer(ScrollId);
            processor.enterPlay();
            return;
        }
        countdownPhaseStart = now;
    }
    repaint();
}

void SoLyPAudioProcessorEditor::cursorCallback()
{
    if (!SettingsManager::cursorEnabled) { stopTimer(CursorId); return; }
    auto screenPos = juce::Desktop::getInstance().getMousePosition();
    mousePos = getLocalPoint(nullptr, screenPos);
    cursorAngle -= 0.005f * powf(1.3f, (float)(SettingsManager::cursorRotSpeed - 1));
    if (cursorAngle < 0.0f) cursorAngle += juce::MathConstants<float>::twoPi;
    int sz = SettingsManager::cursorSize + 4;
    static juce::Point<int> prevPos;
    repaint(juce::Rectangle<int>(prevPos.x - sz, prevPos.y - sz, sz * 2, sz * 2));
    repaint(juce::Rectangle<int>(mousePos.x - sz, mousePos.y - sz, sz * 2, sz * 2));
    prevPos = mousePos;
}
