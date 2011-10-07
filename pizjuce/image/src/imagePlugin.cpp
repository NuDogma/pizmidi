#include "imagePlugin.h"
#include "imagePluginEditor.h"

//==============================================================================
/*
    This function must be implemented to create the actual plugin object that
    you want to use.
*/
PizAudioProcessor* JUCE_CALLTYPE createPluginFilter() 
{
    return new imagePluginFilter();
}

ImageBank::ImageBank () : ValueTree("ImageBank")
{
	//default values
	for (int b=0;b<128;b++) {
		ValueTree bank("BankValues");
		bank.setProperty("bankIndex",b,0);
		for (int p=0;p<128;p++) 
		{
			ValueTree pv("ProgValues");
			pv.setProperty("progIndex",p,0);
			pv.setProperty("name","Bank " + String(b) + " Image " + String(p+1),0);
			pv.setProperty("icon",String("Images") + File::separator + "Bank " + String(b) + File::separator + String(p+1) + ".svg",0);
			pv.setProperty("text",String::empty,0);
			pv.setProperty("bgcolor",Colour(0xFFFFFFFF).toString(),0);
			pv.setProperty("textcolor",Colour(0xFF000000).toString(),0);
			pv.setProperty("h",400,0);
			pv.setProperty("w",400,0);

			bank.addChild(pv,p,0);
		}
		this->addChild(bank,b,0);
	}
	ValueTree settings("GlobalSettings");
	settings.setProperty("lastProgram",0,0);
	settings.setProperty("lastBank",0,0);
	settings.setProperty("channel",0,0);
	settings.setProperty("noteInput",false,0);
	settings.setProperty("usePC",true,0);
	this->addChild(settings,128,0);
}

//==============================================================================
imagePluginFilter::imagePluginFilter() 
{
	iconPath = getCurrentPath() + File::separator + "images";

    // create built-in programs
    programs = new ImageBank();//new JuceProgram[16384];
    if (!loadDefaultFxb())
    {
		//for(int b=0;b<128;b++)
		//{
		//	for(int i=0;i<128;i++){ 
		//		programs[b*128+i].icon = String("Images") + File::separator + "Bank " + String(b) + File::separator + String(i+1) + ".svg";
		//		programs[b*128+i].name = "Bank " + String(b) + " Image " + String(i+1);
		//	}
		//}
    }

    //start up with the first program
    init = true;
	curBank = 0;
	curProgram = 0;
	setCurrentBank (0,0);
}

imagePluginFilter::~imagePluginFilter() 
{
    if (programs) delete programs;
}

//==============================================================================
float imagePluginFilter::getParameter (int index) 
{
    return param[index];
}

void imagePluginFilter::setParameter (int index, float newValue) 
{
	switch(index)
	{
	case kChannel:
		if (param[index] != newValue) {
			param[index] = newValue;
			programs->getChildWithName("GlobalSettings").setProperty("channel",roundFloatToInt(newValue*16.f),0);
			sendChangeMessage ();
		}
		break;
	default:
		break;
	}
}

const String imagePluginFilter::getParameterName (int index) 
{
    if (index == kChannel) return "Channel";
    return String::empty;
}

const String imagePluginFilter::getParameterText (int index) 
{
    if (index==kChannel) {
        if (roundFloatToInt(param[kChannel]*16.0f)==0) return String("Any");
        else return String(roundFloatToInt(param[kChannel]*16.0f));
    }
    return String::empty;
}

const String imagePluginFilter::getInputChannelName (const int channelIndex) const
{
    return String (channelIndex + 1);
}

const String imagePluginFilter::getOutputChannelName (const int channelIndex) const
{
    return String (channelIndex + 1);
}

bool imagePluginFilter::isInputChannelStereoPair (int index) const
{
    return true;
}

bool imagePluginFilter::isOutputChannelStereoPair (int index) const
{
    return true;
}


