#pragma once
#include "PluginProcessor.h"
namespace blucherui {
const juce::Colour background(0xff0c0e13), panel(0xff151922), raised(0xff1f2531), line(0xff2b3342),
                   accent(0xffb6f25c), hot(0xffff5c8a), text(0xffe8ecf2), dim(0xff8a93a6);
// One hue per sample slot, shared by the slot cards and the step grid.
inline juce::Colour slotColour(int slot){
    static const juce::uint32 hues[]={0xffb6f25c,0xff5ce1f2,0xff9d7bff,0xffff5c8a,0xffffb547,0xff3ddc97,0xff5c9dff,0xffff7a5c};
    return juce::Colour(hues[size_t(juce::jlimit(0,7,slot))]);
}
inline juce::String utf8(const char* s){ return juce::String(juce::CharPointer_UTF8(s)); }
}
class BlucherLookAndFeel : public juce::LookAndFeel_V4 {
public:
    BlucherLookAndFeel();
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
    juce::Font getTextButtonFont(juce::TextButton&,int) override { return juce::Font(juce::FontOptions(12.5f,juce::Font::bold)); }
    void drawComboBox(juce::Graphics&,int,int,bool,int,int,int,int,juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&,juce::Label&) override;
};
class BlucherStepButton : public juce::TextButton {
public:
    int index=0, slot=0, slice=0, repeats=1;
    bool reverse=false, locked=false, enabled=true, playing=false;
    void paintButton(juce::Graphics&,bool over,bool down) override;
};
class BlucherSlotButton : public juce::TextButton {
public:
    int index=0;
    juce::String name, detail;
    bool loaded=false;
    void paintButton(juce::Graphics&,bool over,bool down) override;
};
class BlucherEditor : public juce::AudioProcessorEditor, public juce::FileDragAndDropTarget, private juce::Timer {
public:
    explicit BlucherEditor(BlucherProcessor&);
    ~BlucherEditor() override { stopTimer(); setLookAndFeel(nullptr); }
    void paint(juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag(const juce::StringArray&) override { return true; }
    void filesDropped(const juce::StringArray&,int,int) override;
private:
    void timerCallback() override;
    void chooseSample(int);
    void load(int,const juce::File&);
    void exportAudio(bool drag);
    void showSound(bool);
    void setStatus(const juce::String&);
    BlucherProcessor& processor;
    BlucherLookAndFeel lookAndFeel;
    std::array<BlucherSlotButton,8> sampleButtons;
    std::array<BlucherStepButton,16> stepButtons;
    std::array<juce::TextButton,4> lockButtons;
    std::array<juce::Slider,4> sliders;
    std::array<juce::Label,4> labels;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
    juce::TextButton generate{"GENERATE"},mutate{"MUTATE"},undo{"UNDO"},ab{"SWAP A/B"},play{"PREVIEW"},slice{"RE-SLICE"},save{"EXPORT WAV"},drag{"DRAG WAV"};
    juce::TextButton bankTab{"SAMPLES"}, soundTab{"SOUND LAB"}, soundRandom{"RANDOMIZE SOUND"};
    bool soundVisible=false;
    std::array<juce::Slider,5> soundSliders;
    std::array<juce::Label,5> soundLabels;
    std::array<juce::ComboBox,4> soundChoices;
    std::array<juce::Label,4> choiceLabels;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> soundChoiceAttachments;
    juce::ComboBox division;
    juce::TextButton grains{"GRAIN"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> divisionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> grainAttachment;
    juce::Label status;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::File dragFile;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BlucherEditor)
};
