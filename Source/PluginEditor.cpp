#include "PluginEditor.h"
namespace { const juce::Colour background(0xff10131a), panel(0xff1c2230), accent(0xffa4f26c), text(0xffe4e8f0); }
BlucherEditor::BlucherEditor(BlucherProcessor& p) : AudioProcessorEditor(p),processor(p) {
    setSize(1000,740);
    for(int i=0;i<8;++i){auto& b=sampleButtons[size_t(i)];addAndMakeVisible(b);b.onClick=[this,i]{chooseSample(i);};b.setColour(juce::TextButton::buttonColourId,panel);b.setColour(juce::TextButton::textColourOffId,text);}
    for(int i=0;i<16;++i){auto& b=stepButtons[size_t(i)];addAndMakeVisible(b);b.onClick=[this,i]{
        if(juce::ModifierKeys::getCurrentModifiers().isAltDown()){
            juce::PopupMenu menu;auto step=processor.getPattern().notes[size_t(i)];
            menu.addItem(1,"Reverse",true,step.reverse);menu.addItem(2,"Previous slice");menu.addItem(3,"Next slice");
            menu.addItem(4,"Pitch -1 semitone");menu.addItem(5,"Pitch +1 semitone");menu.addItem(6,"Reset pitch");
            juce::PopupMenu sources,repeats;for(int slot=0;slot<8;++slot)sources.addItem(100+slot,"Slot "+juce::String(slot+1),true,step.slot==slot);
            for(int n=1;n<=8;++n)repeats.addItem(10+n,juce::String(n),true,step.repeats==n);
            menu.addSubMenu("Source",sources);menu.addSubMenu("Retriggers",repeats);
            juce::Component::SafePointer<BlucherEditor> safe(this);menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&stepButtons[size_t(i)]),[safe,i](int action){if(safe){safe->processor.editStep(i,action);safe->timerCallback();}});
        }else processor.toggleStep(i,juce::ModifierKeys::getCurrentModifiers().isShiftDown());timerCallback();};}
    const char* lockNames[]={"Lock rhythm","Lock sources","Lock pitch","Lock glitches"};
    for(int i=0;i<4;++i){auto& b=lockButtons[size_t(i)];addAndMakeVisible(b);b.setButtonText(lockNames[i]);b.onClick=[this,i]{const juce::ScopedLock l(processor.mutex);bool value=lockButtons[size_t(i)].getToggleState();if(i==0)processor.locks.rhythm=value;if(i==1)processor.locks.source=value;if(i==2)processor.locks.pitch=value;if(i==3)processor.locks.effects=value;};}
    const char* ids[]={"chaos","sensitivity","gain","bpm"};const char* titles[]={"CHAOS","SLICE SENSITIVITY","OUTPUT","PREVIEW BPM"};
    for(int i=0;i<4;++i){auto& s=sliders[size_t(i)];addAndMakeVisible(s);s.setSliderStyle(juce::Slider::LinearHorizontal);s.setTextBoxStyle(juce::Slider::TextBoxRight,false,60,22);s.setColour(juce::Slider::thumbColourId,accent);auto& label=labels[size_t(i)];addAndMakeVisible(label);label.setText(titles[i],juce::dontSendNotification);label.setColour(juce::Label::textColourId,text);sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,ids[i],s));}
    for(auto* b:{&generate,&mutate,&undo,&ab,&play,&slice,&save,&drag}){addAndMakeVisible(*b);b->setColour(juce::TextButton::buttonColourId,panel);b->setColour(juce::TextButton::textColourOffId,text);}
    generate.setColour(juce::TextButton::buttonColourId,accent);generate.setColour(juce::TextButton::textColourOffId,background);
    generate.onClick=[this]{processor.generate(false);status.setText("New phrase generated. Undo keeps your previous result.",juce::dontSendNotification);timerCallback();};
    mutate.onClick=[this]{processor.generate(true);timerCallback();};undo.onClick=[this]{processor.undo();timerCallback();};ab.onClick=[this]{processor.swapAB();timerCallback();};
    play.onClick=[this]{processor.preview.store(!processor.preview.load());};
    slice.onClick=[this]{processor.reslice();status.setText("Slices updated using the current sensitivity.",juce::dontSendNotification);timerCallback();};
    drag.setTriggeredOnMouseDown(true);
    save.onClick=[this]{exportAudio(false);};drag.onClick=[this]{exportAudio(true);};
    division.addItemList({"1/2","1/4","1/8","1/16","1/32","1/8 triplet","1/16 triplet","1/16 dotted"},1);addAndMakeVisible(division);
    divisionAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.parameters,"division",division);
    addAndMakeVisible(grains);grainAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters,"grain",grains);
    addAndMakeVisible(status);status.setColour(juce::Label::textColourId,text);status.setText("Load a few samples → Generate → Preview. Host playback follows Ableton's tempo.",juce::dontSendNotification);
    addAndMakeVisible(soundTab);soundTab.onClick=[this]{showSound(!soundVisible);};
    addAndMakeVisible(soundRandom);soundRandom.onClick=[this]{processor.randomizeSound();status.setText("Sound randomized. Locks apply. Undo restores the previous sound. Controls save with your session.",juce::dontSendNotification);};
    const char* soundIds[]={"formant","vocoder","spectral","clarity","layerMix"};
    const char* soundTitles[]={"FORMANT SHIFT / SEMITONES","VOCODER MIX","SPECTRAL BLEND","VOCAL CLARITY / DRY VOICE","LAYER MIX"};
    for(int i=0;i<5;++i){auto& slider=soundSliders[size_t(i)];addAndMakeVisible(slider);slider.setSliderStyle(juce::Slider::LinearHorizontal);slider.setTextBoxStyle(juce::Slider::TextBoxRight,false,60,22);slider.setColour(juce::Slider::thumbColourId,accent);addAndMakeVisible(soundLabels[size_t(i)]);soundLabels[size_t(i)].setText(soundTitles[i],juce::dontSendNotification);sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,soundIds[i],slider));}
    const char* choiceIds[]={"layers","blendSlot","vocalSlot","carrier"};const char* choiceTitles[]={"LAYER COUNT","SPECTRAL SOURCE","VOCAL SOURCE","VOCODER CARRIER"};
    for(int i=0;i<4;++i){auto& box=soundChoices[size_t(i)];addAndMakeVisible(box);if(i==0)box.addItemList({"1 voice","2 voices","3 voices","4 voices"},1);else {int offset=i==3?2:1;if(i==3)box.addItem("Internal synth",1);for(int slot=0;slot<8;++slot)box.addItem("Slot "+juce::String(slot+1),slot+offset);}addAndMakeVisible(choiceLabels[size_t(i)]);choiceLabels[size_t(i)].setText(choiceTitles[i],juce::dontSendNotification);soundChoiceAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.parameters,choiceIds[i],box));}
    showSound(false);timerCallback();startTimerHz(15);
}
void BlucherEditor::paint(juce::Graphics& g){
    g.fillAll(background);g.setColour(accent);g.setFont(38.f);g.drawText("THE BLUCHER",24,20,600,50,juce::Justification::centredLeft);
    g.setColour(text.withAlpha(.6f));g.setFont(14.f);g.drawText("NOISYLOOP  /  MULTI-SAMPLE GLITCH INSTRUMENT",26,72,800,24,juce::Justification::centredLeft);
    g.setColour(panel);g.fillRoundedRectangle(20,110,960,230,12);g.fillRoundedRectangle(20,355,960,155,12);
    g.setColour(text);g.setFont(13.f);g.drawText(soundVisible?"SOUND LAB  ·  VOCODING / FORMANTS / LAYERS / SPECTRAL MORPH":"SAMPLE BANK  ·  UP TO 30 SECONDS EACH  ·  DROP FILES ON A SLOT",36,119,900,24,juce::Justification::centredLeft);
    g.drawText("PHRASE  ·  CLICK TO MUTE  ·  SHIFT-CLICK TO LOCK  ·  ALT-CLICK TO EDIT",36,363,900,24,juce::Justification::centredLeft);
    auto pattern=processor.getPattern();g.setColour(text.withAlpha(.6f));g.drawText("SEED "+juce::String(juce::int64(pattern.seed))+"  /  SAVED WITH YOUR SESSION",26,704,850,22,juce::Justification::centredLeft);
}
void BlucherEditor::resized(){
    for(int i=0;i<8;++i)sampleButtons[size_t(i)].setBounds(36+(i%2)*468,150+(i/2)*45,458,36);
    for(int i=0;i<16;++i)stepButtons[size_t(i)].setBounds(36+i*58,396,52,60);
    for(int i=0;i<4;++i)lockButtons[size_t(i)].setBounds(36+i*225,470,220,26);
    for(int i=0;i<4;++i){int x=24+(i%2)*484,y=526+(i/2)*64;labels[size_t(i)].setBounds(x,y,180,20);sliders[size_t(i)].setBounds(x,y+22,455,28);}
    generate.setBounds(24,650,118,36);mutate.setBounds(150,650,98,36);undo.setBounds(256,650,68,36);ab.setBounds(332,650,96,36);play.setBounds(436,650,96,36);slice.setBounds(540,650,96,36);save.setBounds(644,650,124,36);drag.setBounds(776,650,112,36);
    soundTab.setBounds(650,35,124,30);soundRandom.setBounds(580,240,290,30);
    for(int i=0;i<5;++i){int column=i<3?0:1,row=i<3?i:i-3;int x=36+column*468,y=148+row*48;soundLabels[size_t(i)].setBounds(x,y-17,440,20);soundSliders[size_t(i)].setBounds(x,y,440,26);}
    for(int i=0;i<4;++i){choiceLabels[size_t(i)].setBounds(36+i*234,283,220,20);soundChoices[size_t(i)].setBounds(36+i*234,305,220,26);}
    division.setBounds(780,75,124,26);grains.setBounds(776,43,180,26);status.setBounds(24,688,950,20);
}
void BlucherEditor::showSound(bool show){soundVisible=show;for(auto& b:sampleButtons)b.setVisible(!show);for(auto& s:soundSliders)s.setVisible(show);for(auto& l:soundLabels)l.setVisible(show);for(auto& c:soundChoices)c.setVisible(show);for(auto& l:choiceLabels)l.setVisible(show);soundRandom.setVisible(show);soundTab.setButtonText(show?"SAMPLE BANK":"SOUND LAB");repaint();}
void BlucherEditor::timerCallback(){
    for(int i=0;i<8;++i)sampleButtons[size_t(i)].setButtonText(juce::String(i+1)+"  "+processor.slotName(i));
    auto pattern=processor.getPattern();int current=processor.currentStep.load();
    for(int i=0;i<16;++i){const auto& s=pattern.notes[size_t(i)];auto& b=stepButtons[size_t(i)];b.setButtonText(juce::String(i+1)+(s.locked?" *":"")+"\n"+juce::String(s.slot+1)+":"+juce::String(s.slice+1)+(s.reverse?" R":"")+"\nx"+juce::String(s.repeats));b.setColour(juce::TextButton::buttonColourId,i==current?accent:(s.enabled?juce::Colour(0xff34445a):background));b.setColour(juce::TextButton::textColourOffId,i==current?background:text);}
    {const juce::ScopedLock l(processor.mutex);const auto locks=processor.locks;const bool values[]={locks.rhythm,locks.source,locks.pitch,locks.effects};for(int i=0;i<4;++i)lockButtons[size_t(i)].setToggleState(values[i],juce::dontSendNotification);}
    play.setButtonText(processor.preview.load()?"STOP":"PREVIEW");repaint();
}
void BlucherEditor::load(int slot,const juce::File& file){auto error=processor.loadSample(slot,file);status.setText(error.isEmpty()?"Loaded "+file.getFileName()+". Press Generate to combine your samples.":error,juce::dontSendNotification);timerCallback();}
void BlucherEditor::chooseSample(int slot){
    chooser=std::make_unique<juce::FileChooser>("Choose a sample",juce::File{},"*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
    juce::Component::SafePointer<BlucherEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[safe,slot](const juce::FileChooser& c){if(safe&&c.getResult().existsAsFile())safe->load(slot,c.getResult());});
}
void BlucherEditor::filesDropped(const juce::StringArray& files,int x,int y){if(soundVisible)showSound(false);int slot=0;for(int i=0;i<8;++i)if(sampleButtons[size_t(i)].getBounds().contains(x,y)){slot=i;break;}for(const auto& file:files){if(slot>=8)break;load(slot++,juce::File(file));}}
void BlucherEditor::exportAudio(bool externalDrag){
    if(externalDrag){dragFile=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("The-Blucher-phrase",".wav");if(processor.exportWav(dragFile)){juce::DragAndDropContainer::performExternalDragDropOfFiles({dragFile.getFullPathName()},false,this);status.setText("Drag the WAV onto an Ableton audio track.",juce::dontSendNotification);}else status.setText("Could not render WAV.",juce::dontSendNotification);return;}
    chooser=std::make_unique<juce::FileChooser>("Export phrase",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("The-Blucher-phrase.wav"),"*.wav");juce::Component::SafePointer<BlucherEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[safe](const juce::FileChooser& c){if(safe&&c.getResult()!=juce::File{}){bool ok=safe->processor.exportWav(c.getResult().withFileExtension("wav"));safe->status.setText(ok?"WAV exported.":"Could not write WAV.",juce::dontSendNotification);}});
}
