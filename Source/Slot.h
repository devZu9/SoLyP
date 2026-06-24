#pragma once

struct Slot {
    juce::String text;
    double y = 0.0;
};

struct AnimationConfig {
    int slotStart = 0;
    int slotCount = 0;
    double startY = 0.0;
    double endY = 0.0;
    double durationMs = 0.0;
    bool active = false;
    double elapsed = 0.0;
    std::function<void()> onComplete = nullptr;
};
