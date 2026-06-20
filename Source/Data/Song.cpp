#include "Song.h"

Song Song::fromJson(const juce::String& jsonText)
{
    Song song;
    auto parsed = juce::JSON::parse(jsonText);

    if (!parsed.isObject())
    {
        song.validationError = "Invalid JSON: root must be an object";
        return song;
    }

    auto* root = parsed.getDynamicObject();
    if (!root)
    {
        song.validationError = "Invalid JSON: root must be an object";
        return song;
    }

    auto songArray = root->getProperty("song");
    if (!songArray.isArray())
    {
        song.validationError = "Missing 'song' array";
        return song;
    }

    for (auto& sectionVal : *songArray.getArray())
    {
        if (!sectionVal.isObject()) continue;

        auto* sectionObj = sectionVal.getDynamicObject();
        if (!sectionObj) continue;

        Section section;
        section.name = sectionObj->getProperty("section").toString();
        auto lines = sectionObj->getProperty("lines");
        if (lines.isArray())
        {
            for (auto& line : *lines.getArray())
                section.lines.add(line.toString());
        }
        song.sections.add(section);
    }

    auto legends = root->getProperty("legends");
    if (legends.isArray())
    {
        for (auto& legVal : *legends.getArray())
        {
            if (!legVal.isObject()) continue;
            auto* legObj = legVal.getDynamicObject();
            if (!legObj) continue;

            LegendEntry entry;
            entry.section = legObj->getProperty("section").toString();
            entry.note = legObj->getProperty("note").toString();
            song.legends.add(entry);
        }
    }

    if (root->hasProperty("title"))
        song.fileTitle = root->getProperty("title").toString();

    auto settings = root->getProperty("settings");
    if (settings.isObject())
    {
        auto* settingsObj = settings.getDynamicObject();
        if (settingsObj)
        {
            if (settingsObj->hasProperty("preLinesOnPause"))
                song.preLinesOnPause = static_cast<int>(settingsObj->getProperty("preLinesOnPause"));
        }
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

    Section currentSection;
    bool hasSection = false;

    for (auto& line : lines)
    {
        auto trimmed = line.trim();
        if (trimmed.isEmpty())
            continue;

        if (trimmed.startsWith("[") && trimmed.endsWith("]"))
        {
            if (hasSection)
                song.sections.add(currentSection);

            currentSection = Section();
            currentSection.name = trimmed.substring(1, trimmed.length() - 1).trim();
            hasSection = true;
            continue;
        }

        if (!hasSection)
            continue;

        currentSection.lines.add(trimmed);
    }

    if (hasSection)
        song.sections.add(currentSection);

    if (song.sections.isEmpty())
        song.validationError = "No sections found in text";

    // Auto-generate legends
    const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    for (int i = 0; i < song.sections.size(); ++i)
    {
        LegendEntry entry;
        entry.section = song.sections[i].name;
        int octave = 1 + (i / 12);
        int noteIdx = i % 12;
        entry.note = juce::String(noteNames[noteIdx]) + juce::String(octave);
        song.legends.add(entry);
    }

    return song;
}

juce::String Song::toJson() const
{
    auto root = new juce::DynamicObject();

    juce::Array<juce::var> songArray;
    for (auto& section : sections)
    {
        auto sectionObj = new juce::DynamicObject();
        sectionObj->setProperty("section", section.name);
        juce::Array<juce::var> linesArray;
        for (auto& line : section.lines)
            linesArray.add(line);
        sectionObj->setProperty("lines", juce::var(linesArray));
        songArray.add(juce::var(sectionObj));
    }
    root->setProperty("song", juce::var(songArray));

    juce::Array<juce::var> legendsArray;
    for (auto& leg : legends)
    {
        auto legObj = new juce::DynamicObject();
        legObj->setProperty("section", leg.section);
        legObj->setProperty("note", leg.note);
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

void Song::rebuildDisplayLines(float maxWidth, float fontSize, int longLineBehavior) const
{
    displayLines.clear();
    lastBuildWidth = maxWidth;
    lastBuildFontSize = fontSize;

    if (maxWidth <= 0 || fontSize <= 0 || sections.isEmpty())
        return;

    juce::Font f{juce::FontOptions(fontSize)};

    for (int si = 0; si < sections.size(); ++si)
    {
        const auto& section = sections[si];
        for (int li = 0; li < section.lines.size(); ++li)
        {
            const auto& line = section.lines[li];

            if (longLineBehavior == 0) // wrap — split long lines
            {
                juce::GlyphArrangement ga;
                ga.addLineOfText(f, line, 0.0f, 0.0f);
                float textW = ga.getBoundingBox(0, ga.getNumGlyphs(), false).getWidth();

                if (textW <= maxWidth)
                {
                    displayLines.add({line, si, 1});
                    continue;
                }

                // collect fragments first to know parts count
                juce::StringArray fragments;
                juce::String remaining = line;
                while (remaining.isNotEmpty())
                {
                    juce::GlyphArrangement ga2;
                    ga2.addLineOfText(f, remaining, 0.0f, 0.0f);
                    float w = ga2.getBoundingBox(0, ga2.getNumGlyphs(), false).getWidth();

                    if (w <= maxWidth)
                    {
                        fragments.add(remaining);
                        break;
                    }

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
                            else
                                break;
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
                            {
                                splitIdx = ch - 1;
                                if (splitIdx <= 0) splitIdx = 1;
                                break;
                            }
                        }
                        if (splitIdx <= 0) splitIdx = 1;
                    }

                    fragments.add(remaining.substring(0, splitIdx));
                    remaining = remaining.substring(splitIdx).trimStart();
                }

                int parts = fragments.size();
                for (auto& frag : fragments)
                    displayLines.add({frag, si, parts});
            }
            else // shrink — one entry per line, parts=1
            {
                displayLines.add({line, si, 1});
            }
        }
    }
}
