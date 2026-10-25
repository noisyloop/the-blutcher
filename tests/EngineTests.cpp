#include "BlucherEngine.h"
#include <cstdlib>
#include <iostream>
#define CHECK(x) do { if(!(x)){std::cerr << "Failed at line " << __LINE__ << ": " #x "\n";std::exit(1);} }while(false)
bool equal(const blucher::Step& a,const blucher::Step& b){return a.slot==b.slot&&a.slice==b.slice&&a.semitones==b.semitones&&a.repeats==b.repeats&&a.gain==b.gain&&a.gate==b.gate&&a.enabled==b.enabled&&a.reverse==b.reverse&&a.locked==b.locked;}
int main(){
    blucher::Engine a;
    CHECK(a.atBeat(0,120,3)[0]==0);
    a.generate(1,.7f,false,{});
    for(int slot=0;slot<3;++slot){auto& s=a.samples[size_t(slot)];s.rate=44100;s.left.resize(44100);s.right.resize(44100);for(int i=0;i<44100;++i)s.left[size_t(i)]=s.right[size_t(i)]=float(std::sin(i*.02))*.6f;s.slice();CHECK(s.loaded());CHECK(s.cuts.front()==0);CHECK(s.cuts.back()==44100);for(size_t i=1;i<s.cuts.size();++i)CHECK(s.cuts[i]>s.cuts[i-1]);}
    auto b=a;a.generate(123,.8f,false,{});b.generate(123,.8f,false,{});for(int i=0;i<16;++i)CHECK(equal(a.pattern.notes[size_t(i)],b.pattern.notes[size_t(i)]));
    a.pattern.notes[3].locked=true;auto anchor=a.pattern.notes[3];a.generate(72,1,false,{});CHECK(equal(anchor,a.pattern.notes[3]));
    auto before=a.pattern;a.generate(100,1,false,{true,true,true,true});for(int i=0;i<16;++i)CHECK(equal(before.notes[size_t(i)],a.pattern.notes[size_t(i)]));
    before=a.pattern;a.generate(44,1,true,{true,true,true,true});for(int i=0;i<16;++i)CHECK(equal(before.notes[size_t(i)],a.pattern.notes[size_t(i)]));
    before=a.pattern;a.generate(777,1,false,{true,false,false,false});for(int i=0;i<16;++i){const auto& old=before.notes[size_t(i)];const auto& current=a.pattern.notes[size_t(i)];CHECK(old.enabled==current.enabled);CHECK(old.gate==current.gate);CHECK(old.repeats==current.repeats);}
    double energy=0;for(int div=0;div<8;++div)for(bool grain:{false,true})for(int i=0;i<44100;++i){auto v=a.atBeat(i*120.0/(60*44100),120,div,0,grain);for(float x:v){CHECK(std::isfinite(x));CHECK(std::abs(x)<=1);energy+=std::abs(x);}}CHECK(energy>1);
    auto first=a.atBeat(.017,120,3),loop=a.atBeat(4.017,120,3);CHECK(std::abs(first[0]-loop[0])<.0001);
    CHECK(a.atBeat(0,0,3)[0]==0);CHECK(a.atBeat(NAN,120,3)[0]==0);
    // Synthetic impulses must produce separate transient slices.
    blucher::Sample impulses;impulses.rate=44100;impulses.left.assign(44100,0);impulses.right=impulses.left;for(int onset:{4410,13230,26460})for(int i=onset;i<onset+200;++i)impulses.left[size_t(i)]=.8f;impulses.slice();CHECK(impulses.cuts.size()>=5);
    std::cout<<"Engine tests passed: slicing, seeded generation, locks, mutation, looping, finite playback.\n";
}
