#pragma once
#include "Common.h"

// ============================================================================
//  Anillo sin bloqueo (audio -> interfaz) y analizador:
//  espectro, osciloscopio, deteccion de nota (YIN) y tonalidad (Krumhansl)
// ============================================================================
namespace orymy
{
struct Ring
{
    static constexpr int N = 1 << 16;
    std::vector<float> d = std::vector<float> ((size_t) N, 0.0f);
    std::atomic<int> w { 0 };

    void push (const float* x, int n) noexcept
    {
        int wi = w.load (std::memory_order_relaxed);
        for (int i = 0; i < n; ++i) { d[(size_t) wi] = x[i]; wi = (wi + 1) & (N - 1); }
        w.store (wi, std::memory_order_release);
    }
    void copyLatest (float* dst, int count) const noexcept
    {
        const int wi = w.load (std::memory_order_acquire);
        int s = (wi - count) & (N - 1);
        for (int i = 0; i < count; ++i) { dst[i] = d[(size_t) s]; s = (s + 1) & (N - 1); }
    }
};

inline const char* const kNoteNames[12] = { "C","Db","D","Eb","E","F","Gb","G","Ab","A","Bb","B" };
inline bool pcInMajorScale (int pc, int tonic)
{
    static const bool inScale[12] = { true,false,true,false,true,true,false,true,false,true,false,true };
    return inScale[((pc - tonic) % 12 + 12) % 12];
}

class Analyzer
{
public:
    static constexpr int kBands = 96, kScope = 256, kFFT = 4096;
    float spec[kBands] = {};
    float scope[kScope] = {};
    // nota
    bool voiced = false; int midi = -1; float cents = 0.0f, freq = 0.0f;
    // tonalidad (tonica de la escala mayor relativa). Por defecto Eb mayor / C menor
    int keyMajor = 3; bool keyConfident = false;

    Analyzer() : fft (12)
    {
        for (int i = 0; i < kFFT; ++i) window[(size_t) i] = 0.5f - 0.5f * std::cos (2.0f * juce::MathConstants<float>::pi * (float) i / (float) (kFFT - 1));
    }

    void update (const Ring& in, const Ring& out, double fs)
    {
        if (fs < 8000.0) return;
        updateSpectrum (out, fs);
        updatePitch (in, fs);
    }

private:
    juce::dsp::FFT fft;
    std::array<float, kFFT> window {};
    std::array<float, kFFT * 2> fdata {};
    std::vector<float> tmp, dec, dif, cm;
    float hist[12] = {};
    int stableMidi = -1, stableCount = 0, silence = 0;

    void updateSpectrum (const Ring& out, double fs)
    {
        tmp.resize ((size_t) kFFT);
        out.copyLatest (tmp.data(), kFFT);
        for (int i = 0; i < kFFT; ++i) { fdata[(size_t) i] = tmp[(size_t) i] * window[(size_t) i]; fdata[(size_t) (i + kFFT)] = 0.0f; }
        fft.performFrequencyOnlyForwardTransform (fdata.data());
        const float norm = 4.0f / (float) kFFT;
        const double fLo = 30.0, fHi = juce::jmin (20000.0, fs * 0.48);
        for (int b = 0; b < kBands; ++b)
        {
            const double f0 = fLo * std::pow (fHi / fLo, (double) b / kBands);
            const double f1 = fLo * std::pow (fHi / fLo, (double) (b + 1) / kBands);
            const int k0 = juce::jlimit (1, kFFT / 2 - 1, (int) std::floor (f0 * kFFT / fs));
            const int k1 = juce::jlimit (k0, kFFT / 2 - 1, (int) std::ceil (f1 * kFFT / fs));
            float m = 0.0f;
            for (int k = k0; k <= k1; ++k) m = std::max (m, fdata[(size_t) k]);
            const float db = 20.0f * std::log10 (m * norm + 1.0e-7f) + 2.5f * (float) std::log2 (std::sqrt (f0 * f1) / 30.0);
            const float v = juce::jlimit (0.0f, 1.0f, (db + 88.0f) / 70.0f);
            spec[b] = v > spec[b] ? spec[b] + (v - spec[b]) * 0.6f : spec[b] * 0.86f + v * 0.14f;
        }
        // osciloscopio
        float pk = 0.05f;
        for (int i = 0; i < kScope; ++i)
        {
            const float v = tmp[(size_t) (kFFT - 1024 + i * 4)];
            scope[i] = v; pk = std::max (pk, std::abs (v));
        }
        for (auto& s : scope) s /= pk;
    }

