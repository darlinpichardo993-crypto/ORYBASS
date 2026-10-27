#pragma once
#include "Common.h"

// ============================================================================
//  ORYMY Bass  -  motor DSP
//  Cadena: Gate > Comp > Tone > Split(+Sub) > Drive/Amp (2x oversampling)
//          > Master > EQ grafico (+Auto) > Cabina > Modulacion > Delay > Reverb
// ============================================================================
namespace orymy
{
constexpr double kPi = 3.14159265358979323846;
inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
inline float gainToDb (float g) noexcept  { return 20.0f * std::log10 (std::max (g, 1.0e-7f)); }

// ---------------------------------------------------------------- Biquad (RBJ)
struct Biquad
{
    enum Type { LP, HP, Peak, LowShelf, HighShelf, BP };
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    void reset() { z1 = z2 = 0; }
    void set (Type t, double fs, double f, double q, double gdb = 0.0)
    {
        f = juce::jlimit (10.0, fs * 0.49, f);
        q = std::max (0.1, q);
        const double w0 = 2.0 * kPi * f / fs, c = std::cos (w0), s = std::sin (w0), al = s / (2.0 * q);
        const double A = std::pow (10.0, gdb / 40.0), beta = 2.0 * std::sqrt (A) * al;
        double B0 = 1, B1 = 0, B2 = 0, A0 = 1, A1 = 0, A2 = 0;
        switch (t)
        {
            case LP: B0 = (1 - c) / 2; B1 = 1 - c; B2 = B0; A0 = 1 + al; A1 = -2 * c; A2 = 1 - al; break;
            case HP: B0 = (1 + c) / 2; B1 = -(1 + c); B2 = B0; A0 = 1 + al; A1 = -2 * c; A2 = 1 - al; break;
            case BP: B0 = al; B1 = 0; B2 = -al; A0 = 1 + al; A1 = -2 * c; A2 = 1 - al; break;
            case Peak: B0 = 1 + al * A; B1 = -2 * c; B2 = 1 - al * A; A0 = 1 + al / A; A1 = -2 * c; A2 = 1 - al / A; break;
            case LowShelf:
                B0 = A * ((A + 1) - (A - 1) * c + beta); B1 = 2 * A * ((A - 1) - (A + 1) * c);
                B2 = A * ((A + 1) - (A - 1) * c - beta); A0 = (A + 1) + (A - 1) * c + beta;
                A1 = -2 * ((A - 1) + (A + 1) * c); A2 = (A + 1) + (A - 1) * c - beta; break;
            case HighShelf:
                B0 = A * ((A + 1) + (A - 1) * c + beta); B1 = -2 * A * ((A - 1) + (A + 1) * c);
                B2 = A * ((A + 1) + (A - 1) * c - beta); A0 = (A + 1) - (A - 1) * c + beta;
                A1 = 2 * ((A - 1) - (A + 1) * c); A2 = (A + 1) - (A - 1) * c - beta; break;
        }
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
    inline float process (float x) noexcept
    {
        const double xi = x, y = b0 * xi + z1;
        z1 = b1 * xi - a1 * y + z2;
        z2 = b2 * xi - a2 * y;
        return (float) y;
    }
};

// ---------------------------------------------------------------- Delay line
struct DelayLine
{
    std::vector<float> buf; int w = 0, size = 1;
    void prepare (int maxSamples) { size = maxSamples + 8; buf.assign ((size_t) size, 0.0f); w = 0; }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); }
    inline void push (float x) noexcept { buf[(size_t) w] = x; if (++w >= size) w = 0; }
    inline float read (float d) const noexcept   // d >= 1 muestras
    {
        float pos = (float) w - d;
        int i0 = (int) std::floor (pos);
        const float fr = pos - (float) i0;
        i0 %= size; if (i0 < 0) i0 += size;
        int i1 = i0 + 1; if (i1 >= size) i1 = 0;
        return buf[(size_t) i0] * (1.0f - fr) + buf[(size_t) i1] * fr;
    }
};

