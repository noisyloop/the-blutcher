#pragma once
#include "BlucherEngine.h"
#include <complex>
namespace blucher {
// Fixed-storage STFT: 1024-frame latency, 4x overlap, no allocations in process().
class SpectralProcessor {
public:
    static constexpr int size=1024,hop=256;
    struct Settings { float formant=0, blend=0, vocoder=0, clarity=.2f; };
    void reset() {
        write=0; total=0;
        for(auto& a:inputs)a.fill(0);
        for(auto& a:output)a.fill(0);
    }
    SpectralProcessor() { for(int i=0;i<size;++i)window[size_t(i)]=float(.5-.5*std::cos(2*pi*i/size)); reset(); }
    std::array<float,2> process(std::array<float,2> main,std::array<float,2> other,std::array<float,2> voice,std::array<float,2> carrier,const Settings& settings) {
        std::array<float,2> result{};
        for(int ch=0;ch<2;++ch){
            result[size_t(ch)]=output[size_t(ch)][size_t(write)];output[size_t(ch)][size_t(write)]=0;
            inputs[size_t(ch*4)][size_t(write)]=main[size_t(ch)];inputs[size_t(ch*4+1)][size_t(write)]=other[size_t(ch)];
            inputs[size_t(ch*4+2)][size_t(write)]=voice[size_t(ch)];inputs[size_t(ch*4+3)][size_t(write)]=carrier[size_t(ch)];
        }
        write=(write+1)%size;++total;
        if(total%hop==0) for(int ch=0;ch<2;++ch)frame(ch,settings);
        return result;
    }
private:
    using Complex=std::complex<float>;
    std::array<std::array<float,size>,8> inputs{};
    std::array<std::array<float,size>,2> output{};
    std::array<float,size> window{};
    std::array<std::array<Complex,size>,4> spectrum{};
    std::array<std::array<float,size/2+1>,4> magnitude{},envelope{};
    int write=0;uint64_t total=0;
    static void fft(std::array<Complex,size>& values,bool inverse) {
        for(int i=1,j=0;i<size;++i){int bit=size>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(values[size_t(i)],values[size_t(j)]);}
        for(int length=2;length<=size;length<<=1){float angle=float((inverse?2:-2)*pi/length);Complex step(std::cos(angle),std::sin(angle));for(int base=0;base<size;base+=length){Complex w(1,0);for(int k=0;k<length/2;++k){auto a=values[size_t(base+k)],b=w*values[size_t(base+k+length/2)];values[size_t(base+k)]=a+b;values[size_t(base+k+length/2)]=a-b;w*=step;}}}
        if(inverse)for(auto& x:values)x/=float(size);
    }
    void frame(int ch,const Settings& settings) {
        const bool cross=settings.blend>0, vocode=settings.vocoder>0, shift=std::abs(settings.formant)>.001f;
        for(int source=0;source<4;++source){
            if(source==1&&!cross)continue;
            if(source>=2&&!vocode)continue;
            for(int i=0;i<size;++i)spectrum[size_t(source)][size_t(i)]=inputs[size_t(ch*4+source)][size_t((write+i)%size)]*window[size_t(i)];
            fft(spectrum[size_t(source)],false);
            for(int bin=0;bin<=size/2;++bin)magnitude[size_t(source)][size_t(bin)]=std::abs(spectrum[size_t(source)][size_t(bin)]);
            // Smooth across harmonics to estimate the broad spectral envelope.
            for(int bin=0;bin<=size/2;++bin){int width=std::max(3,bin/12);int begin=std::max(0,bin-width),end=std::min(size/2,bin+width);float sum=0;for(int k=begin;k<=end;++k)sum+=magnitude[size_t(source)][size_t(k)];envelope[size_t(source)][size_t(bin)]=sum/float(end-begin+1);}
        }
        const float ratio=std::pow(2.f,std::clamp(settings.formant,-12.f,12.f)/12.f);
        for(int bin=0;bin<=size/2;++bin){
            auto dry=spectrum[0][size_t(bin)];float m=magnitude[0][size_t(bin)];
            if(shift){float sourceBin=float(bin)/ratio;int lo=std::clamp(int(sourceBin),0,size/2),hi=std::min(lo+1,size/2);float fraction=std::clamp(sourceBin-float(lo),0.f,1.f);float shifted=envelope[0][size_t(lo)]+fraction*(envelope[0][size_t(hi)]-envelope[0][size_t(lo)]);m*=std::clamp(shifted/(envelope[0][size_t(bin)]+1.e-6f),0.f,6.f);}
            if(cross)m=std::sqrt(std::max(0.f,(1-settings.blend)*m*m+settings.blend*magnitude[1][size_t(bin)]*magnitude[1][size_t(bin)]));
            Complex transformed=std::abs(dry)>1.e-8f?dry*(m/(std::abs(dry)+1.e-12f)):Complex(m,0);
            if(vocode){
                // Transfer the vocal's spectral envelope onto the carrier's fine structure.
                const float vocal=envelope[2][size_t(std::clamp(int(float(bin)/ratio),0,size/2))];
                const float carrierEnvelope=envelope[3][size_t(bin)];
                const auto carrier=spectrum[3][size_t(bin)];
                const auto wet=carrier*std::clamp(vocal/(carrierEnvelope+1.e-5f),0.f,12.f);
                const auto intelligible=wet*(1-settings.clarity)+spectrum[2][size_t(bin)]*settings.clarity;
                transformed=transformed*(1-settings.vocoder)+intelligible*settings.vocoder;
            }
            spectrum[0][size_t(bin)]=transformed;
            if(bin>0&&bin<size/2)spectrum[0][size_t(size-bin)]=std::conj(transformed);
        }
        fft(spectrum[0],true);
        for(int i=0;i<size;++i)output[size_t(ch)][size_t((write+i)%size)]+=spectrum[0][size_t(i)].real()*window[size_t(i)]*(2.f/3.f);
    }
};
}
