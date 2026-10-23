#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    const juce::Colour bgCol  (0xff060609);
    const juce::Colour blue   (0xff2f5cff);
    const juce::Colour hot    (0xffff2d55);
    const juce::Colour acid   (0xffd6ff1f);
    const juce::Colour ice    (0xffbfe3ff);

    constexpr int W = 980, H = 620;

    float easeOutExpo (float x) { return x >= 1.0f ? 1.0f : 1.0f - std::pow (2.0f, -10.0f * x); }

    juce::Font bold (float h) { return juce::Font (juce::FontOptions (h, juce::Font::bold)); }
}

// ===========================================================================
//  LookAndFeel
// ===========================================================================
void FxLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                      float startAngle, float endAngle, juce::Slider& s)
{
    const auto col = juce::Colour::fromString (s.getProperties().getWithDefault ("col", "ff2f5cff").toString());
    const float hover = (float) (double) s.getProperties().getWithDefault ("hover", 0.0);
    const int dec = (int) s.getProperties().getWithDefault ("dec", 1);
    const auto suffix = s.getProperties().getWithDefault ("suffix", "").toString();

    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (8.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const juce::PathStrokeType thin (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path track;
    track.addCentredArc (c.x, c.y, r, r, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff1a1c28));
    g.strokePath (track, thin);

    juce::Path val;
    val.addCentredArc (c.x, c.y, r, r, 0.0f, startAngle, angle, true);
    for (int i = 3; i >= 1; --i)
    {
        g.setColour (col.withAlpha (0.07f + 0.05f * hover));
        g.strokePath (val, juce::PathStrokeType (5.0f + (float) i * (4.0f + 3.0f * hover),
                                                 juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setColour (col);
    g.strokePath (val, thin);

    const float br = r * 0.72f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2f42), c.x - br * 0.4f, c.y - br * 0.7f,
                                             juce::Colour (0xff08090e), c.x + br * 0.5f, c.y + br, true));
    g.fillEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f);
    g.setColour (col.withAlpha (0.25f + 0.55f * hover));
    g.drawEllipse (c.x - br, c.y - br, br * 2.0f, br * 2.0f, 1.2f);

    if (hover > 0.01f)
    {
        const float rr = r + 5.0f + hover * 4.0f;
        g.setColour (col.withAlpha (0.35f * hover));
        g.drawEllipse (c.x - rr, c.y - rr, rr * 2.0f, rr * 2.0f, 1.0f);
    }

    juce::Path ptr;
    ptr.addRoundedRectangle (-1.6f, -br + 2.0f, 3.2f, br * 0.42f, 1.6f);
    g.setColour (col.brighter (0.5f));
    g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (c.x, c.y));

    g.setColour (juce::Colours::white.withAlpha (0.55f + 0.4f * hover));
    g.setFont (bold (10.5f));
    g.drawText (juce::String (s.getValue(), dec) + suffix,
                juce::Rectangle<float> (c.x - br, c.y + br * 0.05f, br * 2.0f, 14.0f),
                juce::Justification::centred, false);
}

void FxLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.5f);
    const bool on = b.getToggleState();
    if (on)
    {
        g.setColour (acid.withAlpha (0.18f));
        g.fillRoundedRectangle (r.expanded (3.0f), 7.0f);
    }
    g.setColour (on ? acid : juce::Colour (0xff12141c));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (on ? acid.brighter (0.4f) : juce::Colour (0xff3a3f55));
    g.drawRoundedRectangle (r, 4.0f, over ? 2.0f : 1.2f);
    g.setColour (on ? juce::Colours::black : juce::Colours::white.withAlpha (0.75f));
    g.setFont (bold (11.5f));
    g.drawText (b.getButtonText(), r, juce::Justification::centred, false);
}

// ===========================================================================
//  Editor
// ===========================================================================
juce::Image TwoKEditor::makeGlow (const juce::Image& src, juce::Colour c)
{
    const int pad = 20;
    auto small = src.rescaled (juce::jmax (8, src.getWidth() / 2), juce::jmax (8, src.getHeight() / 2));
    juce::Image a (juce::Image::ARGB, small.getWidth() + pad * 2, small.getHeight() + pad * 2, true);
    juce::Image b (juce::Image::ARGB, a.getWidth(), a.getHeight(), true);
    {
        juce::Graphics g (a);
        g.setColour (c);
        g.drawImage (small, juce::Rectangle<float> ((float) pad, (float) pad, (float) small.getWidth(), (float) small.getHeight()),
                     juce::RectanglePlacement::stretchToFit, true);
    }
    juce::ImageConvolutionKernel k (21);
    k.createGaussianBlur (6.0f);
    k.applyToImage (b, a, a.getBounds());
    k.applyToImage (a, b, b.getBounds());
    return a;
}

