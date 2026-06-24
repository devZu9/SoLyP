#include "Song.h"

Song Song::fromJson(const juce::String& jsonText)
{
    Song song;
    auto parsed = juce::JSON::parse(jsonText);
    if (!parsed.isObject()) { song.validationError = "Invalid JSON"; return song; }
    auto* root = parsed.getDynamicObject();
    if (!root) { song.validationError = "Invalid JSON"; return song; }

    auto songArray = root->getProperty("song");
    if (!songArray.isArray()) { song.validationError = "Missing 'song' array"; return song; }

    int sectionId = 0;
    for (auto& sectionVal : *songArray.getArray())
    {
        if (!sectionVal.isObject()) continue;
        auto* sectionObj = sectionVal.getDynamicObject();
        if (!sectionObj) continue;

        juce::String sectionName = sectionObj->getProperty("section").toString();

        auto lines = sectionObj->getProperty("lines");
        if (lines.isArray())
        {
            for (auto& line : *lines.getArray())
            {
                TextLine tl;
                tl.sectionId = sectionId;
                tl.sectionName = sectionName;
                tl.text = line.toString();
                song.textSong.add(tl);
            }
        }
        sectionId++;
    }

    // Count sections from textSong
    int sectionCount = 0;
    for (auto& tl : song.textSong)
        sectionCount = juce::jmax(sectionCount, tl.sectionId + 1);

    for (int i = 0; i < sectionCount; ++i)
        song.legends.add({});

    auto legends = root->getProperty("legends");
    if (legends.isArray())
    {
        for (auto& legVal : *legends.getArray())
        {
            if (!legVal.isObject()) continue;
            auto* legObj = legVal.getDynamicObject();
            if (!legObj) continue;
            juce::String secName = legObj->getProperty("section").toString();
            juce::String note = legObj->getProperty("note").toString();
            for (auto& tl : song.textSong)
            {
                if (tl.sectionName == secName)
                {
                    song.legends.set(tl.sectionId, note);
                    break;
                }
            }
        }
    }

    // Fill missing legends automatically
    for (int i = 0; i < sectionCount; ++i)
    {
        if (song.legends[i].isEmpty())
        {
            const char* noteNames[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
            int octave = 1 + (i / 12);
            int noteIdx = i % 12;
            song.legends.set(i, juce::String(noteNames[noteIdx]) + juce::String(octave));
        }
    }

    if (root->hasProperty("title"))
        song.fileTitle = root->getProperty("title").toString();

    auto settings = root->getProperty("settings");
    if (settings.isObject())
    {
        auto* settingsObj = settings.getDynamicObject();
        if (settingsObj && settingsObj->hasProperty("preLinesOnPause"))
            song.preLinesOnPause = static_cast<int>(settingsObj->getProperty("preLinesOnPause"));
    }

    song.validationError = {};
    return song;
}

Song Song::fromText(const juce::String& text)
{
    Song song;
    juce::StringArray lines;
    lines.addTokens(text, "\n", {});
    lines.removeEmptyStrings(false);

    juce::String currentName;
    bool hasSection = false;
    int sectionId = 0;

    for (auto& line : lines)
    {
        auto trimmed = line.trim();
        if (trimmed.isEmpty()) continue;

        if (trimmed.startsWith("[") && trimmed.endsWith("]"))
        {
            if (hasSection) sectionId++;
            currentName = trimmed.substring(1, trimmed.length() - 1).trim();
            hasSection = true;
            continue;
        }

        if (!hasSection) continue;

        TextLine tl;
        tl.sectionId = sectionId;
        tl.sectionName = currentName;
        tl.text = trimmed;
        song.textSong.add(tl);
    }

    // Auto-generate legends
    int sectionCount = 0;
    for (auto& tl : song.textSong)
        sectionCount = juce::jmax(sectionCount, tl.sectionId + 1);
    const char* noteNames[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    for (int i = 0; i < sectionCount; ++i)
    {
        int octave = 1 + (i / 12);
        int noteIdx = i % 12;
        song.legends.add(juce::String(noteNames[noteIdx]) + juce::String(octave));
    }

    if (song.textSong.isEmpty())
        song.validationError = "No sections found in text";

    return song;
}

juce::String Song::toJson() const
{
    auto root = new juce::DynamicObject();

    // Rebuild sections array from textSong
    juce::Array<juce::var> songArray;
    int currentSid = -1;
    juce::DynamicObject* currentSectionObj = nullptr;
    juce::Array<juce::var>* currentLines = nullptr;

    for (auto& tl : textSong)
    {
        if (tl.sectionId != currentSid)
        {
            if (currentSectionObj)
            {
                currentSectionObj->setProperty("lines", juce::var(*currentLines));
                songArray.add(juce::var(currentSectionObj));
            }
            currentSid = tl.sectionId;
            currentSectionObj = new juce::DynamicObject();
            currentSectionObj->setProperty("section", tl.sectionName);
            currentLines = new juce::Array<juce::var>();
        }
        currentLines->add(tl.text);
    }
    if (currentSectionObj)
    {
        currentSectionObj->setProperty("lines", juce::var(*currentLines));
        songArray.add(juce::var(currentSectionObj));
    }
    root->setProperty("song", juce::var(songArray));

    // Legends
    juce::Array<juce::var> legendsArray;
    for (int i = 0; i < legends.size(); ++i)
    {
        if (legends[i].isEmpty()) continue;
        auto legObj = new juce::DynamicObject();
        for (auto& tl : textSong)
        {
            if (tl.sectionId == i)
            {
                legObj->setProperty("section", tl.sectionName);
                break;
            }
        }
        legObj->setProperty("note", legends[i]);
        legendsArray.add(juce::var(legObj));
    }
    root->setProperty("legends", juce::var(legendsArray));

    if (preLinesOnPause > 0)
    {
        auto settingsObj = new juce::DynamicObject();
        settingsObj->setProperty("preLinesOnPause", preLinesOnPause);
        root->setProperty("settings", juce::var(settingsObj));
    }

    return juce::JSON::toString(juce::var(root), false);
}

void Song::rebuildDisplayLines(float maxWidth, float fontSize) const
{
    displayLines.clear();
    lastBuildWidth = maxWidth;
    lastBuildFontSize = fontSize;

    if (maxWidth <= 0 || fontSize <= 0 || textSong.isEmpty()) return;

    juce::Font f{juce::FontOptions(fontSize)};

    for (auto& tl : textSong)
    {
        juce::GlyphArrangement ga;
            ga.addLineOfText(f, tl.text, 0.0f, 0.0f);
            float textW = ga.getBoundingBox(0, ga.getNumGlyphs(), false).getWidth();

            if (textW <= maxWidth)
            {
                displayLines.add({tl.sectionId, tl.sectionName, tl.text, 1});
                continue;
            }

            juce::StringArray fragments;
            juce::String remaining = tl.text;
            while (remaining.isNotEmpty())
            {
                juce::GlyphArrangement ga2;
                ga2.addLineOfText(f, remaining, 0.0f, 0.0f);
                float w = ga2.getBoundingBox(0, ga2.getNumGlyphs(), false).getWidth();
                if (w <= maxWidth) { fragments.add(remaining); break; }

                int splitIdx = -1;
                for (int ch = 0; ch < remaining.length(); ++ch)
                {
                    if (remaining[ch] == ' ')
                    {
                        juce::String sub = remaining.substring(0, ch);
                        juce::GlyphArrangement ga3;
                        ga3.addLineOfText(f, sub, 0.0f, 0.0f);
                        if (ga3.getBoundingBox(0, ga3.getNumGlyphs(), false).getWidth() <= maxWidth)
                            splitIdx = ch;
                        else break;
                    }
                }
                if (splitIdx <= 0)
                {
                    for (int ch = 1; ch < remaining.length(); ++ch)
                    {
                        juce::String sub = remaining.substring(0, ch);
                        juce::GlyphArrangement ga3;
                        ga3.addLineOfText(f, sub, 0.0f, 0.0f);
                        if (ga3.getBoundingBox(0, ga3.getNumGlyphs(), false).getWidth() > maxWidth)
                        { splitIdx = ch - 1; if (splitIdx <= 0) splitIdx = 1; break; }
                    }
                    if (splitIdx <= 0) splitIdx = 1;
                }
                fragments.add(remaining.substring(0, splitIdx));
                remaining = remaining.substring(splitIdx).trimStart();
            }

            int parts = fragments.size();
            for (auto& frag : fragments)
                displayLines.add({tl.sectionId, tl.sectionName, frag, parts});
    }
}