//======================Programs================================================
void imagePluginFilter::setCurrentProgram (int index)
{
    //save non-parameter info to the old program, except the first time
    if (!init) {
		programs->set(curBank,curProgram,"h",lastUIHeight);
        programs->set(curBank,curProgram,"w",lastUIWidth);
        programs->set(curBank,curProgram,"icon",icon);
        programs->set(curBank,curProgram,"text",text);
		programs->set(curBank,curProgram,"bgcolor",bgcolor.toString());
		programs->set(curBank,curProgram,"textcolor",textcolor.toString());
    }
    init = false;

    //then set the new program
    curProgram = index;
	param[kChannel] = (float)programs->getChildWithName("GlobalSettings").getProperty("channel")*0.0625f;
    lastUIHeight = programs->get(curBank,curProgram,"h");
    lastUIWidth = programs->get(curBank,curProgram,"w");
    icon = programs->get(curBank,curProgram,"icon");
    text = programs->get(curBank,curProgram,"text");
	bgcolor = Colour::fromString(programs->get(curBank,curProgram,"bgcolor"));
    textcolor = Colour::fromString(programs->get(curBank,curProgram,"textcolor"));
	noteInput = programs->getChildWithName("GlobalSettings").getProperty("noteInput");
	usePC = programs->getChildWithName("GlobalSettings").getProperty("usePC");

    sendChangeMessage();    
}

void imagePluginFilter::setCurrentBank(int index, int program)
{
    if (!init) {
		programs->set(curBank,curProgram,"h",lastUIHeight);
        programs->set(curBank,curProgram,"w",lastUIWidth);
        programs->set(curBank,curProgram,"icon",icon);
        programs->set(curBank,curProgram,"text",text);
		programs->set(curBank,curProgram,"bgcolor",bgcolor.toString());
		programs->set(curBank,curProgram,"textcolor",textcolor.toString());
    }
    init = true;

	curBank = index;
	if (program==-1) 
		program = curProgram;
	updateHostDisplay();
	setCurrentProgram(program);
}

void imagePluginFilter::changeProgramName(int index, const String &newName) {
	programs->set(curBank,index,"name",newName);
}

const String imagePluginFilter::getProgramName(int index) {
    return programs->get(curBank,index,"name");
}

int imagePluginFilter::getCurrentProgram() {
    return curProgram;
}

//==============================================================================
void imagePluginFilter::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // do your pre-playback setup stuff here..
}