TwoKEditor::TwoKEditor (TwoKProcessor& p) : juce::AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    setSize (W, H);

    img = juce::ImageCache::getFromMemory (BinaryData::jxhn2k_png, BinaryData::jxhn2k_pngSize);
    if (img.isValid())
    {
        glow = makeGlow (img, blue);
        const float aspect = (float) img.getWidth() / (float) img.getHeight();
        const float ih = 520.0f, iw = ih * aspect;
        imgRect = { 195.0f - iw * 0.5f, 62.0f, iw, ih };
    }

    // knobs ---------------------------------------------------------------
    struct Def { const char* id; const char* name; const char* suffix; int dec; juce::Colour col; };
    const Def defs[12] = {
        { "in",     "INPUT",   " dB", 1, ice  }, { "thr",  "THRESH",  " dB", 1, hot  },
        { "ratio",  "RATIO",   ":1",  1, hot  }, { "atk",  "ATTACK",  " ms", 1, acid },
        { "rel",    "RELEASE", " ms", 0, acid }, { "knee", "KNEE",    " dB", 1, blue },
        { "hpf",    "SC HPF",  " Hz", 0, blue }, { "grit", "GRIT",    " %",  0, hot  },
        { "air",    "AIR",     " %",  0, ice  }, { "makeup","MAKEUP", " dB", 1, acid },
        { "mix",    "MIX",     " %",  0, blue }, { "out",  "OUTPUT",  " dB", 1, ice  } };

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];
        const auto& d = defs[i];
        k.name = d.name;
        addAndMakeVisible (k.slider);
        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        k.slider.getProperties().set ("col", d.col.toString());
        k.slider.getProperties().set ("suffix", d.suffix);
        k.slider.getProperties().set ("dec", d.dec);
        k.att = std::make_unique<SliderAtt> (proc.apvts, d.id, k.slider);
        if (auto* prm = proc.apvts.getParameter (d.id))
            k.slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
    }

    addAndMakeVisible (rmsBtn);
    addAndMakeVisible (autoBtn);
    rmsAtt  = std::make_unique<ButtonAtt> (proc.apvts, "rms", rmsBtn);
    autoAtt = std::make_unique<ButtonAtt> (proc.apvts, "auto", autoBtn);

    addAndMakeVisible (prevBtn);
    addAndMakeVisible (nextBtn);
    prevBtn.onClick = [this] { const int n = proc.getNumPrograms(); proc.setCurrentProgram ((proc.getCurrentProgram() + n - 1) % n); presetFlash = 1.0f; glitchFrames = 6; };
    nextBtn.onClick = [this] { const int n = proc.getNumPrograms(); proc.setCurrentProgram ((proc.getCurrentProgram() + 1) % n); presetFlash = 1.0f; glitchFrames = 6; };
    for (auto* bt : { &prevBtn, &nextBtn })
    {
        bt->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff12141c));
        bt->setColour (juce::TextButton::textColourOffId, acid);
    }

    // ghost "2K" outline --------------------------------------------------
    {
        juce::GlyphArrangement ga;
        ga.addLineOfText (juce::Font (juce::FontOptions (400.0f, juce::Font::bold)), "2K", 0.0f, 0.0f);
        ga.createPath (ghostPath);
    }

    // ticker --------------------------------------------------------------
    {
        juce::GlyphArrangement ga;
        ga.addLineOfText (bold (13.0f), "JXHN2K  //  2KCOMP  //  UNDERGROUND VOCAL COMPRESSOR  //  NO LIMITS  //  ", 0.0f, 0.0f);
        tickerW = juce::jmax (100.0f, ga.getBoundingBox (0, ga.getNumGlyphs(), true).getWidth());
    }

    // particles -----------------------------------------------------------
    for (int i = 0; i < 70; ++i)
        particles.push_back ({ rng.nextFloat() * W, rng.nextFloat() * H, 0.2f + rng.nextFloat() * 0.8f,
                               1.0f + rng.nextFloat() * 2.5f, rng.nextFloat() * 6.28f, 0.15f + rng.nextFloat() * 0.45f,
                               rng.nextInt (3) });

    startTimerHz (60);
}

