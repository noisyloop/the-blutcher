#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

namespace blucher {
constexpr int slots = 8, steps = 16;
constexpr double pi = 3.14159265358979323846;
struct Sample {
    std::vector<float> left, right;
    std::vector<int> cuts;
    double rate = 44100;
    void slice(float sensitivity = 0.45f) {
        cuts.clear(); cuts.push_back(0);
        if (left.empty()) return;
        const int hop = std::max(1, int(rate * .005));
        const int spacing = std::max(1, int(rate * .045));
        double previous = 0, average = 0;
        for (int start = 0; start < int(left.size()); start += hop) {
            double energy = 0;
            int end = std::min(start + hop, int(left.size()));
            for (int i = start; i < end; ++i) energy += std::abs(left[size_t(i)]);
            energy /= std::max(1, end - start);
            average = average * .92 + energy * .08;
            if (energy > .008 && energy > previous * (1.1 + (1-sensitivity)*1.6)
                && energy > average * 1.2 && start - cuts.back() >= spacing && cuts.size() < 127) {
                int cut = start;
                // Snap the onset to a nearby zero crossing.
                for (int i = start; i < std::min(start + 128, end); ++i)
                    if (i > 0 && left[size_t(i-1)] * left[size_t(i)] <= 0) { cut = i; break; }
                cuts.push_back(cut);
            }
            previous = energy;
        }
        if (cuts.size() == 1) {
            const int stride = std::max(spacing, int(left.size()/16));
            for (int i = stride; i < int(left.size()) - spacing; i += stride) cuts.push_back(i);
        }
        cuts.push_back(int(left.size()));
    }
    bool loaded() const { return left.size() > 1 && cuts.size() > 1; }
};
struct Step {
    int slot = 0, slice = 0, semitones = 0, repeats = 1;
    float gain = .8f, gate = .9f;
    bool enabled = true, reverse = false, locked = false;
};
struct Pattern {
    std::array<Step, steps> notes{};
    uint32_t seed = 1;
};
struct Locks { bool rhythm=false, source=false, pitch=false, effects=false; };
class Engine {
public:
    std::array<Sample, slots> samples;
    Pattern pattern;
    static double stepBeats(int division) {
        constexpr double values[] = {2, 1, .5, .25, .125, 1.0/3.0, 1.0/6.0, .375};
        return values[std::clamp(division, 0, 7)];
    }
    void generate(uint32_t seed, float chaos, bool mutate, Locks locks) {
        std::mt19937 random(seed);
        auto unit = [&]() { return float(random() % 100000) / 100000.f; };
        std::vector<int> available;
        for (int i=0; i<slots; ++i) if (samples[size_t(i)].loaded()) available.push_back(i);
        if (available.empty()) return;
        pattern.seed = seed;
        for (int i=0; i<steps; ++i) {
            auto& s = pattern.notes[size_t(i)];
            if (s.locked || (mutate && unit() > .15f + chaos * .45f)) continue;
            if (!locks.source) {
                s.slot = available[size_t(random() % available.size())];
                s.slice = int(random() % (samples[size_t(s.slot)].cuts.size()-1));
            }
            if (!locks.rhythm) { s.enabled = i%4 == 0 || unit() > chaos*.35f; s.gate = .45f + unit()*.53f; }
            if (!locks.pitch) { constexpr int pitches[] = {-12,-7,-5,0,0,0,5,7,12}; s.semitones = unit()<chaos ? pitches[random()%9] : 0; }
            if (!locks.effects) {
                s.reverse = unit() < chaos*.35f;
                s.repeats = unit()<chaos*.6f ? (1 << (1 + random()%3)) : 1;
                s.gain = .55f + unit()*.4f;
            }
        }
    }
    // No allocation, file IO, or random generation in sample playback.
    std::array<float,2> atBeat(double beat, double bpm, int division, int transpose=0, bool grain=false) const {
        if (!std::isfinite(beat) || !std::isfinite(bpm) || bpm<=0) return {};
        const double duration = stepBeats(division);
        const double stepPosition = beat/duration;
        const double stepFloor = std::floor(stepPosition);
        const int index = int(std::fmod(std::fmod(stepFloor, steps)+steps, steps));
        const auto& s = pattern.notes[size_t(index)];
        if (!s.enabled || s.slot<0 || s.slot>=slots) return {};
        const auto& sample = samples[size_t(s.slot)];
        if (!sample.loaded()) return {};
        const int cut = std::clamp(s.slice,0,int(sample.cuts.size())-2);
        const int begin=sample.cuts[size_t(cut)], end=sample.cuts[size_t(cut+1)];
        const double seconds=(stepPosition-stepFloor)*duration*60.0/bpm;
        const int repeats=std::clamp(s.repeats,1,8);
        const double segment=duration*60.0/bpm/repeats;
        const double local=std::fmod(seconds, segment);
        const double length=segment*std::clamp(double(s.gate),.05,1.0);
        if (local>=length || end-begin<2) return {};
        const double ratio=std::pow(2.0,std::clamp(s.semitones+transpose,-36,36)/12.0);
        const double grainDuration=.025;
        const double time=grain ? std::fmod(local,grainDuration) : local;
        const double offset=time*sample.rate*ratio;
        if (offset>=end-begin-1) return {};
        const double position=s.reverse ? end-1-offset : begin+offset;
        const int a=std::clamp(int(position),begin,end-1), b=std::min(a+1,end-1);
        const float fraction=float(position-a);
        double envelope=std::min({1.0,local/.003,(length-local)/.005});
        if (grain) envelope *= std::pow(std::sin(pi * time/grainDuration),2.0);
        std::array<float,2> result{};
        for (int ch=0; ch<2; ++ch) {
            const auto& data=ch==0 ? sample.left : sample.right;
            if (data.size()!=sample.left.size()) continue;
            result[size_t(ch)]=(data[size_t(a)]+fraction*(data[size_t(b)]-data[size_t(a)]))*float(envelope)*s.gain;
        }
        return result;
    }
};
}