// ---------------------------------------------------------------- Noise gate
struct NoiseGate
{
    float env = 0, gain = 1, relEnv = 0.999f, aCoef = 0.1f, rCoef = 0.001f; int hold = 0, holdSamples = 2400; bool open = true;
    void prepare (double sr)
    {
        env = 0; gain = 1; hold = 0; open = true;
        relEnv = std::exp (-1.0f / float (0.04 * sr));
        aCoef = 1.0f - std::exp (-1.0f / float (0.002 * sr));
        rCoef = 1.0f - std::exp (-1.0f / float (0.09 * sr));
        holdSamples = int (0.05 * sr);
    }
    inline float process (float x, float thrLin, float rangeLin) noexcept
    {
        const float a = std::abs (x);
        env = a > env ? a : env * relEnv;
        if (env > thrLin) { open = true; hold = holdSamples; }
        else if (env < thrLin * 0.6f) { if (hold > 0) --hold; else open = false; }
        const float target = open ? 1.0f : rangeLin;
        gain += (target - gain) * (target > gain ? aCoef : rCoef);
        return x * gain;
    }
};

// ---------------------------------------------------------------- Compresor
struct Compressor
{
    float grDb = 0, aCoef = 0.01f, rCoef = 0.0005f;
    void prepare (double sr)
    {
        grDb = 0;
        aCoef = 1.0f - std::exp (-1.0f / float (0.012 * sr));
        rCoef = 1.0f - std::exp (-1.0f / float (0.14 * sr));
    }
    inline float process (float x, float thr, float ratio, float makeupLin) noexcept
    {
        const float over = gainToDb (std::abs (x)) - thr, knee = 6.0f, k = 1.0f - 1.0f / ratio;
        float target;
        if (over <= -knee * 0.5f) target = 0.0f;
        else if (over >= knee * 0.5f) target = -over * k;
        else { const float t = over + knee * 0.5f; target = -k * t * t / (2.0f * knee); }
        grDb += (target - grDb) * (target < grDb ? aCoef : rCoef);
        return x * dbToGain (grDb) * makeupLin;
    }
};

// ---------------------------------------------------------------- Sub octava
struct SubOctave
{
    Biquad lp1, lp2, o1, o2; float env = 0, relEnv = 0.999f; bool pos = false, tog = false;
    void prepare (double sr)
    {
        lp1.set (Biquad::LP, sr, 160, 0.707); lp2.set (Biquad::LP, sr, 160, 0.707);
        o1.set (Biquad::LP, sr, 100, 0.707);  o2.set (Biquad::LP, sr, 100, 0.707);
        lp1.reset(); lp2.reset(); o1.reset(); o2.reset();
        env = 0; pos = tog = false; relEnv = std::exp (-1.0f / float (0.03 * sr));
    }
    inline float process (float x) noexcept
    {
        const float f = lp2.process (lp1.process (x));
        const float a = std::abs (f);
        env = a > env ? a : env * relEnv;
        const float th = 0.12f * env + 1.0e-4f;
        if (! pos && f > th) { pos = true; tog = ! tog; }
        else if (pos && f < -th) pos = false;
        const float sq = (env > 2.0e-4f) ? (tog ? 1.0f : -1.0f) : 0.0f;
        return o2.process (o1.process (sq * env * 1.6f));
    }
};

struct AllPass
{
    float z = 0;
    inline float process (float x, float a) noexcept { const float y = a * x + z; z = x - a * y; return y; }
};