TwoKEditor::~TwoKEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void TwoKEditor::resized()
{
    const int x0 = 390, cw = 93, ch = 114, rowY[2] = { 334, 452 };
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int col = (int) (i % 6), row = (int) (i / 6);
        knobs[i].slider.setBounds (x0 + col * cw + 4, rowY[row], 85, 85);
    }
    rmsBtn.setBounds (610, 30, 68, 24);
    autoBtn.setBounds (686, 30, 84, 24);
    prevBtn.setBounds (778, 30, 26, 24);
    nextBtn.setBounds (932, 30, 26, 24);
    juce::ignoreUnused (ch);

    scanlines = juce::Image (juce::Image::ARGB, W, H, true);
    juce::Graphics g (scanlines);
    g.setColour (juce::Colours::black.withAlpha (0.16f));
    for (int y = 0; y < H; y += 3)
        g.fillRect (0, y, W, 1);
}

// ===========================================================================
//  Animation tick
// ===========================================================================
void TwoKEditor::timerCallback()
{
    t += 1.0f / 60.0f;
    ++frame;

    auto follow = [] (float& s, float v, float up, float down) { s += (v - s) * (v > s ? up : down); };
    const float inDb = proc.comp.meterIn.load(), outDb = proc.comp.meterOut.load(), gr = -proc.comp.meterGr.load();
    follow (inDbS, inDb, 0.6f, 0.12f);
    follow (outDbS, outDb, 0.6f, 0.12f);
    follow (grS, gr, 0.7f, 0.08f);

    if (inDb > inPeak)   { inPeak = inDb;   inHold = 40; }  else if (--inHold < 0)  inPeak -= 0.6f;
    if (outDb > outPeak) { outPeak = outDb; outHold = 40; } else if (--outHold < 0) outPeak -= 0.6f;
    if (gr > grPeak)     { grPeak = gr;     grHold = 40; }  else if (--grHold < 0)  grPeak = juce::jmax (0.0f, grPeak - 0.3f);

    outN = juce::jlimit (0.0f, 1.0f, (outDbS + 60.0f) / 60.0f);
    grN  = juce::jlimit (0.0f, 1.0f, grS / 18.0f);

    // glitch trigger: hard compression hits or a random twitch
    if (grS - lastGr > 2.5f || (t > 3.0f && rng.nextInt (420) == 0))
        glitchFrames = 5 + rng.nextInt (9);
    lastGr = grS;
    if (glitchFrames > 0)
    {
        --glitchFrames;
        if ((frame & 1) == 0)
            for (int i = 0; i < 6; ++i)
            {
                stripY[i]  = rng.nextFloat();
                stripH[i]  = 8.0f + rng.nextFloat() * 42.0f;
                stripDx[i] = (rng.nextFloat() - 0.5f) * 70.0f;
            }
    }

    if ((frame % 2) == 0)
    {
        history[(size_t) histPos] = grS;
        histPos = (histPos + 1) % histN;
    }

    for (auto& k : knobs)
    {
        const float target = k.slider.isMouseOverOrDragging (true) ? 1.0f : 0.0f;
        k.hover += (target - k.hover) * 0.25f;
        k.slider.getProperties().set ("hover", (double) k.hover);
    }

    presetFlash = juce::jmax (0.0f, presetFlash - 0.03f);
    gridPhase += 0.006f + outN * 0.05f;
    tickerX -= 1.2f + outN * 2.5f;
    if (tickerX < -tickerW) tickerX += tickerW;

    for (auto& pt : particles)
    {
        pt.y -= pt.vy * (1.0f + outN * 4.0f);
        pt.x += std::sin (pt.phase + t) * 0.35f;
        if (pt.y < -10.0f) { pt.y = (float) H + 10.0f; pt.x = rng.nextFloat() * W; }
    }

    // title scramble
    static const juce::String pool ("#@%&$X01/<>*+=?");
    const juce::String target ("2KCOMP");
    juce::String s;
    for (int i = 0; i < target.length(); ++i)
    {
        const bool scr = (t < 0.35f + (float) i * 0.12f) || (glitchFrames > 0 && ((i + frame) % 3 == 0));
        s += scr ? juce::String::charToString (pool[rng.nextInt (pool.length())]) : target.substring (i, i + 1);
    }
    titleText = s;

    repaint();
}