    void updatePitch (const Ring& in, double fs)
    {
        const int D = juce::jmax (1, (int) std::round (fs / 12000.0));
        const int N = 2048, total = N * D;
        if (total > Ring::N) return;
        tmp.resize ((size_t) total);
        in.copyLatest (tmp.data(), total);

        double e = 0.0;
        for (int i = total - 1024; i < total; ++i) e += (double) tmp[(size_t) i] * tmp[(size_t) i];
        const float rms = (float) std::sqrt (e / 1024.0);
        if (rms < 0.004f)
        {
            if (++silence > 8) { voiced = false; midi = -1; stableMidi = -1; stableCount = 0; }
            return;
        }
        silence = 0;

        dec.resize ((size_t) N);
        for (int i = 0; i < N; ++i) { float s = 0; for (int k = 0; k < D; ++k) s += tmp[(size_t) (i * D + k)]; dec[(size_t) i] = s / (float) D; }
        const double fsd = fs / D;
        const int W = 1024, tauMin = juce::jmax (2, (int) (fsd / 450.0)), tauMax = juce::jmin ((int) (fsd / 30.0), N - W - 2);
        dif.assign ((size_t) tauMax + 1, 0.0f); cm.assign ((size_t) tauMax + 1, 1.0f);
        for (int t = 1; t <= tauMax; ++t)
        {
            float s = 0;
            for (int j = 0; j < W; ++j) { const float df = dec[(size_t) j] - dec[(size_t) (j + t)]; s += df * df; }
            dif[(size_t) t] = s;
        }
        float run = 0;
        for (int t = 1; t <= tauMax; ++t) { run += dif[(size_t) t]; cm[(size_t) t] = dif[(size_t) t] * (float) t / std::max (run, 1.0e-12f); }

        int found = -1;
        for (int t = tauMin; t < tauMax - 1; ++t)
            if (cm[(size_t) t] < 0.15f) { while (t + 1 < tauMax - 1 && cm[(size_t) (t + 1)] < cm[(size_t) t]) ++t; found = t; break; }
        if (found < 0)
        {
            int am = tauMin; for (int t = tauMin; t < tauMax - 1; ++t) if (cm[(size_t) t] < cm[(size_t) am]) am = t;
            if (cm[(size_t) am] < 0.25f) found = am; else return;
        }
        double tau = found;
        if (found > 1 && found < tauMax)
        {
            const float s0 = cm[(size_t) found - 1], s1 = cm[(size_t) found], s2 = cm[(size_t) found + 1];
            const float den = 2.0f * (2.0f * s1 - s0 - s2);
            if (std::abs (den) > 1.0e-9f) tau += (s2 - s0) / den;
        }
        const double f = fsd / tau;
        if (f < 28.0 || f > 600.0) return;
        const double m = 69.0 + 12.0 * std::log2 (f / 440.0);
        const int mi = (int) std::lround (m);
        if (mi == stableMidi) ++stableCount; else { stableMidi = mi; stableCount = 1; }
        if (stableCount >= 2)
        {
            voiced = true; midi = mi; cents = (float) ((m - mi) * 100.0); freq = (float) f;
            for (auto& h : hist) h *= 0.997f;
            hist[((mi % 12) + 12) % 12] += 1.0f;
            detectKey();
        }
    }

    void detectKey()
    {
        static const float maj[12] = { 6.35f,2.23f,3.48f,2.33f,4.38f,4.09f,2.52f,5.19f,2.39f,3.66f,2.29f,2.88f };
        static const float mnr[12] = { 6.33f,2.68f,3.52f,5.38f,2.60f,3.53f,2.54f,4.75f,3.98f,2.69f,3.34f,3.17f };
        float total = 0; for (float h : hist) total += h;
        if (total < 10.0f) return;
        keyConfident = true;
        auto corr = [&] (const float* prof, int k)
        {
            float mh = total / 12.0f, mp = 0; for (int i = 0; i < 12; ++i) mp += prof[i]; mp /= 12.0f;
            float num = 0, dh = 0, dp = 0;
            for (int pc = 0; pc < 12; ++pc)
            {
                const float a = hist[pc] - mh, b = prof[((pc - k) % 12 + 12) % 12] - mp;
                num += a * b; dh += a * a; dp += b * b;
            }
            return num / (std::sqrt (dh * dp) + 1.0e-9f);
        };
        float score[12]; int best = 0;
        for (int m = 0; m < 12; ++m)
        {
            score[m] = std::max (corr (maj, m), corr (mnr, (m + 9) % 12));
            if (score[m] > score[best]) best = m;
        }
        if (best != keyMajor && score[best] > score[keyMajor] + 0.06f) keyMajor = best;
    }
};
}