// ---------------------------------------------------------------- Definiciones
struct AmpDef { float baseDb, bias, lsF, lsG, pkF, pkQ, pkG, hsF, hsG, lpF; };
inline const AmpDef kAmps[4] = {
    {  0.f, 0.10f,  80.f, 2.f,  800.f, 0.9f, 3.f, 3500.f, -1.f,  9000.f },  // Bass Amp
    {  2.f, 0.25f, 110.f, 3.f,  500.f, 0.8f, 2.f, 3000.f, -3.f,  6000.f },  // Vintage Tube
    { -6.f, 0.00f,  70.f, 1.f, 2500.f, 1.0f, 2.f, 6000.f,  2.f, 16000.f },  // Modern Clean
    {  8.f, 0.05f, 100.f, 1.f, 1000.f, 1.0f, 4.f, 4000.f,  0.f,  7500.f }   // Grit
};
struct CabDef { float hp, lp, f1, q1, g1, f2, q2, g2, f3, q3, g3; };
inline const CabDef kCabs[4] = {
    { 40.f, 5200.f, 110.f, 1.0f,  3.f,  800.f, 1.0f,  2.f, 2400.f, 2.0f, -3.f },  // 4x10 Bass
    { 48.f, 3800.f,  90.f, 1.2f,  4.f,  600.f, 1.0f,  2.f, 1800.f, 1.5f, -4.f },  // 1x15 Vintage
    { 60.f, 6500.f, 150.f, 1.0f,  2.f, 1200.f, 1.0f,  3.f, 3500.f, 2.0f, -3.f },  // 2x10 Tight
    { 35.f, 4500.f,  70.f, 1.0f,  5.f,  400.f, 1.0f, -1.f, 2000.f, 1.5f, -2.f }   // 8x10 Stack
};

// ---------------------------------------------------------------- Etapa de ampli
struct AmpStage
{
    juce::dsp::Oversampling<float> os { 1, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false };
    Biquad preHP, ls, pk, hs, lp;
    double fs = 48000; float kPrev = 1.0f; int model = -1; float dcX = 0, dcY = 0;

    void prepare (double sr, int maxBlock)
    {
        fs = sr; os.initProcessing ((size_t) maxBlock); os.reset();
        preHP.set (Biquad::HP, sr, 28.0, 0.707); preHP.reset();
        ls.reset(); pk.reset(); hs.reset(); lp.reset();
        model = -1; kPrev = 1.0f; dcX = dcY = 0;
    }
    float latency() const { return os.getLatencyInSamples(); }

    void process (float* x, int n, int m, float gain, float drive)
    {
        m = juce::jlimit (0, 3, m);
        const AmpDef& d = kAmps[m];
        if (m != model)
        {
            model = m;
            ls.set (Biquad::LowShelf, fs, d.lsF, 0.7, d.lsG);
            pk.set (Biquad::Peak, fs, d.pkF, d.pkQ, d.pkG);
            hs.set (Biquad::HighShelf, fs, d.hsF, 0.7, d.hsG);
            lp.set (Biquad::LP, fs, d.lpF, 0.7);
        }
        const float kTarget = dbToGain (d.baseDb + gain * 1.8f + drive * 2.4f);
        for (int i = 0; i < n; ++i) x[i] = preHP.process (x[i]);

        float* ch[1] = { x };
        juce::dsp::AudioBlock<float> blk (ch, 1, (size_t) n);
        auto up = os.processSamplesUp (blk);
        float* u = up.getChannelPointer (0);
        const size_t un = up.getNumSamples();
        const float bias = d.bias, tb = std::tanh (bias);
        for (size_t j = 0; j < un; ++j)
        {
            const float t = (float) j / (float) un;
            const float k = kPrev + (kTarget - kPrev) * t;
            const float s = u[j] * k;
            float v;
            switch (m)
            {
                case 0: case 1: v = std::tanh (s + bias) - tb; break;
                case 2:         v = std::tanh (s); break;
                default:        { const float q = s * 1.6f; v = q > 0 ? q / (1.0f + q) : q / (1.0f - 0.7f * q); break; }
            }
            u[j] = v / std::sqrt (1.0f + 0.25f * std::max (k - 1.0f, -0.5f));
        }
        os.processSamplesDown (blk);
        kPrev = kTarget;

        for (int i = 0; i < n; ++i)
        {
            const float in = x[i];
            dcY = in - dcX + 0.995f * dcY; dcX = in;           // bloqueo de DC (asimetria)
            x[i] = lp.process (hs.process (pk.process (ls.process (dcY))));
        }
    }
};

