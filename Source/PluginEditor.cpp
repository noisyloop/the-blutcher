#include "PluginEditor.h"
using namespace blucherui;
namespace {
juce::Font font(float size,bool bold=false){ return juce::Font(juce::FontOptions(size,bold?juce::Font::bold:juce::Font::plain)); }
juce::Font caps(float size){ return juce::Font(juce::FontOptions(size,juce::Font::bold).withKerningFactor(.08f)); }
const juce::Rectangle<int> topPanel(20,84,960,228), phrasePanel(20,324,960,172), knobPanel(20,508,470,156), actionPanel(502,508,478,156);
void drawPanel(juce::Graphics& g,juce::Rectangle<int> area,const juce::String& title,const juce::String& hint){
    auto r=area.toFloat();
    g.setColour(panel);g.fillRoundedRectangle(r,12.f);
    g.setColour(line.withAlpha(.7f));g.drawRoundedRectangle(r.reduced(.5f),12.f,1.f);
    g.setColour(text);g.setFont(caps(11.f));g.drawText(title,area.getX()+16,area.getY()+12,300,16,juce::Justification::centredLeft);
    g.setColour(dim);g.setFont(font(11.f));g.drawText(hint,area.getX()+260,area.getY()+12,area.getWidth()-276,16,juce::Justification::centredRight);
}
}
BlucherLookAndFeel::BlucherLookAndFeel(){
    setColour(juce::ResizableWindow::backgroundColourId,background);
    setColour(juce::TextButton::buttonColourId,raised);setColour(juce::TextButton::buttonOnColourId,accent.withAlpha(.16f));
    setColour(juce::TextButton::textColourOffId,text);setColour(juce::TextButton::textColourOnId,accent);
    setColour(juce::Label::textColourId,text);
    setColour(juce::Slider::textBoxTextColourId,text);setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);setColour(juce::Slider::textBoxHighlightColourId,accent.withAlpha(.3f));
    setColour(juce::ComboBox::backgroundColourId,raised);setColour(juce::ComboBox::textColourId,text);setColour(juce::ComboBox::outlineColourId,line);setColour(juce::ComboBox::arrowColourId,dim);
    setColour(juce::PopupMenu::backgroundColourId,panel);setColour(juce::PopupMenu::textColourId,text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId,accent.withAlpha(.18f));setColour(juce::PopupMenu::highlightedTextColourId,accent);
    setColour(juce::TextEditor::backgroundColourId,raised);setColour(juce::TextEditor::textColourId,text);setColour(juce::TextEditor::highlightColourId,accent.withAlpha(.3f));
    setColour(juce::CaretComponent::caretColourId,accent);
}
void BlucherLookAndFeel::drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,float position,float start,float end,juce::Slider& slider){
    const auto bounds=juce::Rectangle<int>(x,y,width,height).toFloat().reduced(4.f);
    const float radius=juce::jmin(bounds.getWidth(),bounds.getHeight())*.5f, cx=bounds.getCentreX(), cy=bounds.getCentreY();
    const float angle=start+position*(end-start);
    // Bipolar parameters such as formant shift draw their arc from zero.
    float origin=start;
    if(slider.getMinimum()<0&&slider.getMaximum()>0)origin=start+float(slider.valueToProportionOfLength(0.0))*(end-start);
    const float arcRadius=radius-3.f;
    juce::Path track;track.addCentredArc(cx,cy,arcRadius,arcRadius,0.f,start,end,true);
    g.setColour(line);g.strokePath(track,juce::PathStrokeType(4.f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    juce::Path value;value.addCentredArc(cx,cy,arcRadius,arcRadius,0.f,juce::jmin(origin,angle),juce::jmax(origin,angle),true);
    g.setColour(slider.isEnabled()?accent:dim);g.strokePath(value,juce::PathStrokeType(4.f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    const float body=radius-10.f;
    g.setGradientFill(juce::ColourGradient(raised.brighter(.12f),cx,cy-body,raised.darker(.3f),cx,cy+body,false));
    g.fillEllipse(cx-body,cy-body,body*2.f,body*2.f);
    g.setColour(line.brighter(.1f));g.drawEllipse(cx-body,cy-body,body*2.f,body*2.f,1.f);
    const juce::Point<float> inner(cx+std::sin(angle)*body*.35f,cy-std::cos(angle)*body*.35f), outer(cx+std::sin(angle)*body*.85f,cy-std::cos(angle)*body*.85f);
    g.setColour(text);g.drawLine({inner,outer},2.5f);
}
void BlucherLookAndFeel::drawButtonBackground(juce::Graphics& g,juce::Button& button,const juce::Colour& colour,bool over,bool down){
    const auto r=button.getLocalBounds().toFloat().reduced(.5f);
    const bool left=button.isConnectedOnLeft(), right=button.isConnectedOnRight();
    juce::Path shape;shape.addRoundedRectangle(r.getX(),r.getY(),r.getWidth(),r.getHeight(),6.f,6.f,!left,!right,!left,!right);
    auto fill=colour;
    if(down)fill=fill.brighter(.16f);else if(over)fill=fill.brighter(.08f);
    g.setColour(fill);g.fillPath(shape);
    const bool solid=colour.getAlpha()==255&&colour.getPerceivedBrightness()>.5f;
    g.setColour(button.getToggleState()?accent.withAlpha(.75f):(solid?colour:line.brighter(over?.2f:0.f)));
    g.strokePath(shape,juce::PathStrokeType(1.f));
}
void BlucherLookAndFeel::drawComboBox(juce::Graphics& g,int width,int height,bool,int,int,int,int,juce::ComboBox& box){
    const auto r=juce::Rectangle<int>(width,height).toFloat().reduced(.5f);
    g.setColour(box.findColour(juce::ComboBox::backgroundColourId).brighter(box.isMouseOver(true)?.06f:0.f));g.fillRoundedRectangle(r,6.f);
    g.setColour(box.hasKeyboardFocus(true)||box.isPopupActive()?accent.withAlpha(.75f):line);g.drawRoundedRectangle(r,6.f,1.f);
    const float ax=float(width)-18.f, ay=float(height)*.5f;
    juce::Path chevron;chevron.startNewSubPath(ax-4.f,ay-2.f);chevron.lineTo(ax,ay+2.f);chevron.lineTo(ax+4.f,ay-2.f);
    g.setColour(box.findColour(juce::ComboBox::arrowColourId));g.strokePath(chevron,juce::PathStrokeType(1.6f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
}
void BlucherLookAndFeel::positionComboBoxText(juce::ComboBox& box,juce::Label& label){
    label.setBounds(4,1,box.getWidth()-28,box.getHeight()-2);label.setFont(font(13.f));
}
void BlucherStepButton::paintButton(juce::Graphics& g,bool over,bool down){
    const auto r=getLocalBounds().toFloat().reduced(1.f);
    const auto hue=slotColour(slot);
    auto fill=playing?accent:(enabled?raised:background);
    if(!playing&&(over||down))fill=fill.brighter(down?.14f:.07f);
    g.setColour(fill);g.fillRoundedRectangle(r,6.f);
    if(!enabled&&!playing){
        // Muted steps are hatched so they read as "off" rather than just darker.
        juce::Graphics::ScopedSaveState state(g);
        juce::Path clip;clip.addRoundedRectangle(r,6.f);g.reduceClipRegion(clip);
        g.setColour(line.withAlpha(.55f));
        for(float d=-r.getHeight();d<r.getWidth();d+=7.f)g.drawLine(r.getX()+d,r.getBottom(),r.getX()+d+r.getHeight(),r.getY(),1.f);
    }
    g.setColour(playing?accent:(enabled?line.brighter(.1f):line));g.drawRoundedRectangle(r,6.f,1.f);
    const auto ink=playing?background:(enabled?blucherui::text:dim);
    g.setColour(playing?background.withAlpha(.55f):hue.withAlpha(enabled?1.f:.35f));
    g.fillRoundedRectangle(r.getX()+6.f,r.getY()+5.f,r.getWidth()-12.f,3.f,1.5f);
    g.setColour(ink.withAlpha(.65f));g.setFont(font(10.f,true));
    g.drawText(juce::String(index+1),juce::Rectangle<float>(r.getX()+6.f,r.getY()+11.f,24.f,12.f),juce::Justification::centredLeft);
    if(locked){
        // Small padlock in the top-right corner.
        const float lx=r.getRight()-14.f, ly=r.getY()+12.f;
        g.setColour(playing?background:hot);
        juce::Path shackle;shackle.addCentredArc(lx+4.f,ly+4.f,2.6f,2.8f,0.f,-juce::MathConstants<float>::halfPi,juce::MathConstants<float>::halfPi,true);
        g.strokePath(shackle,juce::PathStrokeType(1.4f));g.fillRoundedRectangle(lx,ly+4.f,8.f,6.f,1.2f);
    }
    g.setColour(ink);g.setFont(font(15.f,true));
    g.drawText(juce::String(slot+1)+":"+juce::String(slice+1),juce::Rectangle<float>(r.getX(),r.getY()+26.f,r.getWidth(),22.f),juce::Justification::centred);
    juce::StringArray chips;
    if(reverse)chips.add("REV");
    chips.add(utf8("\xc3\x97")+juce::String(repeats));
    const auto chipFont=font(9.5f,true);
    float total=0.f;for(auto& c:chips)total+=juce::GlyphArrangement::getStringWidth(chipFont,c)+8.f;
    total+=float(chips.size()-1)*3.f;
    float cx=r.getCentreX()-total*.5f;const float cy=r.getBottom()-21.f;
    g.setFont(chipFont);
    for(auto& c:chips){
        const float w=juce::GlyphArrangement::getStringWidth(chipFont,c)+8.f;
        const bool emphasis=c=="REV"||repeats>1;
        const auto tint=playing?background:(c=="REV"?hot:accent);
        g.setColour(emphasis?tint.withAlpha(playing?.2f:.16f):juce::Colours::transparentBlack);g.fillRoundedRectangle(cx,cy,w,14.f,4.f);
        g.setColour(emphasis?(playing?background:tint):ink.withAlpha(.45f));g.drawText(c,juce::Rectangle<float>(cx,cy,w,14.f),juce::Justification::centred);
        cx+=w+3.f;
    }
}
void BlucherSlotButton::paintButton(juce::Graphics& g,bool over,bool down){
    const auto r=getLocalBounds().toFloat().reduced(.5f);
    const auto hue=slotColour(index);
    auto fill=loaded?raised:panel;
    if(over||down)fill=fill.brighter(down?.1f:.05f);
    g.setColour(fill);g.fillRoundedRectangle(r,8.f);
    if(loaded){g.setColour(over?hue.withAlpha(.7f):line.brighter(.1f));g.drawRoundedRectangle(r,8.f,1.f);}
    else{
        juce::Path outline,dashed;outline.addRoundedRectangle(r,8.f);
        const float dashes[]={5.f,4.f};juce::PathStrokeType(1.f).createDashedStroke(dashed,outline,dashes,2);
        g.setColour(over?hue.withAlpha(.7f):line.brighter(.25f));g.fillPath(dashed);
    }
    const juce::Rectangle<float> badge(r.getX()+12.f,r.getCentreY()-14.f,28.f,28.f);
    g.setColour(loaded?hue:line);g.fillRoundedRectangle(badge,6.f);
    g.setColour(loaded?background:dim);g.setFont(font(13.f,true));g.drawText(juce::String(index+1),badge,juce::Justification::centred);
    const float tx=badge.getRight()+12.f, tw=r.getRight()-tx-10.f;
    g.setColour(loaded?blucherui::text:dim);g.setFont(font(13.5f,true));
    g.drawFittedText(loaded?name:"Empty slot",juce::Rectangle<float>(tx,r.getCentreY()-18.f,tw,18.f).toNearestInt(),juce::Justification::centredLeft,1,.8f);
    g.setColour(dim);g.setFont(font(11.5f));
    g.drawFittedText(loaded?detail:"Drop audio or click to load",juce::Rectangle<float>(tx,r.getCentreY()+1.f,tw,16.f).toNearestInt(),juce::Justification::centredLeft,1,.8f);
}
BlucherEditor::BlucherEditor(BlucherProcessor& p) : AudioProcessorEditor(p),processor(p) {
    setLookAndFeel(&lookAndFeel);
    setSize(1000,720);
    for(int i=0;i<8;++i){auto& b=sampleButtons[size_t(i)];b.index=i;addAndMakeVisible(b);b.onClick=[this,i]{chooseSample(i);};}
    for(int i=0;i<16;++i){auto& b=stepButtons[size_t(i)];b.index=i;addAndMakeVisible(b);b.onClick=[this,i]{
        if(juce::ModifierKeys::getCurrentModifiers().isAltDown()){
            juce::PopupMenu menu;auto step=processor.getPattern().notes[size_t(i)];
            menu.addItem(1,"Reverse",true,step.reverse);menu.addItem(2,"Previous slice");menu.addItem(3,"Next slice");
            menu.addItem(4,"Pitch -1 semitone");menu.addItem(5,"Pitch +1 semitone");menu.addItem(6,"Reset pitch");
            juce::PopupMenu sources,repeats;for(int slot=0;slot<8;++slot)sources.addItem(100+slot,"Slot "+juce::String(slot+1),true,step.slot==slot);
            for(int n=1;n<=8;++n)repeats.addItem(10+n,juce::String(n),true,step.repeats==n);
            menu.addSubMenu("Source",sources);menu.addSubMenu("Retriggers",repeats);
            juce::Component::SafePointer<BlucherEditor> safe(this);menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&stepButtons[size_t(i)]),[safe,i](int action){if(safe){safe->processor.editStep(i,action);safe->timerCallback();}});
        }else processor.toggleStep(i,juce::ModifierKeys::getCurrentModifiers().isShiftDown());timerCallback();};}
    const char* lockNames[]={"RHYTHM","SOURCES","PITCH","GLITCHES"};
    for(int i=0;i<4;++i){auto& b=lockButtons[size_t(i)];addAndMakeVisible(b);b.setButtonText(lockNames[i]);b.setClickingTogglesState(true);b.onClick=[this,i]{const juce::ScopedLock l(processor.mutex);bool value=lockButtons[size_t(i)].getToggleState();if(i==0)processor.locks.rhythm=value;if(i==1)processor.locks.source=value;if(i==2)processor.locks.pitch=value;if(i==3)processor.locks.effects=value;};}
    auto knob=[this](juce::Slider& s,juce::Label& label,const juce::String& title){
        addAndMakeVisible(s);s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,72,18);
        addAndMakeVisible(label);label.setText(title,juce::dontSendNotification);label.setJustificationType(juce::Justification::centred);label.setFont(caps(10.f));label.setColour(juce::Label::textColourId,dim);
    };
    const char* ids[]={"chaos","sensitivity","gain","bpm"};const char* titles[]={"CHAOS","SLICE SENS","OUTPUT","PREVIEW BPM"};
    for(int i=0;i<4;++i){knob(sliders[size_t(i)],labels[size_t(i)],titles[i]);sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,ids[i],sliders[size_t(i)]));}
    for(auto* b:{&generate,&mutate,&undo,&ab,&play,&slice,&save,&drag})addAndMakeVisible(*b);
    generate.setColour(juce::TextButton::buttonColourId,accent);generate.setColour(juce::TextButton::textColourOffId,background);
    generate.onClick=[this]{processor.generate(false);setStatus("New phrase generated. Undo keeps your previous result.");timerCallback();};
    mutate.onClick=[this]{processor.generate(true);timerCallback();};undo.onClick=[this]{processor.undo();timerCallback();};ab.onClick=[this]{processor.swapAB();timerCallback();};
    play.onClick=[this]{processor.preview.store(!processor.preview.load());timerCallback();};
    slice.onClick=[this]{processor.reslice();setStatus("Slices updated using the current sensitivity.");timerCallback();};
    drag.setTriggeredOnMouseDown(true);
    save.onClick=[this]{exportAudio(false);};drag.onClick=[this]{exportAudio(true);};
    division.addItemList({"1/2","1/4","1/8","1/16","1/32","1/8 triplet","1/16 triplet","1/16 dotted"},1);addAndMakeVisible(division);
    divisionAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.parameters,"division",division);
    addAndMakeVisible(grains);grains.setClickingTogglesState(true);grainAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters,"grain",grains);
    addAndMakeVisible(status);status.setFont(font(12.f));status.setColour(juce::Label::textColourId,text.withAlpha(.8f));
    setStatus(utf8("Load a few samples \xe2\x86\x92 Generate \xe2\x86\x92 Preview. Host playback follows Ableton's tempo."));
    for(auto* tab:{&bankTab,&soundTab}){addAndMakeVisible(*tab);tab->setRadioGroupId(1);tab->setClickingTogglesState(true);}
    bankTab.setConnectedEdges(juce::Button::ConnectedOnRight);soundTab.setConnectedEdges(juce::Button::ConnectedOnLeft);
    bankTab.onClick=[this]{showSound(false);};soundTab.onClick=[this]{showSound(true);};
    addAndMakeVisible(soundRandom);soundRandom.onClick=[this]{processor.randomizeSound();setStatus("Sound randomized. Locks apply. Undo restores the previous sound. Controls save with your session.");};
    const char* soundIds[]={"formant","vocoder","spectral","clarity","layerMix"};
    const char* soundTitles[]={"FORMANT","VOCODER","SPECTRAL","CLARITY","LAYER MIX"};
    for(int i=0;i<5;++i){knob(soundSliders[size_t(i)],soundLabels[size_t(i)],soundTitles[i]);sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,soundIds[i],soundSliders[size_t(i)]));}
    const char* choiceIds[]={"layers","blendSlot","vocalSlot","carrier"};const char* choiceTitles[]={"LAYER COUNT","SPECTRAL SOURCE","VOCAL SOURCE","VOCODER CARRIER"};
    for(int i=0;i<4;++i){auto& box=soundChoices[size_t(i)];addAndMakeVisible(box);if(i==0)box.addItemList({"1 voice","2 voices","3 voices","4 voices"},1);else {int offset=i==3?2:1;if(i==3)box.addItem("Internal synth",1);for(int slot=0;slot<8;++slot)box.addItem("Slot "+juce::String(slot+1),slot+offset);}
        auto& label=choiceLabels[size_t(i)];addAndMakeVisible(label);label.setText(choiceTitles[i],juce::dontSendNotification);label.setFont(caps(10.f));label.setColour(juce::Label::textColourId,dim);
        soundChoiceAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.parameters,choiceIds[i],box));}
    showSound(false);timerCallback();startTimerHz(15);
}
void BlucherEditor::paint(juce::Graphics& g){
    g.fillAll(background);
    g.setColour(accent);g.setFont(juce::Font(juce::FontOptions(30.f,juce::Font::bold).withKerningFactor(.02f)));g.drawText("THE BLUCHER",24,16,400,36,juce::Justification::centredLeft);
    g.setColour(dim);g.setFont(caps(10.f));g.drawText(utf8("NOISYLOOP  \xc2\xb7  MULTI-SAMPLE GLITCH INSTRUMENT"),26,52,400,14,juce::Justification::centredLeft);
    g.setFont(caps(9.f));
    g.drawText("VIEW",bankTab.getX(),12,100,12,juce::Justification::centredLeft);
    g.drawText("STEP LENGTH",division.getX(),12,120,12,juce::Justification::centredLeft);
    g.drawText("PLAYBACK",grains.getX(),12,100,12,juce::Justification::centredLeft);
    const auto dot=utf8("  \xc2\xb7  ");
    if(soundVisible){
        drawPanel(g,topPanel,"SOUND LAB","VOCODER"+dot+"FORMANTS"+dot+"LAYERS"+dot+"SPECTRAL MORPH");
        g.setColour(dim);g.setFont(font(11.5f));
        g.drawFittedText("Formant shifts the spectral envelope without moving pitch. Vocoder imposes the vocal slot on the carrier; "
                         "Clarity mixes the dry voice back in. Layers add the next loaded slots under each step.",36,250,540,44,juce::Justification::topLeft,3,1.f);
    }else drawPanel(g,topPanel,"SAMPLE BANK","UP TO 30 S PER SLOT"+dot+"DROP FILES ON A SLOT"+dot+"MULTIPLE FILES FILL SUCCESSIVE SLOTS");
    drawPanel(g,phrasePanel,"PHRASE","CLICK MUTE"+dot+"SHIFT-CLICK LOCK"+dot+"ALT-CLICK EDIT");
    drawPanel(g,knobPanel,"PERFORMANCE","");
    drawPanel(g,actionPanel,"PHRASE ACTIONS","");
    g.setColour(dim);g.setFont(caps(9.5f));g.drawText("LOCKS",36,lockButtons[0].getY(),60,lockButtons[0].getHeight(),juce::Justification::centredLeft);
    g.setFont(font(11.f));
    g.drawText(utf8("slot:slice   REV reverse   \xc3\x97n retriggers"),640,lockButtons[0].getY(),324,lockButtons[0].getHeight(),juce::Justification::centredRight);
    g.setColour(line);g.fillRect(20,674,960,1);
    auto pattern=processor.getPattern();g.setColour(dim);g.setFont(caps(9.5f));
    g.drawText("SEED "+juce::String(juce::int64(pattern.seed))+dot+"SAVED WITH YOUR SESSION",620,684,360,22,juce::Justification::centredRight);
}
void BlucherEditor::resized(){
    bankTab.setBounds(560,26,100,30);soundTab.setBounds(660,26,100,30);
    division.setBounds(780,26,112,30);grains.setBounds(904,26,76,30);
    for(int i=0;i<8;++i)sampleButtons[size_t(i)].setBounds(36+(i%4)*235,122+(i/4)*92,223,80);
    // Four beat groups of four steps, with a wider gap between groups.
    const float cellGap=6.f, groupGap=16.f, cellWidth=(928.f-12.f*cellGap-3.f*groupGap)/16.f;
    for(int i=0;i<16;++i){const float x=36.f+float(i)*(cellWidth+cellGap)+float(i/4)*(groupGap-cellGap);stepButtons[size_t(i)].setBounds(juce::Rectangle<float>(x,362.f,cellWidth,84.f).toNearestInt());}
    for(int i=0;i<4;++i)lockButtons[size_t(i)].setBounds(90+i*108,458,100,26);
    for(int i=0;i<4;++i){const int x=32+i*114;labels[size_t(i)].setBounds(x,540,106,14);sliders[size_t(i)].setBounds(x,556,106,100);}
    const int columnWidth=143, rowHeight=32;
    auto cell=[&](int column,int row,int span=1){return juce::Rectangle<int>(518+column*(columnWidth+8),542+row*(rowHeight+8),columnWidth*span+8*(span-1),rowHeight);};
    generate.setBounds(cell(0,0));mutate.setBounds(cell(1,0));play.setBounds(cell(2,0));
    undo.setBounds(cell(0,1));ab.setBounds(cell(1,1));slice.setBounds(cell(2,1));
    const int half=(446-8)/2;save.setBounds(cell(0,2).withWidth(half));drag.setBounds(cell(0,2).withX(518+half+8).withWidth(half));
    for(int i=0;i<5;++i){const int x=32+i*110;soundLabels[size_t(i)].setBounds(x,122,104,14);soundSliders[size_t(i)].setBounds(x,138,104,100);}
    for(int i=0;i<4;++i){const int x=604+(i%2)*184,y=122+(i/2)*62;choiceLabels[size_t(i)].setBounds(x,y,176,14);soundChoices[size_t(i)].setBounds(x+2,y+18,172,30);}
    soundRandom.setBounds(606,258,356,36);
    status.setBounds(20,684,600,22);
}
void BlucherEditor::showSound(bool show){
    soundVisible=show;for(auto& b:sampleButtons)b.setVisible(!show);for(auto& s:soundSliders)s.setVisible(show);for(auto& l:soundLabels)l.setVisible(show);for(auto& c:soundChoices)c.setVisible(show);for(auto& l:choiceLabels)l.setVisible(show);soundRandom.setVisible(show);
    bankTab.setToggleState(!show,juce::dontSendNotification);soundTab.setToggleState(show,juce::dontSendNotification);repaint();
}
void BlucherEditor::setStatus(const juce::String& message){status.setText(message,juce::dontSendNotification);}
void BlucherEditor::timerCallback(){
    for(int i=0;i<8;++i){
        auto& b=sampleButtons[size_t(i)];const auto label=processor.slotName(i);
        if(label==b.getButtonText())continue;
        b.setButtonText(label);b.loaded=label.contains("  /  ");
        b.name=b.loaded?label.upToLastOccurrenceOf("  /  ",false,false):juce::String();b.detail=b.loaded?label.fromLastOccurrenceOf("  /  ",false,false):juce::String();b.repaint();
    }
    auto pattern=processor.getPattern();int current=processor.currentStep.load();
    for(int i=0;i<16;++i){
        const auto& s=pattern.notes[size_t(i)];auto& b=stepButtons[size_t(i)];
        const bool changed=b.slot!=s.slot||b.slice!=s.slice||b.repeats!=s.repeats||b.reverse!=s.reverse||b.locked!=s.locked||b.enabled!=s.enabled||b.playing!=(i==current);
        b.slot=s.slot;b.slice=s.slice;b.repeats=s.repeats;b.reverse=s.reverse;b.locked=s.locked;b.enabled=s.enabled;b.playing=i==current;
        if(changed){b.setButtonText("Step "+juce::String(i+1)+", slot "+juce::String(s.slot+1)+" slice "+juce::String(s.slice+1)+(s.enabled?"":", muted")+(s.locked?", locked":""));b.repaint();}
    }
    {const juce::ScopedLock l(processor.mutex);const auto locks=processor.locks;const bool values[]={locks.rhythm,locks.source,locks.pitch,locks.effects};for(int i=0;i<4;++i)lockButtons[size_t(i)].setToggleState(values[i],juce::dontSendNotification);}
    const bool previewing=processor.preview.load();
    play.setButtonText(previewing?"STOP":"PREVIEW");play.setColour(juce::TextButton::buttonColourId,previewing?hot.withAlpha(.22f):raised);play.setColour(juce::TextButton::textColourOffId,previewing?hot:text);
    repaint(620,684,360,22);
}
void BlucherEditor::load(int slot,const juce::File& file){auto error=processor.loadSample(slot,file);setStatus(error.isEmpty()?"Loaded "+file.getFileName()+". Press Generate to combine your samples.":error);timerCallback();}
void BlucherEditor::chooseSample(int slot){
    chooser=std::make_unique<juce::FileChooser>("Choose a sample",juce::File{},"*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
    juce::Component::SafePointer<BlucherEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[safe,slot](const juce::FileChooser& c){if(safe&&c.getResult().existsAsFile())safe->load(slot,c.getResult());});
}
void BlucherEditor::filesDropped(const juce::StringArray& files,int x,int y){if(soundVisible)showSound(false);int slot=0;for(int i=0;i<8;++i)if(sampleButtons[size_t(i)].getBounds().contains(x,y)){slot=i;break;}for(const auto& file:files){if(slot>=8)break;load(slot++,juce::File(file));}}
void BlucherEditor::exportAudio(bool externalDrag){
    if(externalDrag){dragFile=juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("The-Blucher-phrase",".wav");if(processor.exportWav(dragFile)){juce::DragAndDropContainer::performExternalDragDropOfFiles({dragFile.getFullPathName()},false,this);setStatus("Drag the WAV onto an Ableton audio track.");}else setStatus("Could not render WAV.");return;}
    chooser=std::make_unique<juce::FileChooser>("Export phrase",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("The-Blucher-phrase.wav"),"*.wav");juce::Component::SafePointer<BlucherEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[safe](const juce::FileChooser& c){if(safe&&c.getResult()!=juce::File{}){bool ok=safe->processor.exportWav(c.getResult().withFileExtension("wav"));safe->setStatus(ok?"WAV exported.":"Could not write WAV.");}});
}
