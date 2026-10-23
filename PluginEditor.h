#pragma once
#include "PluginProcessor.h"
#include <array>
#include <vector>

// ---------------------------------------------------------------------------
//  Look: glowing rotary knobs + acid toggle buttons
// ---------------------------------------------------------------------------
class FxLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
};

class TwoKEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TwoKEditor (TwoKProcessor&);
    ~TwoKEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::String name;
        float hover = 0.0f;
        std::unique_ptr<SliderAtt> att;
    };

    struct Particle { float x, y, vy, size, phase, alpha; int kind; };

    void timerCallback() override;
    void drawGhost (juce::Graphics&);
    void drawFloor (juce::Graphics&);
    void drawParticles (juce::Graphics&);
    void drawPerson (juce::Graphics&);
    void drawTitle (juce::Graphics&);
    void drawDisplay (juce::Graphics&);
    void drawBar (juce::Graphics&, juce::Rectangle<float>, float norm, float peakNorm, bool fromTop,
                  const juce::String& label, const juce::String& readout);
    void drawTicker (juce::Graphics&);
    static juce::Image makeGlow (const juce::Image& src, juce::Colour c);

    FxLookAndFeel laf;
    TwoKProcessor& proc;

    juce::Image img, glow, scanlines;
    juce::Rectangle<float> imgRect;
    juce::Path ghostPath;
    float tickerW = 1.0f, tickerX = 0.0f;

    std::array<Knob, 12> knobs;
    juce::ToggleButton rmsBtn { "RMS" }, autoBtn { "AUTO REL" };
    std::unique_ptr<ButtonAtt> rmsAtt, autoAtt;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };

    // animation state
    float t = 0.0f;
    int frame = 0;
    float inDbS = -100.f, outDbS = -100.f, grS = 0.f, lastGr = 0.f;
    float inPeak = -100.f, outPeak = -100.f, grPeak = 0.f;
    int inHold = 0, outHold = 0, grHold = 0;
    float outN = 0.f, grN = 0.f;
    float gridPhase = 0.f, presetFlash = 0.f;
    int glitchFrames = 14;
    float stripY[6] {}, stripH[6] {}, stripDx[6] {};
    juce::String titleText = "2KCOMP";

    static constexpr int histN = 150;
    std::array<float, histN> history {};
    int histPos = 0;
    std::vector<Particle> particles;
    juce::Random rng;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TwoKEditor)
};