// ---------------------------------------------------------------- Cabina
struct Cabinet
{
    Biquad hp, lp1, lp2, pk[3]; int model = -1; double fs = 48000;
    void prepare (double sr) { fs = sr; model = -1; hp.reset(); lp1.reset(); lp2.reset(); for (auto& p : pk) p.reset(); }
    void set (int m)
    {
        if (m == model || m < 0 || m > 3) { model = (m > 3 ? m : model); return; }
        model = m; const CabDef& c = kCabs[m];
        hp.set (Biquad::HP, fs, c.hp, 0.707);
        lp1.set (Biquad::LP, fs, c.lp, 0.707); lp2.set (Biquad::LP, fs, c.lp, 0.6);
        pk[0].set (Biquad::Peak, fs, c.f1, c.q1, c.g1);
        pk[1].set (Biquad::Peak, fs, c.f2, c.q2, c.g2);
        pk[2].set (Biquad::Peak, fs, c.f3, c.q3, c.g3);
    }
    inline float process (float x) noexcept
    {
        x = hp.process (x);
        x = pk[0].process (x); x = pk[1].process (x); x = pk[2].process (x);
        return lp2.process (lp1.process (x));
    }
};

// ---------------------------------------------------------------- Parametros
struct EngineParams
{
    bool gateOn = true; float gateThr = -50, gateRange = -40;
    bool compOn = true; float compThr = -18, compRatio = 3, compMakeup = 3;
    bool toneOn = true; float tone = 5;
    bool splitOn = false; float splitFreq = 180;
    bool subOn = false; float subLevel = 3;
    float eq[kNumEq] = {}; int eqMode = 0;
    int ampModel = 0; float gain = 4, master = 7, drive = 0;
    int cabModel = 0; float cabMix = 100;
    bool modOn = false; int modType = 0; float modAmt = 4;
    bool delayOn = false; float delayMs = 300; int delaySync = 3;
    bool reverbOn = false; int reverbType = 0; float reverbMix = 15;
    double bpm = 120.0;
};

// ============================================================================
class BassEngine
{
public:
    float grDbMax = 0.0f;       // metering
    bool gateOpen = true;

    void prepare (double sr, int maxBlock)
    {
        fs = sr; maxN = maxBlock;
        for (auto* v : { &low, &high, &sum, &rvL, &rvR, &outLv, &outRv }) v->assign ((size_t) maxBlock, 0.0f);
        gate.prepare (sr); comp.prepare (sr); sub.prepare (sr); amp.prepare (sr, maxBlock); cab.prepare (sr);
        toneLo.reset(); toneHi.reset(); toneLoG = toneHiG = 1000.0f;
        for (auto& b : xo) b.reset();
        for (auto& b : eqF) b.reset();
        for (auto& v : eqLast) v = 1000.0f;
        tiltLs.reset(); tiltHs.reset(); tiltDb = 0; tiltApplied = 1000.0f;
        aLow.set (Biquad::BP, sr, 120, 1.0); aHigh.set (Biquad::BP, sr, 2500, 1.0); aLow.reset(); aHigh.reset();
        eLow = eHigh = 0;
        splitLast = -1;
        modDl.prepare ((int) (sr * 0.03)); modHP.set (Biquad::HP, sr, 150, 0.707); modHP.reset();
        lfo = 0; fbState = 0; for (auto& a : ap) for (auto& s : a) s = {};
        dl[0].prepare ((int) (sr * 2.1)); dl[1].prepare ((int) (sr * 2.1));
        for (int c = 0; c < 2; ++c) { dlLP[c].set (Biquad::LP, sr, 3500, 0.707); dlHP[c].set (Biquad::HP, sr, 110, 0.707); dlLP[c].reset(); dlHP[c].reset(); }
        dlTime.reset (sr, 0.08); dlTime.setCurrentAndTargetValue ((float) (0.3 * sr));
        dlFbL = dlFbR = 0;
        reverb.setSampleRate (sr); reverb.reset(); rvHP.set (Biquad::HP, sr, 200, 0.707); rvHP.reset(); rvType = -1;
        masterPrev = 1.0f;
    }
    float getLatency() const { return amp.latency(); }