// ===========================================================================
//  Painting
// ===========================================================================
void TwoKEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.fillAll (bgCol);

    const float cx = 200.0f + std::sin (t * 0.5f) * 60.0f;
    g.setGradientFill (juce::ColourGradient (blue.withAlpha (0.22f + 0.30f * outN), cx, 300.0f,
                                             juce::Colours::transparentBlack, cx + 400.0f, 300.0f, true));
    g.fillRect (bounds);
    g.setGradientFill (juce::ColourGradient (hot.withAlpha (0.06f + 0.22f * grN), 900.0f, 60.0f,
                                             juce::Colours::transparentBlack, 900.0f, 420.0f, true));
    g.fillRect (bounds);

    drawFloor (g);
    drawGhost (g);
    drawParticles (g);
    drawPerson (g);

    g.setColour (blue.withAlpha (0.35f));
    g.drawVerticalLine (388, 70.0f, 568.0f);

    drawTitle (g);
    drawDisplay (g);

    // knob labels
    for (auto& k : knobs)
    {
        const auto kb = k.slider.getBounds();
        g.setColour (juce::Colours::white.withAlpha (0.55f + 0.4f * k.hover));
        g.setFont (bold (10.5f));
        g.drawText (k.name, kb.getX(), kb.getBottom() - 2, kb.getWidth(), 14, juce::Justification::centred, false);
    }

    // preset name
    {
        const float flash = presetFlash;
        auto r = juce::Rectangle<float> (806.0f, 30.0f, 124.0f, 24.0f);
        g.setColour (juce::Colour (0xff12141c));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (acid.withAlpha (0.35f + 0.65f * flash));
        g.drawRoundedRectangle (r, 4.0f, 1.2f);
        g.setColour (juce::Colours::white.withAlpha (0.8f + 0.2f * flash));
        g.setFont (bold (10.0f));
        g.drawText (proc.getProgramName (proc.getCurrentProgram()), r.reduced (4.0f, 0.0f), juce::Justification::centred, false);
    }

    drawTicker (g);
}

void TwoKEditor::drawFloor (juce::Graphics& g)
{
    juce::Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (0, 392, 388, 190);
    const float horizon = 392.0f, vx = 195.0f;
    g.setGradientFill (juce::ColourGradient (blue.withAlpha (0.0f), 0.0f, horizon, blue.withAlpha (0.45f + 0.3f * outN), 0.0f, 582.0f, false));
    for (int i = -12; i <= 12; ++i)
        g.drawLine (vx, horizon, vx + (float) i * 70.0f, 600.0f, 1.0f);
    const float scroll = gridPhase - std::floor (gridPhase);
    for (int i = 0; i < 12; ++i)
    {
        const float f = ((float) i + scroll) / 12.0f;
        g.drawHorizontalLine ((int) (horizon + f * f * 190.0f), 0.0f, 388.0f);
    }
}

void TwoKEditor::drawGhost (juce::Graphics& g)
{
    const float ox = -30.0f + std::sin (t * 0.3f) * 22.0f, oy = 470.0f + std::cos (t * 0.4f) * 10.0f;
    const auto tf = juce::AffineTransform::translation (ox, oy);
    g.setColour (blue.withAlpha (0.05f + 0.10f * outN));
    g.fillPath (ghostPath, tf);
    const float sp = 3.0f + grN * 8.0f;
    g.setColour (hot.withAlpha (0.20f));
    g.strokePath (ghostPath, juce::PathStrokeType (1.5f), tf.translated (-sp, 0.0f));
    g.setColour (ice.withAlpha (0.22f));
    g.strokePath (ghostPath, juce::PathStrokeType (1.5f), tf.translated (sp, 0.0f));
}

void TwoKEditor::drawParticles (juce::Graphics& g)
{
    for (auto& pt : particles)
    {
        const juce::Colour c = pt.kind == 0 ? ice : (pt.kind == 1 ? blue.brighter (0.4f) : acid);
        g.setColour (c.withAlpha (pt.alpha * (0.5f + 0.5f * std::sin (pt.phase + t * 2.0f))));
        g.fillEllipse (pt.x, pt.y, pt.size, pt.size);
    }
}

