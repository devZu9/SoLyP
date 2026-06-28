#include "../../Display.h"
#include "../../Data/SettingsManager.h"

void SoLyPAudioProcessorEditor::timerCallback(int timerId)
{
    switch (timerId) {
        case TimerScroll:     scrollCallback();     break;
        case TimerPreLines:   preLinesCallback();    break;
        case TimerPause:      pauseCallback();       break;
        case TimerCountdown:  countdownCallback();   break;
        case TimerCursor:     cursorCallback();      break;
    }
}

void SoLyPAudioProcessorEditor::scrollCallback()
{
    auto state = processor.getTransportState();
    if (state != SoLyPAudioProcessor::TransportState::Playing
        && state != SoLyPAudioProcessor::TransportState::Paused
        && state != SoLyPAudioProcessor::TransportState::Countdown) return;

    double now = juce::Time::getMillisecondCounterHiRes();
    double actualTimePerFrameMs = now - lastScrollTime;
    lastScrollTime = now;
    double timePerLine = getTimePerLine();
    if (timePerLine <= 0.0 || slots.empty()) { repaint(); return; }

    double step = actualTimePerFrameMs / (timePerLine * 1000.0);
    if (state == SoLyPAudioProcessor::TransportState::Paused)
        step *= SoLyPAudioProcessorEditor::boost;

    double lineHeight = realLineHeight;
    double stepPerFramePx = step * lineHeight;
    int N = (int)slots.size();

    if (state == SoLyPAudioProcessor::TransportState::Paused)
    {
        if (fastImage.isValid())
        {
            fastImageY -= stepPerFramePx;
            if (fastImageY + (double)fastImage.getHeight() < 0.0)
                fastImage = {};
        }
        else
            stopTimer(TimerScroll);
        repaint();
        return;
    }

    // Playing / Countdown — все слоты, нормальный shift
    for (auto& s : slots)
        s.y -= stepPerFramePx;

    while (!slots.empty() && slots[N - 1].y + lineHeight < topYlastLine)
    {
        const auto& song = processor.getCurrentSong();
        if (song.displayLines.isEmpty()) { repaint(); return; }
        for (int i = 0; i < N - 1; ++i)
        {
            slots[i].text = slots[i + 1].text;
            slots[i].y = slots[i + 1].y;
        }
        if (nextLineIndex < song.displayLines.size())
        {
            slots[N - 1].text = song.displayLines[nextLineIndex++].text;
            slots[N - 1].y = slots[N - 2].y + lineHeight;
        }
        else
            processor.switchStop();
    }
    repaint();
}

void SoLyPAudioProcessorEditor::preLinesCallback()
{
    auto state = processor.getTransportState();
    if (state != SoLyPAudioProcessor::TransportState::Paused) return;
    if (slots.empty()) return;

    double now = juce::Time::getMillisecondCounterHiRes();
    double actualTimePerFrameMs = now - lastPreLineTime;
    lastPreLineTime = now;
    double timePerLine = getTimePerLine();
    if (timePerLine <= 0.0) { repaint(); return; }

    double step = actualTimePerFrameMs / (timePerLine * 1000.0);
    double lineHeight = realLineHeight;
    double stepPerFramePx = step * lineHeight;
    int N = (int)slots.size();

    for (auto& s : slots)
        s.y -= stepPerFramePx;

    const auto& song = processor.getCurrentSong();

    if (!slots.empty()
        && slots[N - 1].y + lineHeight < topYlastLine)
    {
        for (int i = 0; i < N - 1; ++i)
        {
            slots[i].text = slots[i + 1].text;
            slots[i].y = slots[i + 1].y;
        }
        if (nextLineIndex < song.displayLines.size())
        {
            slots[N - 1].text = song.displayLines[nextLineIndex++].text;
            slots[N - 1].y = slots[N - 2].y + lineHeight;
        }
        pauseShiftCount = 1;
    }

    if (pauseShiftCount >= 1 && slots[N - 1].y <= topYlastLine)
    {
        stopTimer(TimerPreLines);
    }

    repaint();
}

void SoLyPAudioProcessorEditor::pauseCallback()
{
    if (!showPauseText) { stopTimer(TimerPause); return; }
    if (slots.empty()) return;

    double now = juce::Time::getMillisecondCounterHiRes();
    double actualTimePerFrameMs = now - lastPauseTime;
    lastPauseTime = now;
    double lineHeight = realLineHeight;
    double stepPerFramePx = actualTimePerFrameMs / pauseMsgSpeed * lineHeight;
    pauseMsgY -= stepPerFramePx;

    if (lyricsViewArea.getBottom() + pauseMsgY < (float)lyricsViewArea.getY())
    {
        pauseMsgY = (float)lyricsViewArea.getY() - (float)lyricsViewArea.getBottom();
        stopTimer(TimerPause);
    }

    repaint();
}

void SoLyPAudioProcessorEditor::countdownCallback()
{
    double now = juce::Time::getMillisecondCounterHiRes();
    double timeDifference = now - countdownPhaseStart;

    if (timeDifference >= countdownPhaseDuration)
    {
        countdownPhase--;
        if (countdownPhase < 1)
        {
            countdownPhase = 0;
            stopTimer(TimerCountdown);
            processor.restoreAfterCountdown();
            return;
        }
        countdownPhaseStart = now;
    }
    repaint();
}

void SoLyPAudioProcessorEditor::cursorCallback()
{
    if (!SettingsManager::cursorEnabled) { stopTimer(TimerCursor); return; }
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
