#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <cmath>

struct CompParams
{
    float inputDb = 0.f, thresholdDb = -18.f, ratio = 4.f, attackMs = 5.f, releaseMs = 120.f, kneeDb = 6.f;
    float makeupDb = 0.f, mixPct = 100.f, hpfHz = 80.f, gritPct = 0.f, airPct = 0.f, outputDb = 0.f;
    bool rms = false, autoRelease = false;
};

// ---------------------------------------------------------------------------
//  2KComp DSP: stereo-linked feed-forward compressor, tuned for rap vocals
//   - sidechain high-pass (keeps plosives / low end from pumping the vocal)
//   - peak or RMS detector
//   - soft knee gain computer (dB domain)
//   - program dependent "dual release" (fast + slow envelope, blended)
//   - GRIT: asymmetric tanh saturation on the wet path
//   - AIR : high band exciter on the wet path
//   - MIX : parallel compression
// ---------------------------------------------------------------------------
class VocalCompressor
{
public:
    static float computeGainDb (float levelDb, float thr, float ratio, float knee) noexcept
    {
        knee = juce::jmax (knee, 0.001f);
        const float over = levelDb - thr;
        float out;
        if (2.0f * over < -knee)
            out = levelDb;
        else if (2.0f * std::abs (over) <= knee)
        {
            const float k = over + knee * 0.5f;
            out = levelDb + (1.0f / ratio - 1.0f) * k * k / (2.0f * knee);
        }
        else
            out = thr + over / ratio;
        return out - levelDb; // <= 0
    }

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        for (auto* s : { &inSm, &makeupSm, &mixSm, &outSm })
        {
            s->reset (sr, 0.03);
            s->setCurrentAndTargetValue (1.0f);
        }
        for (int c = 0; c < 2; ++c)
        {
            hpf[c].setHighPass (sr, 80.0f);
            airHp[c].setHighPass (sr, 6000.0f);
            hpf[c].reset();
            airHp[c].reset();
        }
        lastHpf = 80.0f;
        env1 = env2 = 0.0f;
        rmsState = 0.0f;
    }

    void process (juce::AudioBuffer<float>& buffer, const CompParams& p)
    {
        const int n = buffer.getNumSamples();
        const int nCh = juce::jmin (2, buffer.getNumChannels());
        if (nCh == 0 || n == 0)
            return;

        inSm.setTargetValue (juce::Decibels::decibelsToGain (p.inputDb));
        makeupSm.setTargetValue (juce::Decibels::decibelsToGain (p.makeupDb));
        mixSm.setTargetValue (p.mixPct * 0.01f);
        outSm.setTargetValue (juce::Decibels::decibelsToGain (p.outputDb));

        const float atk  = coef (p.attackMs);
        const float rel1 = coef (p.autoRelease ? p.releaseMs * 0.3f : p.releaseMs);
        const float rel2 = coef (p.autoRelease ? p.releaseMs * 3.0f : p.releaseMs);
        const float rmsC = coef (10.0f);

        const float gritAmt = p.gritPct * 0.01f;
        const float drive   = 1.0f + 4.0f * gritAmt;
        const float bias    = 0.15f * gritAmt;
        const float tb      = std::tanh (bias);
        const float satNorm = 1.0f / (drive * (1.0f - tb * tb));
        const float air     = p.airPct * 0.01f;

        if (p.hpfHz != lastHpf)
        {
            for (int c = 0; c < 2; ++c)
                hpf[c].setHighPass (sr, p.hpfHz);
            lastHpf = p.hpfHz;
        }

        float* ch[2] = { buffer.getWritePointer (0), buffer.getWritePointer (nCh > 1 ? 1 : 0) };
        float inPk = 0.0f, outPk = 0.0f, grMin = 0.0f;

        for (int i = 0; i < n; ++i)
        {
            const float gIn = inSm.getNextValue();
            const float gMk = makeupSm.getNextValue();
            const float mix = mixSm.getNextValue();
            const float gOut = outSm.getNextValue();

            float dry[2] = { 0.0f, 0.0f };
            float sc = 0.0f;
            for (int c = 0; c < nCh; ++c)
            {
                dry[c] = ch[c][i] * gIn;
                inPk = juce::jmax (inPk, std::abs (dry[c]));
                sc = juce::jmax (sc, std::abs (hpf[c].process (dry[c])));
            }

            float det = sc;
            if (p.rms)
            {
                rmsState = rmsC * rmsState + (1.0f - rmsC) * sc * sc;
                det = std::sqrt (rmsState);
            }

            const float levelDb = 20.0f * std::log10 (juce::jmax (det, 1.0e-6f));
            const float gr = computeGainDb (levelDb, p.thresholdDb, p.ratio, p.kneeDb);

            env1 = (gr < env1) ? atk * env1 + (1.0f - atk) * gr : rel1 * env1 + (1.0f - rel1) * gr;
            env2 = (gr < env2) ? atk * env2 + (1.0f - atk) * gr : rel2 * env2 + (1.0f - rel2) * gr;
            const float grFinal = p.autoRelease ? 0.5f * (env1 + env2) : env1;
            grMin = juce::jmin (grMin, grFinal);

            const float gain = juce::Decibels::decibelsToGain (grFinal) * gMk;

            for (int c = 0; c < nCh; ++c)
            {
                float w = dry[c] * gain;

                if (gritAmt > 0.0001f)
                {
                    const float s = (std::tanh (drive * w + bias) - tb) * satNorm;
                    w += gritAmt * (s - w);
                }
                if (air > 0.0001f)
                {
                    const float hf = airHp[c].process (w);
                    w += air * (0.7f * hf + 0.25f * std::tanh (4.0f * hf));
                }

                const float o = (dry[c] * (1.0f - mix) + w * mix) * gOut;
                ch[c][i] = o;
                outPk = juce::jmax (outPk, std::abs (o));
            }
        }

        meterIn.store (juce::Decibels::gainToDecibels (inPk, -100.0f));
        meterOut.store (juce::Decibels::gainToDecibels (outPk, -100.0f));
        meterGr.store (grMin);
    }

    std::atomic<float> meterIn { -100.0f }, meterOut { -100.0f }, meterGr { 0.0f };

private:
    struct Biquad
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

        void setHighPass (double sr, float freq, float q = 0.7071f)
        {
            const double f = juce::jlimit (5.0, sr * 0.45, (double) freq);
            const double w0 = juce::MathConstants<double>::twoPi * f / sr;
            const double cw = std::cos (w0), alpha = std::sin (w0) / (2.0 * q);
            const double a0 = 1.0 + alpha;
            b0 = (float) (((1.0 + cw) * 0.5) / a0);
            b1 = (float) (-(1.0 + cw) / a0);
            b2 = b0;
            a1 = (float) (-2.0 * cw / a0);
            a2 = (float) ((1.0 - alpha) / a0);
        }
        void reset() { z1 = z2 = 0; }
        float process (float x)
        {
            const float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
    };

    float coef (float ms) const
    {
        return std::exp (-1.0f / (juce::jmax (ms, 0.01f) * 0.001f * (float) sr));
    }

    double sr = 44100.0;
    Biquad hpf[2], airHp[2];
    float lastHpf = 80.0f;
    float env1 = 0.0f, env2 = 0.0f, rmsState = 0.0f;
    juce::SmoothedValue<float> inSm, makeupSm, mixSm, outSm;
};