void TwoKEditor::drawPerson (juce::Graphics& g)
{
    if (! img.isValid())
        return;

    const float intro = easeOutExpo (juce::jlimit (0.0f, 1.0f, t / 1.5f));
    const auto r = imgRect.translated (0.0f, (1.0f - intro) * 130.0f + std::sin (t * 1.7f) * 5.0f);
    const float tilt = std::sin (t * 0.8f) * 0.012f + grN * 0.02f * std::sin (t * 40.0f);

    // floor shadow
    g.setColour (blue.withAlpha (0.35f * intro));
    g.fillEllipse (r.getCentreX() - 110.0f, r.getBottom() - 6.0f, 220.0f, 18.0f);

    juce::Graphics::ScopedSaveState ss (g);
    g.addTransform (juce::AffineTransform::rotation (tilt, r.getCentreX(), r.getBottom() - 40.0f));

    // glow
    if (glow.isValid())
    {
        g.setOpacity (intro * (0.35f + 0.65f * outN));
        g.drawImage (glow, r.expanded (40.0f), juce::RectanglePlacement::stretchToFit);
    }

    // RGB split
    const float sp = 2.0f + grN * 14.0f + (glitchFrames > 0 ? 14.0f : 0.0f);
    g.setColour (hot.withAlpha (0.55f * intro));
    g.drawImage (img, r.translated (-sp, 0.0f), juce::RectanglePlacement::stretchToFit, true);
    g.setColour (ice.withAlpha (0.55f * intro));
    g.drawImage (img, r.translated (sp, 0.0f), juce::RectanglePlacement::stretchToFit, true);

    // main
    g.setOpacity (intro);
    g.drawImage (img, r, juce::RectanglePlacement::stretchToFit);

    // glitch slices
    if (glitchFrames > 0)
        for (int i = 0; i < 6; ++i)
        {
            juce::Graphics::ScopedSaveState s2 (g);
            const float sy = r.getY() + stripY[i] * r.getHeight();
            g.reduceClipRegion (juce::Rectangle<int> ((int) r.getX() - 80, (int) sy, (int) r.getWidth() + 160, (int) stripH[i]));
            if (i % 3 == 0)
            {
                g.setColour (acid.withAlpha (0.8f));
                g.drawImage (img, r.translated (stripDx[i], 0.0f), juce::RectanglePlacement::stretchToFit, true);
            }
            else
            {
                g.setOpacity (1.0f);
                g.drawImage (img, r.translated (stripDx[i], 0.0f), juce::RectanglePlacement::stretchToFit);
            }
        }
}

void TwoKEditor::drawTitle (juce::Graphics& g)
{
    auto f = bold (54.0f);
    f.setExtraKerningFactor (0.06f);
    g.setFont (f);
    const juce::Rectangle<float> r (390.0f, 6.0f, 220.0f, 58.0f);
    const float j = glitchFrames > 0 ? 3.0f : 1.0f;
    g.setColour (hot.withAlpha (0.8f));
    g.drawText (titleText, r.translated (-2.0f * j, 0.0f), juce::Justification::centredLeft, false);
    g.setColour (ice.withAlpha (0.8f));
    g.drawText (titleText, r.translated (2.0f * j, 0.0f), juce::Justification::centredLeft, false);
    g.setColour (juce::Colours::white);
    g.drawText (titleText, r, juce::Justification::centredLeft, false);

    g.setColour (ice.withAlpha (0.55f));
    g.setFont (bold (10.5f));
    g.drawText ("JXHN2K // UNDERGROUND VOCAL COMPRESSOR", 392, 64, 330, 14, juce::Justification::centredLeft, false);
}

