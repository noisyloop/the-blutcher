#include "VoiceRenderer.h"
#include <iostream>
#include <cstdlib>
#define CHECK(x) do { if(!(x)){std::cerr<<"Failed at line "<<__LINE__<<": " #x "\n";std::exit(1);} }while(false)
int main(){
    blucher::SpectralProcessor neutral;double error=0;
    for(int i=0;i<10000;++i){float input=float(std::sin(i*.073))*.3f;auto out=neutral.process({input,input},{},{},{},{});if(i>2048){float expected=float(std::sin((i-blucher::SpectralProcessor::size)*.073))*.3f;error=std::max(error,double(std::abs(expected-out[0])));}}
    CHECK(error<.00002);
    for(int mode=0;mode<4;++mode){blucher::SpectralProcessor fx;blucher::SpectralProcessor::Settings settings;if(mode==0)settings.formant=9;if(mode==1)settings.blend=.8f;if(mode==2){settings.vocoder=1;settings.clarity=0;}if(mode==3)settings={-7,.5f,.6f,.3f};double energy=0,difference=0;
        for(int i=0;i<12000;++i){float main=float(std::sin(i*.02)+.3*std::sin(i*.11))*.2f;float other=float(std::sin(i*.3))*.3f;float voice=float(std::sin(i*.05))*.3f;float carrier=float(std::sin(i*.02)+std::sin(i*.05)+std::sin(i*.08))*.15f;auto value=fx.process({main,main},{other,other},{voice,voice},{carrier,carrier},settings);for(float v:value){CHECK(std::isfinite(v));energy+=std::abs(v);}if(i>2048){float dry=float(std::sin((i-1024)*.02)+.3*std::sin((i-1024)*.11))*.2f;difference+=std::abs(value[0]-dry);}}
        CHECK(energy>1);CHECK(difference>1);
    }
    blucher::SpectralProcessor silent;blucher::SpectralProcessor::Settings vocoder;vocoder.vocoder=1;vocoder.clarity=0;for(int i=0;i<4096;++i){auto out=silent.process({.2f,.2f},{},{},{.4f,.4f},vocoder);CHECK(std::abs(out[0])<1.e-8f);}
    blucher::Engine engine;for(int slot=0;slot<4;++slot){auto& s=engine.samples[size_t(slot)];s.left.resize(10000);s.right.resize(10000);for(int i=0;i<10000;++i)s.left[size_t(i)]=s.right[size_t(i)]=float(std::sin(i*(.02+.01*slot)))*.3f;s.slice();}engine.generate(44,.7f,false,{});
    blucher::VoiceRenderer renderer;renderer.prepare(44100);blucher::SoundSettings settings;settings.layers=4;settings.spectral={5,.4f,.5f,.2f};double energy=0;for(int i=0;i<12000;++i){auto out=renderer.process(engine,i*120.0/(60*44100),120,3,0,false,settings);for(float v:out){CHECK(std::isfinite(v));energy+=std::abs(v);}}CHECK(energy>1);
    std::cout<<"Spectral tests passed: latency/reconstruction, formant, blend, vocoder gating, layering.\n";
}