    void process (const float* in, float* outL, float* outR, int n, const EngineParams& p)
    {
        grDbMax = 0.0f;
        updateStatic (p);

        // 1) Gate / Comp / Tone -------------------------------------------------
        const float gThr = dbToGain (p.gateThr), gRng = dbToGain (p.gateRange), cMk = dbToGain (p.compMakeup);
        for (int i = 0; i < n; ++i)
        {
            float x = in[i];
            if (p.gateOn) x = gate.process (x, gThr, gRng);
            if (p.compOn) { x = comp.process (x, p.compThr, p.compRatio, cMk); grDbMax = std::max (grDbMax, -comp.grDb); }
            if (p.toneOn) x = toneHi.process (toneLo.process (x));
            sum[(size_t) i] = x;
        }
        gateOpen = ! p.gateOn || gate.open;

        // 2) Split + Sub --------------------------------------------------------
        const float subG = p.subOn ? p.subLevel / 10.0f * 1.3f : 0.0f;
        for (int i = 0; i < n; ++i)
        {
            const float x = sum[(size_t) i];
            float lo = 0.0f, hi = x;
            if (p.splitOn) { lo = xo[1].process (xo[0].process (x)); hi = xo[3].process (xo[2].process (x)); }
            if (p.subOn) lo += sub.process (x) * subG;
            low[(size_t) i] = lo; high[(size_t) i] = hi;
        }

        // 3) Drive + Amp (oversampling) ----------------------------------------
        amp.process (high.data(), n, p.ampModel, p.gain, p.drive);

        // 4) Master + EQ + Cabina ----------------------------------------------
        const float mDb = p.master < 0.05f ? -100.0f : juce::jmap (p.master, 0.0f, 10.0f, -40.0f, 6.0f);
        const float mTarget = dbToGain (mDb);
        cab.set (p.cabModel);
        const bool cabOn = p.cabModel >= 0 && p.cabModel <= 3;
        const float cm = p.cabMix / 100.0f;
        for (int i = 0; i < n; ++i)
        {
            const float t = (float) (i + 1) / (float) n;
            const float mg = masterPrev + (mTarget - masterPrev) * t;
            float y = (high[(size_t) i] + low[(size_t) i]) * mg;
            for (int b = 0; b < kNumEq; ++b) if (eqActive[b]) y = eqF[b].process (y);
            if (p.eqMode == 0 || std::abs (tiltDb) > 0.01f) y = tiltHs.process (tiltLs.process (y));
            // analisis para el EQ automatico
            const float l = aLow.process (y), h = aHigh.process (y);
            eLow += (l * l - eLow) * 0.0004f; eHigh += (h * h - eHigh) * 0.0004f;
            if (cabOn) { const float w = cab.process (y); y = y * (1.0f - cm) + w * cm; }
            sum[(size_t) i] = y;
        }
        masterPrev = mTarget;
        autoTilt (p);

        // 5) Efectos (estereo) -------------------------------------------------
        const float amt = p.modAmt / 10.0f;
        const double secPerBeat = 60.0 / juce::jlimit (30.0, 300.0, p.bpm);
        float delSamples = (float) (p.delayMs * 0.001 * fs);
        switch (p.delaySync) { case 1: delSamples = (float) (secPerBeat * fs); break;
                               case 2: delSamples = (float) (secPerBeat * 0.5 * fs); break;
                               case 3: delSamples = (float) (secPerBeat * 0.75 * fs); break;
                               case 4: delSamples = (float) (secPerBeat * 0.25 * fs); break; default: break; }
        delSamples = juce::jlimit (32.0f, (float) (fs * 2.0), delSamples);
        dlTime.setTargetValue (delSamples);

        for (int i = 0; i < n; ++i)
        {
            const float x = sum[(size_t) i];
            float L = x, R = x;
            if (p.modOn) modulate (x, L, R, p.modType, amt);
            const float dt = dlTime.getNextValue();
            if (p.delayOn) delayStep (x, L, R, dt);
            else { dl[0].push (0); dl[1].push (0); }
            outLv[(size_t) i] = L; outRv[(size_t) i] = R;
        }

        if (p.reverbOn && p.reverbMix > 0.5f)
        {
            if (p.reverbType != rvType) { rvType = p.reverbType; setReverb (rvType); }
            for (int i = 0; i < n; ++i) { const float s = rvHP.process (0.5f * (outLv[(size_t) i] + outRv[(size_t) i])); rvL[(size_t) i] = s; rvR[(size_t) i] = s; }
            reverb.processStereo (rvL.data(), rvR.data(), n);
            const float wg = p.reverbMix / 100.0f * 0.22f;
            for (int i = 0; i < n; ++i) { outLv[(size_t) i] += rvL[(size_t) i] * wg; outRv[(size_t) i] += rvR[(size_t) i] * wg; }
        }

        // 6) Limitador suave de seguridad --------------------------------------
        for (int i = 0; i < n; ++i)
        {
            outL[i] = softLimit (outLv[(size_t) i]);
            outR[i] = softLimit (outRv[(size_t) i]);
        }
    }

private:
    double fs = 48000; int maxN = 512;
    NoiseGate gate; Compressor comp; SubOctave sub; AmpStage amp; Cabinet cab;
    std::vector<float> low, high, sum, rvL, rvR, outLv, outRv;
    Biquad toneLo, toneHi; float toneLoG = 1000, toneHiG = 1000;
    Biquad xo[4]; float splitLast = -1;
    Biquad eqF[kNumEq]; float eqLast[kNumEq] = {}; bool eqActive[kNumEq] = {};
    Biquad tiltLs, tiltHs, aLow, aHigh; float tiltDb = 0, tiltApplied = 1000, eLow = 0, eHigh = 0;
    float masterPrev = 1.0f;
    // modulacion
    DelayLine modDl; Biquad modHP; double lfo = 0; float fbState = 0; AllPass ap[2][4];
    // delay
    DelayLine dl[2]; Biquad dlLP[2], dlHP[2]; juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> dlTime; float dlFbL = 0, dlFbR = 0;
    // reverb
    juce::Reverb reverb; Biquad rvHP; int rvType = -1;

