#include "PluginProcessor.h"
#include "PluginEditor.h"

const char* const TwoKProcessor::paramIds[14] = { "in", "thr", "ratio", "atk", "rel", "knee", "hpf",
                                                  "grit", "air", "makeup", "mix", "out", "rms", "auto" };

namespace
{
    struct Preset { const char* name; float v[14]; };
    //                           in   thr  ratio atk   rel  knee hpf  grit air makeup mix  out  rms auto
    const Preset presets[] = {
        { "CLEAN GLUE",        {  0, -18,  3.0f, 15.f, 120, 8,  60,   0,   0,  3, 100,  0,  1, 1 } },
        { "UNDERGROUND CRUSH", {  0, -26,  6.0f,  3.f,  90, 4, 100,  35,  10,  9, 100, -1,  0, 0 } },
        { "DARK ROOM",         {  0, -22,  4.0f,  8.f, 180, 10, 80,  20,   0,  6,  85,  0,  1, 1 } },
        { "AIR SMASH",         {  0, -28,  8.0f, 1.5f,  60, 2, 120,  25,  55, 10,  70, -2,  0, 0 } },
        { "AD-LIB GRIT",       {  3, -30, 10.0f, 0.8f,  50, 0, 150,  70,  25, 12,  60, -3,  0, 0 } },
        { "PARALLEL TANK",     {  0, -34, 12.0f,  2.f,  70, 3,  90,  40,  20, 14,  45, -2,  0, 1 } },
    };
    constexpr int numPresets = (int) (sizeof (presets) / sizeof (presets[0]));
}

juce::AudioProcessorValueTreeState::ParameterLayout TwoKProcessor::createLayout()
{
    using F = juce::AudioParameterFloat;
    using Attr = juce::AudioParameterFloatAttributes;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto add = [&p] (const char* id, const char* name, float lo, float hi, float step, float centre, float def, const char* unit)
    {
        juce::NormalisableRange<float> r (lo, hi, step);
        if (centre > lo && centre < hi)
            r.setSkewForCentre (centre);
        p.push_back (std::make_unique<F> (juce::ParameterID { id, 1 }, name, r, def, Attr().withLabel (unit)));
    };

    add ("in",     "Input",       -24.f, 24.f, 0.1f, 0.f,   0.f,   "dB");
    add ("thr",    "Threshold",   -60.f,  0.f, 0.1f, 0.f,  -18.f,  "dB");
    add ("ratio",  "Ratio",         1.f, 20.f, 0.1f, 4.f,   4.f,   ":1");
    add ("atk",    "Attack",       0.1f, 100.f, 0.1f, 8.f,  5.f,   "ms");
    add ("rel",    "Release",      10.f, 1200.f, 1.f, 150.f, 120.f, "ms");
    add ("knee",   "Knee",          0.f, 24.f, 0.1f, 0.f,   6.f,   "dB");
    add ("hpf",    "Sidechain HPF", 20.f, 500.f, 1.f, 100.f, 80.f,  "Hz");
    add ("grit",   "Grit",          0.f, 100.f, 1.f, 0.f,   0.f,   "%");
    add ("air",    "Air",           0.f, 100.f, 1.f, 0.f,   0.f,   "%");
    add ("makeup", "Makeup",        0.f, 24.f, 0.1f, 0.f,   0.f,   "dB");
    add ("mix",    "Mix",           0.f, 100.f, 1.f, 0.f, 100.f,   "%");
    add ("out",    "Output",      -12.f, 12.f, 0.1f, 0.f,   0.f,   "dB");
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "rms", 1 },  "RMS Detector", false));
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "auto", 1 }, "Auto Release",  false));

    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (auto& x : p)
        layout.add (std::move (x));
    return layout;
}

TwoKProcessor::TwoKProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

void TwoKProcessor::prepareToPlay (double sampleRate, int)
{
    comp.prepare (sampleRate);
}

bool TwoKProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

void TwoKProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, buffer.getNumSamples());

    CompParams p;
    p.inputDb = raw ("in");       p.thresholdDb = raw ("thr");  p.ratio = raw ("ratio");
    p.attackMs = raw ("atk");     p.releaseMs = raw ("rel");    p.kneeDb = raw ("knee");
    p.hpfHz = raw ("hpf");        p.gritPct = raw ("grit");     p.airPct = raw ("air");
    p.makeupDb = raw ("makeup");  p.mixPct = raw ("mix");       p.outputDb = raw ("out");
    p.rms = raw ("rms") > 0.5f;   p.autoRelease = raw ("auto") > 0.5f;

    comp.process (buffer, p);
}

int TwoKProcessor::getNumPrograms() { return numPresets; }

void TwoKProcessor::setCurrentProgram (int index)
{
    index = juce::jlimit (0, numPresets - 1, index);
    currentProgram = index;
    for (int k = 0; k < 14; ++k)
        if (auto* prm = apvts.getParameter (paramIds[k]))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 (presets[index].v[k]));
            prm->endChangeGesture();
        }
}

const juce::String TwoKProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, numPresets) ? juce::String (presets[index].name) : juce::String();
}

void TwoKProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void TwoKProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* TwoKProcessor::createEditor() { return new TwoKEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TwoKProcessor(); }