void imagePluginFilter::releaseResources()
{
    // when playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

void imagePluginFilter::processBlock (AudioSampleBuffer& buffer,
                                   MidiBuffer& midiMessages)
{
    for (int i = getNumInputChannels(); i < getNumOutputChannels(); ++i)
    {
        buffer.clear (i, 0, buffer.getNumSamples());
    }
    const int channel = roundFloatToInt(param[kChannel]*16.0f);
    MidiBuffer::Iterator mid_buffer_iter(midiMessages);
    MidiMessage midi_message(0xFE);
    int sample_number;
    while(mid_buffer_iter.getNextEvent(midi_message,sample_number)) {
		if (channel==0 || midi_message.isForChannel(channel))
		{
			if (midi_message.isController() && midi_message.getControllerNumber()==0 && usePC) {
				setCurrentBank(midi_message.getControllerValue());
				updateHostDisplay();
			}
			else if (midi_message.isProgramChange() && usePC) {
				setCurrentProgram(midi_message.getProgramChangeNumber());
				updateHostDisplay();
			}
			else if (midi_message.isNoteOn() && noteInput) {
				setCurrentProgram(midi_message.getNoteNumber());
				updateHostDisplay();
			}
        }
    }
	midiMessages.clear();
}
//==============================================================================
AudioProcessorEditor* imagePluginFilter::createEditor()
{
    return new imagePluginEditor (this);
}

//==============================================================================
void imagePluginFilter::getCurrentProgramStateInformation (MemoryBlock& destData)
{
    // make sure the non-parameter settings are copied to the current program
	programs->set(curBank,curProgram,"h",lastUIHeight);
    programs->set(curBank,curProgram,"w",lastUIWidth);
    programs->set(curBank,curProgram,"icon",icon);
    programs->set(curBank,curProgram,"text",text);
	programs->set(curBank,curProgram,"bgcolor",bgcolor.toString());
	programs->set(curBank,curProgram,"textcolor",textcolor.toString());
    // you can store your parameters as binary data if you want to or if you've got
    // a load of binary to put in there, but if you're not doing anything too heavy,
    // XML is a much cleaner way of doing it - here's an example of how to store your
    // params as XML..

    // create an outer XML element..
    XmlElement xmlState ("MYPLUGINSETTINGS");

    // add some attributes to it..
    xmlState.setAttribute ("pluginVersion", 1);

    xmlState.setAttribute ("bank", getCurrentBank());
    xmlState.setAttribute ("program", getCurrentProgram());
    xmlState.setAttribute ("progname", getProgramName(getCurrentProgram()));

    for (int i=0;i<kNumParams;i++) {
        xmlState.setAttribute (String(i), param[i]);
    }

    xmlState.setAttribute ("uiWidth", lastUIWidth);
    xmlState.setAttribute ("uiHeight", lastUIHeight);
    xmlState.setAttribute ("icon", icon);
    xmlState.setAttribute ("text", text);
    xmlState.setAttribute ("bgcolor", (int)(bgcolor.getARGB() & 0x00FFFFFF));
    xmlState.setAttribute ("textcolor", (int)(textcolor.getARGB() & 0x00FFFFFF));
    xmlState.setAttribute ("noteInput", noteInput);
    xmlState.setAttribute ("usePC", usePC);

    // then use this helper function to stuff it into the binary blob and return it..
    copyXmlToBinary (xmlState, destData);
}
void imagePluginFilter::getStateInformation(MemoryBlock &destData) {
    // make sure the non-parameter settings are copied to the current program
	programs->set(curBank,curProgram,"h",lastUIHeight);
    programs->set(curBank,curProgram,"w",lastUIWidth);
    programs->set(curBank,curProgram,"icon",icon);
    programs->set(curBank,curProgram,"text",text);
	programs->set(curBank,curProgram,"bgcolor",bgcolor.toString());
	programs->set(curBank,curProgram,"textcolor",textcolor.toString());
	programs->getChildWithName("GlobalSettings").setProperty("lastBank",curBank,0);
	programs->getChildWithName("GlobalSettings").setProperty("lastProgram",curProgram,0);
	programs->getChildWithName("GlobalSettings").setProperty("noteInput",noteInput,0);
	programs->getChildWithName("GlobalSettings").setProperty("usePC",usePC,0);

	programs->writeToStream(MemoryOutputStream(destData,false));
}

void imagePluginFilter::setCurrentProgramStateInformation (const void* data, int sizeInBytes)
{
    // use this helper function to get the XML from this binary blob..
    ScopedPointer<XmlElement> const xmlState = getXmlFromBinary (data, sizeInBytes);

    if (xmlState != 0)
    {
        // check that it's the right type of xml..
        if (xmlState->hasTagName ("MYPLUGINSETTINGS"))
        {
            // ok, now pull out our parameters..
			changeProgramName(getCurrentProgram(),xmlState->getStringAttribute ("progname", "Default"));
			for (int i=0;i<kNumParams;i++) {
				param[i] = (float) xmlState->getDoubleAttribute (String(i), param[i]);
			}
			lastUIWidth = xmlState->getIntAttribute ("uiWidth", lastUIWidth);
			lastUIHeight = xmlState->getIntAttribute ("uiHeight", lastUIHeight);
			icon = xmlState->getStringAttribute ("icon", icon);
			text = xmlState->getStringAttribute ("text", text);
			bgcolor = Colour(xmlState->getIntAttribute ("bgcolor", bgcolor.getARGB()) | 0x00000000);
			textcolor = Colour(xmlState->getIntAttribute ("textcolor", bgcolor.contrasting(0.8f).getARGB()) | 0x00000000);
			noteInput = xmlState->getBoolAttribute ("noteInput", noteInput);
			usePC = xmlState->getBoolAttribute ("usePC", usePC);
            
			sendChangeMessage ();
		}
    }
}

void imagePluginFilter::setStateInformation (const void* data, int sizeInBytes) {
    ScopedPointer<XmlElement> const xmlState = getXmlFromBinary (data, sizeInBytes);

    if (xmlState == 0)
	{
		ValueTree vt = ValueTree::readFromStream(MemoryInputStream(data,sizeInBytes,false));
		if (vt.isValid())
		{
			programs->removeAllChildren(0);
			for (int i=0;i<vt.getNumChildren();i++)
			{
				programs->addChild(vt.getChild(i).createCopy(),i,0);
			}
		}
		init=true;
		setCurrentBank(vt.getChild(128).getProperty("lastBank",0),vt.getChild(128).getProperty("lastProgram",0));
	}
	else
    {
        if (xmlState->hasTagName ("MYPLUGINSETTINGS"))
        {
			if (xmlState->getIntAttribute("pluginVersion")<2)
			{
				fullscreen = xmlState->getBoolAttribute ("fullscreen", false);
				for (int p=0;p<getNumPrograms();p++) {
					String prefix = "P" + String(p) + "."; 
					programs->set(0,p,"channel",(float) xmlState->getDoubleAttribute (prefix+String(0), 0));
					programs->set(0,p,"w",xmlState->getIntAttribute (prefix+"uiWidth", 400));
					programs->set(0,p,"h", xmlState->getIntAttribute (prefix+"uiHeight", 400));
					programs->set(0,p,"icon", xmlState->getStringAttribute (prefix+"icon", String::empty));
					programs->set(0,p,"text", xmlState->getStringAttribute (prefix+"text", String::empty));
					programs->set(0,p,"bgcolor", Colour(xmlState->getIntAttribute (prefix+"bgcolor")).toString());
					programs->set(0,p,"textcolor", Colour(xmlState->getIntAttribute ("textcolor")).toString());
					programs->set(0,p,"name", xmlState->getStringAttribute (prefix+"progname", String::empty));
				}
				init=true;
				setCurrentBank(0,xmlState->getIntAttribute("program", 0));
			}
			else 
			{
				//fullscreen = xmlState->getBoolAttribute ("fullscreen", false);
				//for (int b=0;b<128;b++) {
				//	XmlElement* xmlBank = xmlState->getChildByName("Bank"+String(b));
				//	for (int p=0;p<getNumPrograms();p++) {
				//		int prog = b*128+p;
				//		String prefix = "P" + String(p) + "_"; 
				//		for (int i=0;i<kNumParams;i++) {
				//			programs[prog].param[i] = (float) xmlBank->getDoubleAttribute (prefix+String(i), programs[prog].param[i]);
				//		}
				//		programs[prog].lastUIWidth = xmlBank->getIntAttribute (prefix+"uiWidth", programs[prog].lastUIWidth);
				//		programs[prog].lastUIHeight = xmlBank->getIntAttribute (prefix+"uiHeight", programs[prog].lastUIHeight);
				//		programs[prog].icon = xmlBank->getStringAttribute (prefix+"icon", programs[prog].icon);
				//		programs[prog].text = xmlBank->getStringAttribute (prefix+"text", programs[prog].text);
				//		programs[prog].bgcolor = Colour(xmlBank->getIntAttribute (prefix+"bgcolor", programs[prog].bgcolor.getARGB()) | 0xFF000000);
				//		programs[prog].textcolor = Colour(xmlBank->getIntAttribute ("textcolor", programs[prog].bgcolor.contrasting(0.8f).getARGB()) | 0x00000000);
				//		programs[prog].name = xmlBank->getStringAttribute (prefix+"progname", programs[prog].name);
				//	}
				//}
				//init=true;
				//setCurrentBank(xmlState->getIntAttribute("bank"),xmlState->getIntAttribute("program"));
			}
		}
    }
}

void imagePluginFilter::setBankColours(Colour colour, Colour text)
{
	for (int i=0;i<getNumPrograms();i++) 
	{
		programs->set(curBank,i,"bgcolor",colour.toString());
		programs->set(curBank,i,"textcolor",text.toString());
	}
}

void imagePluginFilter::applySizeToBank(int h, int w)
{
	for (int i=0;i<getNumPrograms();i++)
	{
		programs->set(curBank,i,"h",h);
		programs->set(curBank,i,"w",w);
	}
}

void imagePluginFilter::clearAllImages()
{
	for (int i=0;i<getNumPrograms();i++)
		programs->set(curBank,i,"icon", String(""));
}