    static float softLimit (float y) noexcept
    {
        const float a = std::abs (y);
        if (a < 0.8f) return y;
        const float s = y < 0 ? -1.0f : 1.0f;
        return s * (0.8f + 0.2f * std::tanh ((a - 0.8f) / 0.2f));
    }

    void updateStatic (const EngineParams& p)
    {
        // Tone: inclinacion graves/agudos
        const float t = p.tone;
        const float loG = (5.0f - t) * 1.6f, hiG = (t - 5.0f) * 2.4f;
        if (std::abs (loG - toneLoG) > 0.001f) { toneLoG = loG; toneLo.set (Biquad::LowShelf, fs, 250, 0.7, loG); }
        if (std::abs (hiG - toneHiG) > 0.001f) { toneHiG = hiG; toneHi.set (Biquad::HighShelf, fs, 2500, 0.7, hiG); }
        // Crossover Linkwitz-Riley 4
        if (std::abs (p.splitFreq - splitLast) > 0.01f)
        {
            splitLast = p.splitFreq;
            xo[0].set (Biquad::LP, fs, p.splitFreq, 0.7071); xo[1].set (Biquad::LP, fs, p.splitFreq, 0.7071);
            xo[2].set (Biquad::HP, fs, p.splitFreq, 0.7071); xo[3].set (Biquad::HP, fs, p.splitFreq, 0.7071);
        }
        // EQ grafico
        for (int b = 0; b < kNumEq; ++b)
        {
            if (std::abs (p.eq[b] - eqLast[b]) > 0.001f)
            {
                eqLast[b] = p.eq[b];
                eqF[b].set (Biquad::Peak, fs, kEqFreqs[b], 1.3, p.eq[b]);
            }
            eqActive[b] = std::abs (p.eq[b]) > 0.05f;
        }
    }

    void autoTilt (const EngineParams& p)
    {
        float target = 0.0f;
        if (p.eqMode == 0)
        {
            const float lowDb = 10.0f * std::log10 (eLow + 1.0e-12f), highDb = 10.0f * std::log10 (eHigh + 1.0e-12f);
            if (lowDb > -55.0f) target = juce::jlimit (-3.0f, 3.0f, ((lowDb - highDb) - 24.0f) * 0.2f);
            else target = tiltDb;
        }
        tiltDb += (target - tiltDb) * 0.05f;
        if (std::abs (tiltDb - tiltApplied) > 0.02f)
        {
            tiltApplied = tiltDb;
            tiltLs.set (Biquad::LowShelf, fs, 150, 0.7, -tiltDb * 0.5);
            tiltHs.set (Biquad::HighShelf, fs, 2500, 0.7, tiltDb * 0.5);
        }
    }