void TwoKEditor::drawDisplay (juce::Graphics& g)
{
    const juce::Rectangle<float> d (390.0f, 96.0f, 300.0f, 226.0f);
    g.setColour (juce::Colour (0xcc0c0d14));
    g.fillRoundedRectangle (d, 8.0f);
    g.setColour (blue.withAlpha (0.5f));
    g.drawRoundedRectangle (d, 8.0f, 1.2f);

    // moving highlight along the border
    {
        const float k = std::fmod (t * 0.25f, 1.0f);
        g.setColour (acid.withAlpha (0.9f));
        g.fillRect (d.getX() + 8.0f + k * (d.getWidth() - 56.0f), d.getY() - 0.5f, 40.0f, 2.0f);
    }

    const auto c = d.withTrimmedBottom (62.0f).reduced (14.0f, 12.0f);
    auto mapX = [&] (float db) { return c.getX() + (db + 60.0f) / 60.0f * c.getWidth(); };
    auto mapY = [&] (float db) { return c.getBottom() - (db + 60.0f) / 60.0f * c.getHeight(); };

    g.setColour (juce::Colours::white.withAlpha (0.07f));
    for (int db = -48; db <= -12; db += 12)
    {
        g.drawVerticalLine ((int) mapX ((float) db), c.getY(), c.getBottom());
        g.drawHorizontalLine ((int) mapY ((float) db), c.getX(), c.getRight());
    }
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawLine (mapX (-60.0f), mapY (-60.0f), mapX (0.0f), mapY (0.0f), 1.0f);

    const float thr = proc.raw ("thr"), ratio = proc.raw ("ratio"), knee = proc.raw ("knee"), mk = proc.raw ("makeup");
    auto transfer = [&] (float db) { return juce::jlimit (-60.0f, 0.0f, db + VocalCompressor::computeGainDb (db, thr, ratio, knee) + mk); };

    // threshold marker
    g.setColour (hot.withAlpha (0.45f));
    g.drawVerticalLine ((int) mapX (thr), c.getY(), c.getBottom());

    juce::Path curve;
    for (int db = -60; db <= 0; ++db)
    {
        const float px = mapX ((float) db), py = mapY (transfer ((float) db));
        if (db == -60) curve.startNewSubPath (px, py); else curve.lineTo (px, py);
    }
    for (int i = 3; i >= 1; --i)
    {
        g.setColour (blue.withAlpha (0.10f));
        g.strokePath (curve, juce::PathStrokeType (2.0f + (float) i * 3.0f));
    }
    g.setColour (juce::Colours::white);
    g.strokePath (curve, juce::PathStrokeType (2.0f));

    // live dot
    const float lx = juce::jlimit (-60.0f, 0.0f, inDbS);
    if (inDbS > -59.0f)
    {
        const float px = mapX (lx), py = mapY (transfer (lx)), p1 = mapY (lx);
        g.setColour (hot.withAlpha (0.6f));
        g.drawLine (px, p1, px, py, 1.5f);
        const float pulse = 5.0f + 3.0f * std::sin (t * 12.0f) * grN;
        g.setColour (acid.withAlpha (0.25f));
        g.fillEllipse (px - pulse * 2.0f, py - pulse * 2.0f, pulse * 4.0f, pulse * 4.0f);
        g.setColour (acid);
        g.fillEllipse (px - 3.5f, py - 3.5f, 7.0f, 7.0f);
    }

    g.setColour (ice.withAlpha (0.5f));
    g.setFont (bold (9.5f));
    g.drawText ("TRANSFER", (int) d.getX() + 10, (int) d.getY() + 3, 80, 12, juce::Justification::left, false);

    // GR history
    const auto h = juce::Rectangle<float> (d.getX() + 14.0f, d.getBottom() - 52.0f, d.getWidth() - 28.0f, 40.0f);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.fillRoundedRectangle (h, 3.0f);
    juce::Path hp;
    hp.startNewSubPath (h.getX(), h.getY());
    for (int i = 0; i < histN; ++i)
    {
        const float v = juce::jlimit (0.0f, 24.0f, history[(size_t) ((histPos + i) % histN)]);
        hp.lineTo (h.getX() + (float) i / (histN - 1) * h.getWidth(), h.getY() + v / 24.0f * h.getHeight());
    }
    hp.lineTo (h.getRight(), h.getY());
    hp.closeSubPath();
    g.setGradientFill (juce::ColourGradient (hot.withAlpha (0.85f), 0.0f, h.getY(), hot.withAlpha (0.1f), 0.0f, h.getBottom(), false));
    g.fillPath (hp);
    g.setColour (ice.withAlpha (0.5f));
    g.drawText ("GR HISTORY", (int) h.getX() + 4, (int) h.getBottom() - 12, 80, 11, juce::Justification::left, false);

    // meters
    const float top = 96.0f, hgt = 226.0f;
    drawBar (g, { 708.0f, top, 72.0f, hgt }, (inDbS + 60.0f) / 60.0f, (inPeak + 60.0f) / 60.0f, false,
             "IN", juce::String (juce::jmax (-99.9f, inDbS), 1));
    drawBar (g, { 788.0f, top, 72.0f, hgt }, grS / 24.0f, grPeak / 24.0f, true,
             "GR", "-" + juce::String (grS, 1));
    drawBar (g, { 868.0f, top, 72.0f, hgt }, (outDbS + 60.0f) / 60.0f, (outPeak + 60.0f) / 60.0f, false,
             "OUT", juce::String (juce::jmax (-99.9f, outDbS), 1));
}

