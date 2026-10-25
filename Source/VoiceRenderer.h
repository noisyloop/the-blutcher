#pragma once
#include "SpectralProcessor.h"
namespace blucher {
struct SoundSettings {
    SpectralProcessor::Settings spectral;
    int layers=1, blendSlot=1, vocalSlot=0, carrier=0;
    float layerMix=.5f;
};
class VoiceRenderer {
public:
    static constexpr int latency=SpectralProcessor::size;
    void prepare(double sampleRate) {rate=sampleRate;phase=0;spectral.reset();}
    std::array<float,2> process(const Engine& engine,double beat,double bpm,int division,int transpose,bool grain,const SoundSettings& settings) {
        auto main=engine.atBeat(beat,bpm,division,transpose,grain);
        if(settings.layers>1){
            const double step=std::floor(beat/Engine::stepBeats(division));
            const int index=int(std::fmod(std::fmod(step,16)+16,16));
            const int origin=engine.pattern.notes[size_t(index)].slot;
            std::array<float,2> sum{};int count=0;
            for(int offset=1;offset<slots&&count<settings.layers-1;++offset){int slot=(origin+offset)%slots;if(!engine.samples[size_t(slot)].loaded())continue;auto layer=engine.atBeat(beat,bpm,division,transpose,grain,slot);for(int ch=0;ch<2;++ch)sum[size_t(ch)]+=layer[size_t(ch)];++count;}
            if(count>0)for(int ch=0;ch<2;++ch)main[size_t(ch)]=main[size_t(ch)]*(1-settings.layerMix)+sum[size_t(ch)]/float(count)*settings.layerMix;
        }
        std::array<float,2> other{},voice{},carrier{};
        if(settings.spectral.blend>0)other=engine.atBeat(beat,bpm,division,transpose,grain,settings.blendSlot);
        if(settings.spectral.vocoder>0){
            voice=engine.atBeat(beat,bpm,division,transpose,grain,settings.vocalSlot);
            if(settings.carrier>0)carrier=engine.atBeat(beat,bpm,division,transpose,grain,settings.carrier-1);
            else {
                const double step=std::floor(beat/Engine::stepBeats(division));const int index=int(std::fmod(std::fmod(step,16)+16,16));
                const double frequency=261.6255653006*std::pow(2.0,std::clamp(transpose+engine.pattern.notes[size_t(index)].semitones,-36,36)/12.0);
                float value=0;for(int harmonic=1;harmonic<=24&&frequency*harmonic<rate*.45;++harmonic)value+=float(std::sin(2*pi*phase*harmonic)/harmonic)*.22f;
                carrier={value,value};phase+=frequency/rate;phase-=std::floor(phase);
            }
        }
        return spectral.process(main,other,voice,carrier,settings.spectral);
    }
private:
    SpectralProcessor spectral;
    double rate=44100,phase=0;
};
}