    void modulate (float x, float& L, float& R, int type, float amt)
    {
        const double twoPi = 2.0 * kPi;
        const float msToS = (float) (fs * 0.001);
        switch (type)
        {
            default:
            case 0: // Chorus
            {
                lfo += 0.6 / fs; if (lfo >= 1.0) lfo -= 1.0;
                modDl.push (modHP.process (x));
                const float dep = (1.0f + 4.0f * amt) * msToS, base = 12.0f * msToS;
                const float wl = modDl.read (base + dep * (float) std::sin (twoPi * lfo));
                const float wr = modDl.read (base + dep * (float) std::sin (twoPi * lfo + kPi * 0.5));
                const float m = 0.35f + 0.45f * amt;
                L = x + wl * m; R = x + wr * m; break;
            }
            case 1: // Flanger
            {
                lfo += 0.25 / fs; if (lfo >= 1.0) lfo -= 1.0;
                modDl.push (modHP.process (x) + fbState * 0.5f);
                const float dep = (0.5f + 2.5f * amt) * msToS, base = 1.5f * msToS;
                const float wl = modDl.read (juce::jmax (1.5f, base + dep * (0.5f + 0.5f * (float) std::sin (twoPi * lfo))));
                const float wr = modDl.read (juce::jmax (1.5f, base + dep * (0.5f + 0.5f * (float) std::sin (twoPi * lfo + kPi * 0.5))));
                fbState = wl;
                const float m = 0.4f + 0.4f * amt;
                L = x + wl * m; R = x + wr * m; break;
            }
            case 2: // Phaser
            {
                lfo += 0.4 / fs; if (lfo >= 1.0) lfo -= 1.0;
                float out[2];
                for (int c = 0; c < 2; ++c)
                {
                    const double ph = twoPi * lfo + c * kPi * 0.5;
                    const float f = 250.0f * std::pow (10.0f, (0.5f + 0.5f * (float) std::sin (ph)) * (0.4f + 0.6f * amt));
                    const float tn = std::tan ((float) kPi * f / (float) fs), a = (tn - 1.0f) / (tn + 1.0f);
                    float s = x + phFb[c] * 0.35f;
                    for (int k = 0; k < 4; ++k) s = ap[c][k].process (s, a);
                    phFb[c] = s; out[c] = s;
                }
                const float m = 0.5f;
                L = x * (1.0f - 0.25f * m) + out[0] * m; R = x * (1.0f - 0.25f * m) + out[1] * m; break;
            }
            case 3: // Tremolo
            {
                lfo += 4.2 / fs; if (lfo >= 1.0) lfo -= 1.0;
                const float g = 1.0f - amt * 0.8f * (0.5f + 0.5f * (float) std::sin (twoPi * lfo));
                L = x * g; R = x * g; break;
            }
        }
    }
    float phFb[2] = { 0, 0 };

    void delayStep (float x, float& L, float& R, float dt)
    {
        const float tl = dl[0].read (dt), tr = dl[1].read (dt);
        const float fbR = dlHP[0].process (dlLP[0].process (tr));
        const float fbL = dlHP[1].process (dlLP[1].process (tl));
        dl[0].push (x + fbR * 0.36f);
        dl[1].push (fbL);
        L += tl * 0.28f; R += tr * 0.28f;
    }

    void setReverb (int t)
    {
        juce::Reverb::Parameters rp;
        rp.wetLevel = 1.0f; rp.dryLevel = 0.0f; rp.freezeMode = 0.0f;
        switch (t)
        {
            case 1:  rp.roomSize = 0.82f; rp.damping = 0.35f; rp.width = 1.0f; break;   // Hall
            case 2:  rp.roomSize = 0.60f; rp.damping = 0.20f; rp.width = 1.0f; break;   // Plate
            case 3:  rp.roomSize = 0.28f; rp.damping = 0.70f; rp.width = 0.6f; break;   // Spring
            default: rp.roomSize = 0.35f; rp.damping = 0.50f; rp.width = 0.9f; break;   // Room
        }
        reverb.setParameters (rp);
    }
};
}