void TwoKEditor::drawBar (juce::Graphics& g, juce::Rectangle<float> r, float norm, float peakNorm, bool fromTop,
                          const juce::String& label, const juce::String& readout)
{
    g.setColour (juce::Colour (0xcc0c0d14));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (blue.withAlpha (0.35f));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);

    const auto bar = r.reduced (8.0f, 8.0f).withTrimmedBottom (28.0f);
    constexpr int segs = 26;
    const float segH = bar.getHeight() / (float) segs;
    norm = juce::jlimit (0.0f, 1.0f, norm);
    peakNorm = juce::jlimit (0.0f, 1.0f, peakNorm);

    for (int row = 0; row < segs; ++row) // row 0 = top of the screen
    {
        const float frac = fromTop ? (float) (row + 1) / (float) segs
                                   : (float) (segs - row) / (float) segs;
        const juce::Rectangle<float> seg (bar.getX(), bar.getY() + (float) row * segH + 1.0f, bar.getWidth(), segH - 2.0f);
        juce::Colour c;
        if (fromTop)
            c = acid.interpolatedWith (hot, frac);
        else
            c = frac < 0.62f ? blue : (frac < 0.88f ? acid : hot);

        const bool lit = frac <= norm;
        const bool pk = std::abs (frac - peakNorm) < 0.5f / (float) segs;
        g.setColour (pk ? juce::Colours::white : (lit ? c : juce::Colour (0xff141621)));
        g.fillRoundedRectangle (seg, 1.5f);
    }

    g.setColour (ice.withAlpha (0.75f));
    g.setFont (bold (11.0f));
    g.drawText (label, (int) r.getX(), (int) r.getBottom() - 26, (int) r.getWidth(), 12, juce::Justification::centred, false);
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.setFont (bold (9.5f));
    g.drawText (readout, (int) r.getX(), (int) r.getBottom() - 14, (int) r.getWidth(), 11, juce::Justification::centred, false);
}

void TwoKEditor::drawTicker (juce::Graphics& g)
{
    const juce::Rectangle<float> band (0.0f, 580.0f, (float) W, 40.0f);
    g.setColour (juce::Colour (0xee050509));
    g.fillRect (band);
    g.setColour (blue.withAlpha (0.6f));
    g.drawHorizontalLine (580, 0.0f, (float) W);

    g.setFont (bold (13.0f));
    const juce::String text ("JXHN2K  //  2KCOMP  //  UNDERGROUND VOCAL COMPRESSOR  //  NO LIMITS  //  ");
    for (float x = tickerX; x < (float) W; x += tickerW)
    {
        g.setColour (ice.withAlpha (0.55f));
        g.drawSingleLineText (text, (int) x, 604);
    }
}

void TwoKEditor::paintOverChildren (juce::Graphics& g)
{
    if (scanlines.isValid())
        g.drawImageAt (scanlines, 0, 0);

    // film grain
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    for (int i = 0; i < 140; ++i)
        g.fillRect (rng.nextInt (W), rng.nextInt (H), 1 + rng.nextInt (2), 1);

    // vignette
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, W * 0.5f, H * 0.5f,
                                             juce::Colour (0xaa000000), 0.0f, 0.0f, true));
    g.fillAll();

    // glitch band
    if (glitchFrames > 0)
    {
        const int y = rng.nextInt (H - 10);
        g.setColour (hot.withAlpha (0.14f));
        g.fillRect (0, y, W, 6);
        g.setColour (acid.withAlpha (0.10f));
        g.fillRect (0, y + 8, W, 2);
    }
}
