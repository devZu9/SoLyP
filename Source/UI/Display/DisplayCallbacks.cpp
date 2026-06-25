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
    if (state == SoLyPAudioProcessor::TransportState::Paused
        || state == SoLyPAudioProcessor::TransportState::Countdown && countdownPhase > 0)
        step *= 5.0;

    double lineHeight = realLineHeight;
    double stepPerFramePx = step * lineHeight;
    int N = (int)slots.size();

    if (state == SoLyPAudioProcessor::TransportState::Paused)
    {
        int pre = std::min(SettingsManager::preLinesOnPause, N);
        // Только верхние слоты (старый текст) — 5×, shift не нужен
        for (int i = 0; i < N - pre; ++i)
            slots[i].y -= stepPerFramePx;
        repaint();
        return;
    }

    // Playing / Countdown — все слоты, нормальный shift
    for (auto& s : slots)
        s.y -= stepPerFramePx;

    while (!slots.empty() && slots[0].y + lineHeight < 0.0)
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
    int N = (int)slots.size();
    int pre = std::min(SettingsManager::preLinesOnPause, N);
    int preStart = N - pre;
    double lineHeight = realLineHeight;

    // Движение предстрок
    double stepPerFramePx = step * lineHeight;
    for (int i = preStart; i < N; ++i)
        slots[i].y -= stepPerFramePx;

    // Накопление сдвига
    preScroll += step;
    const auto& song = processor.getCurrentSong();

    while (preScroll >= 1.0 && pauseShiftCount < pre)
    {
        preScroll -= 1.0;

        // Сдвиг предстрок наверх
        for (int i = preStart; i < N - 1; ++i)
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

        pauseShiftCount++;
    }

    if (pauseShiftCount >= pre)
        stopTimer(TimerPreLines);

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

    auto bounds = getLocalBounds().reduced(40, 20);
    if (bounds.getBottom() + pauseMsgY < (float)bounds.getY())
    {
        pauseMsgY = (float)bounds.getY() - (float)bounds.getBottom();
        stopTimer(TimerPause);
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
            stopTimer(TimerCountdown);
            stopTimer(TimerScroll);
            processor.switchPlay();
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
