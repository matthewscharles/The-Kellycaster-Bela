/*
Kellycaster string detection/Pd interface
Charles Matthews based on ideas by John Kelly 2018
Based on the Bela custom render.cpp


I've created a few workarounds in here, e.g. converting incoming OSC to MIDI messages. 
Not sure if there are better ways around this..still early days for me in C++

*/

/*
 ____  _____ _        _    
| __ )| ____| |      / \   
|  _ \|  _| | |     / _ \  
| |_) | |___| |___ / ___ \ 
|____/|_____|_____/_/   \_\

The platform for ultra-low latency audio and sensor processing

http://bela.io

A project of the Augmented Instruments Laboratory within the
Centre for Digital Music at Queen Mary University of London.
http://www.eecs.qmul.ac.uk/~andrewm

(c) 2016 Augmented Instruments Laboratory: Andrew McPherson,
	Astrid Bin, Liam Donovan, Christian Heinrichs, Robert Jack,
	Giulio Moro, Laurel Pardue, Victor Zappi. All rights reserved.

The Bela software is distributed under the GNU Lesser General Public License
(LGPL 3.0), available here: https://www.gnu.org/licenses/lgpl-3.0.txt
*/


#include <Bela.h>
#include <DigitalChannelManager.h>
#include <cmath>
#include <I2c_Codec.h>
#include <PRU.h>
#include <stdio.h>
#include <libpd/z_libpd.h>
#include <libpd/s_stuff.h>
#include <UdpServer.h>
#include <Midi.h>
#include <Scope.h>


#include <OSCServer.h>
#include <OSCClient.h>

OSCServer oscServer;
OSCClient oscClient;


// if you are 100% sure of what value was used to compile libpd/puredata, then
// you could #define gBufLength instead of getting it at runtime. It has proved to give some 0.3%
// performance boost when it is 8 (thanks to vectorize optimizations I guess).
int gBufLength;

float* gInBuf;
float* gOutBuf;

void pdnoteon(int ch, int pitch, int vel) {
  printf("noteon: %d %d %d\n", ch, pitch, vel);
}

void Bela_printHook(const char *recv){
	rt_printf("%s", recv);
}
#define PARSE_MIDI
static Midi midi;
static DigitalChannelManager dcm;

void sendDigitalMessage(bool state, unsigned int delay, void* receiverName){
	libpd_float((char*)receiverName, (float)state);
//	rt_printf("%s: %d\n", (char*)receiverName, state);
}

#define LIBPD_DIGITAL_OFFSET 11 // digitals are preceded by 2 audio and 8 analogs (even if using a different number of analogs)

void Bela_messageHook(const char *source, const char *symbol, int argc, t_atom *argv){

	
	if(strcmp(source, "bela_setDigital") == 0){
		// symbol is the direction, argv[0] is the channel, argv[1] (optional)
		// is signal("sig" or "~") or message("message", default) rate
		bool isMessageRate = true; // defaults to message rate
		bool direction = 0; // initialize it just to avoid the compiler's warning
		bool disable = false;
		if(strcmp(symbol, "in") == 0){
			direction = INPUT;
		} else if(strcmp(symbol, "out") == 0){
			direction = OUTPUT;
		} else if(strcmp(symbol, "disable") == 0){
			disable = true;
		} else {
				oscClient.sendMessageNow(oscClient.newMessage.to("/test").add(symbol).end());
			return;
		}
		if(argc == 0){
			return;
		} else if (libpd_is_float(&argv[0]) == false){
			return;
		}
		int channel = libpd_get_float(&argv[0]) - LIBPD_DIGITAL_OFFSET;
		if(disable == true){
			dcm.unmanage(channel);
			return;
		}
		if(argc >= 2){
			t_atom* a = &argv[1];
			if(libpd_is_symbol(a)){
				char *s = libpd_get_symbol(a);
				if(strcmp(s, "~") == 0  || strncmp(s, "sig", 3) == 0){
					isMessageRate = false;
				}
			}
		}
		dcm.manage(channel, direction, isMessageRate);
	}
}

void Bela_floatHook(const char *source, float value){

	/*
	 *  MODIFICATION
 	 *  ------------
	 *  Parse float sent to receiver '.noteRate' and assign it to a global variable
	 *  N.B. When using libpd receiver names need to be registered (see setup() function below)
	 */
	 
	//again surely these things could be formatted as switches but I'm not quite sure how this'd work with string compare
	if(strncmp(source, "string1", 11) == 0){
			oscClient.sendMessageNow(oscClient.newMessage.to("/1").add(value).end());
			
	} else 
	
		if(strncmp(source, "string2", 11) == 0){
			oscClient.sendMessageNow(oscClient.newMessage.to("/2").add(value).end());
			
	}  else 
	
		if(strncmp(source, "string3", 11) == 0){
			oscClient.sendMessageNow(oscClient.newMessage.to("/3").add(value).end());
			
	}  else 
	
		if(strncmp(source, "string4", 11) == 0){
			oscClient.sendMessageNow(oscClient.newMessage.to("/4").add(value).end());
			
	}  else 
	
		if(strncmp(source, "string5", 11) == 0){
			oscClient.sendMessageNow(oscClient.newMessage.to("/5").add(value).end());
			
	}  else 
	
		if(strncmp(source, "string6", 11) == 0){
			oscClient.sendMessageNow(oscClient.newMessage.to("/6").add(value).end());
			
	}
	
	

	/*********/

	// let's make this as optimized as possible for built-in digital Out parsing
	// the built-in digital receivers are of the form "bela_digitalOutXX" where XX is between 11 and 26
	static int prefixLength = 15; // strlen("bela_digitalOut")
	if(strncmp(source, "bela_digitalOut", prefixLength)==0){
		if(source[prefixLength] != 0){ //the two ifs are used instead of if(strlen(source) >= prefixLength+2)
			if(source[prefixLength + 1] != 0){
				// quickly convert the suffix to integer, assuming they are numbers, avoiding to call atoi
				int receiver = ((source[prefixLength] - 48) * 10);
				receiver += (source[prefixLength+1] - 48);
				unsigned int channel = receiver - 11; // go back to the actual Bela digital channel number
				if(channel < 16){ //16 is the hardcoded value for the number of digital channels
					dcm.setValue(channel, value);
				}
			}
		}
	}
}

char receiverNames[16][21]={
	{"bela_digitalIn11"},{"bela_digitalIn12"},{"bela_digitalIn13"},{"bela_digitalIn14"},{"bela_digitalIn15"},
	{"bela_digitalIn16"},{"bela_digitalIn17"},{"bela_digitalIn18"},{"bela_digitalIn19"},{"bela_digitalIn20"},
	{"bela_digitalIn21"},{"bela_digitalIn22"},{"bela_digitalIn23"},{"bela_digitalIn24"},{"bela_digitalIn25"},
	{"bela_digitalIn26"}
};

static unsigned int gAnalogChannelsInUse;
static unsigned int gLibpdBlockSize;
// 2 audio + (up to)8 analog + (up to) 16 digital + 4 scope outputs
static const unsigned int gChannelsInUse = 30;
//static const unsigned int gFirstAudioChannel = 0;
static const unsigned int gFirstAnalogChannel = 2;
static const unsigned int gFirstDigitalChannel = 10;
static const unsigned int gFirstScopeChannel = 26;

Scope scope;
unsigned int gScopeChannelsInUse = 4;
float* gScopeOut;

 void parseMessage(oscpkt::Message msg){ //from examples
    
     rt_printf("received message to: %s\n", msg.addressPattern().c_str());
    
     int intArg;
	 	if (msg.match("/calibrate").popInt32(intArg).isOkNoMoreArgs()){
         rt_printf("calibrate int %i \n", intArg);
         libpd_controlchange(0, 127, intArg);
         //libpd_programchange(0, intArg);
     }
     
     //OSC to MIDI
     
     //this is long-winded, can I get this into a switch format?
     //(I'm not sure about this msg.match stuff..)
     //!!check the channels, start at 0 or 1?
     
     if (msg.match("/thresh/low/1").popInt32(intArg).isOkNoMoreArgs()){
         rt_printf("calibrate int %i \n", intArg);
         libpd_controlchange(3, 1, intArg);
         //libpd_programchange(0, intArg);
     }
     
     if (msg.match("/thresh/low/2").popInt32(intArg).isOkNoMoreArgs()){
         rt_printf("calibrate int %i \n", intArg);
         libpd_controlchange(3, 2, intArg);
         //libpd_programchange(0, intArg);
     }
     
      if (msg.match("/thresh/low/3").popInt32(intArg).isOkNoMoreArgs()){
         rt_printf("calibrate int %i \n", intArg);
         libpd_controlchange(3, 3, intArg);
         //libpd_programchange(0, intArg);
     }
     
   if (msg.match("/thresh/low/4").popInt32(intArg).isOkNoMoreArgs()){
         rt_printf("calibrate int %i \n", intArg);
         libpd_controlchange(3, 4, intArg);
         //libpd_programchange(0, intArg);
     }
     
        if (msg.match("/thresh/low/5").popInt32(intArg).isOkNoMoreArgs()){
         rt_printf("calibrate int %i \n", intArg);
         libpd_controlchange(3, 5, intArg);
         //libpd_programchange(0, intArg);
     }
     
        if (msg.match("/thresh/low/6").popInt32(intArg).isOkNoMoreArgs()){
         rt_printf("calibrate int %i \n", intArg);
         libpd_controlchange(3, 6, intArg);
         //libpd_programchange(0, intArg);
     }
     
       //this is long-winded, can I get this into a switch format?
     //!!check the channels, start at 0 or 1?
     
     if (msg.match("/thresh/high/1").popInt32(intArg).isOkNoMoreArgs()){
         rt_printf("calibrate int %i \n", intArg);
         libpd_controlchange(4, 1, intArg);
         //libpd_programchange(0, intArg);
     }
     
     if (msg.match("/thresh/high/2").popInt32(intArg).isOkNoMoreArgs()){
         rt_printf("calibrate int %i \n", intArg);
         libpd_controlchange(4, 2, intArg);
         //libpd_programchange(0, intArg);
     }
     
      if (msg.match("/thresh/high/3").popInt32(intArg).isOkNoMoreArgs()){
         rt_printf("calibrate int %i \n", intArg);
         libpd_controlchange(4, 3, intArg);
         //libpd_programchange(0, intArg);
     }
     
   if (msg.match("/thresh/high/4").popInt32(intArg).isOkNoMoreArgs()){
         rt_printf("calibrate int %i \n", intArg);
         libpd_controlchange(4, 4, intArg);
         //libpd_programchange(0, intArg);
     }
     
        if (msg.match("/thresh/high/5").popInt32(intArg).isOkNoMoreArgs()){
         rt_printf("calibrate int %i \n", intArg);
         libpd_controlchange(4, 5, intArg);
         //libpd_programchange(0, intArg);
     }
     
        if (msg.match("/thresh/high/6").popInt32(intArg).isOkNoMoreArgs()){
         rt_printf("calibrate int %i \n", intArg);
         libpd_controlchange(4, 6, intArg);
         //libpd_programchange(0, intArg);
     }
     
        //this is long-winded, can I get this into a switch format?
     //!!check the channels, start at 0 or 1?
     
     if (msg.match("/minvel/1").popInt32(intArg).isOkNoMoreArgs()){
         rt_printf("calibrate int %i \n", intArg);
         libpd_controlchange(2, 1, intArg);
         //libpd_programchange(0, intArg);
     }
     
     if (msg.match("/minvel/2").popInt32(intArg).isOkNoMoreArgs()){
         rt_printf("calibrate int %i \n", intArg);
         libpd_controlchange(2, 2, intArg);
         //libpd_programchange(0, intArg);
     }
     
      if (msg.match("/minvel/3").popInt32(intArg).isOkNoMoreArgs()){
         rt_printf("calibrate int %i \n", intArg);
         libpd_controlchange(2, 3, intArg);
         //libpd_programchange(0, intArg);
     }
     
   if (msg.match("/minvel/4").popInt32(intArg).isOkNoMoreArgs()){
         rt_printf("calibrate int %i \n", intArg);
         libpd_controlchange(2, 4, intArg);
         //libpd_programchange(0, intArg);
     }
     
        if (msg.match("/minvel/5").popInt32(intArg).isOkNoMoreArgs()){
         rt_printf("calibrate int %i \n", intArg);
         libpd_controlchange(2, 5, intArg);
         //libpd_programchange(0, intArg);
     }
     
        if (msg.match("/minvel/6").popInt32(intArg).isOkNoMoreArgs()){
         rt_printf("calibrate int %i \n", intArg);
         libpd_controlchange(2, 6, intArg);
         //libpd_programchange(0, intArg);
     }
     
 }

bool setup(BelaContext *context, void *userData)
{

// setup the OSC server to receive on port 7562
    oscServer.setup(7562);
    // setup the OSC client to send on port 7563
    oscClient.setup(7563, "192.168.7.1");
    int test = rand();
	oscClient.sendMessageNow(oscClient.newMessage.to("/note").add(test).end());
		

    scope.setup(gScopeChannelsInUse, context->audioSampleRate);
    gScopeOut = new float[gScopeChannelsInUse];

	// Check first of all if file exists. Will actually open it later.
	char file[] = "_main.pd";
	char folder[] = "./";
	unsigned int strSize = strlen(file) + strlen(folder) + 1;
	char* str = (char*)malloc(sizeof(char) * strSize);
	snprintf(str, strSize, "%s%s", folder, file);
	if(access(str, F_OK) == -1 ) {
		printf("Error file %s/%s not found. The %s file should be your main patch.\n", folder, file, file);
		return false;
	}
	if(context->analogInChannels != context->analogOutChannels ||
			context->audioInChannels != context->audioOutChannels){
		printf("This project requires the number of inputs and the number of outputs to be the same\n");
		return false;
	}
	// analog setup
	gAnalogChannelsInUse = context->analogInChannels;

	// digital setup
	dcm.setCallback(sendDigitalMessage);
	if(context->digitalChannels > 0){
		for(unsigned int ch = 0; ch < context->digitalChannels; ++ch){
			dcm.setCallbackArgument(ch, receiverNames[ch]);
		}
	}

	midi.readFrom(0);
	midi.writeTo(0);
#ifdef PARSE_MIDI
	midi.enableParser(true);
#else
	midi.enableParser(false);
#endif /* PARSE_MIDI */
//	udpServer.bindToPort(1234);

	gLibpdBlockSize = libpd_blocksize();
	// check that we are not running with a blocksize smaller than gLibPdBlockSize
	// We could still make it work, but the load would be executed unevenly between calls to render
	if(context->audioFrames < gLibpdBlockSize){
		fprintf(stderr, "Error: minimum block size must be %d\n", gLibpdBlockSize);
		return false;
	}
	// set hooks before calling libpd_init
	libpd_set_printhook(Bela_printHook);
	libpd_set_floathook(Bela_floatHook);
	libpd_set_messagehook(Bela_messageHook);
	libpd_set_noteonhook(pdnoteon);
	//TODO: add hooks for other midi events and generate MIDI output appropriately
	libpd_init();
	//TODO: ideally, we would analyse the ASCII of the patch file and find out which in/outs to use
	libpd_init_audio(gChannelsInUse, gChannelsInUse, context->audioSampleRate);
	gInBuf = libpd_get_sys_soundin();
	gOutBuf = libpd_get_sys_soundout();

	libpd_start_message(1); // one entry in list
	libpd_add_float(1.0f);
	libpd_finish_message("pd", "dsp");

	gBufLength = max(gLibpdBlockSize, context->audioFrames);


	// bind your receivers here
	libpd_bind("bela_digitalOut11");
	libpd_bind("bela_digitalOut12");
	libpd_bind("bela_digitalOut13");
	libpd_bind("bela_digitalOut14");
	libpd_bind("bela_digitalOut15");
	libpd_bind("bela_digitalOut16");
	libpd_bind("bela_digitalOut17");
	libpd_bind("bela_digitalOut18");
	libpd_bind("bela_digitalOut19");
	libpd_bind("bela_digitalOut20");
	libpd_bind("bela_digitalOut21");
	libpd_bind("bela_digitalOut22");
	libpd_bind("bela_digitalOut23");
	libpd_bind("bela_digitalOut24");
	libpd_bind("bela_digitalOut25");
	libpd_bind("bela_digitalOut26");
	libpd_bind("bela_setDigital");
	/*
	 *  MODIFICATION
	 *  ------------
	 *  Bind an additional receiver for the tremoloRate parameter
	 */
	libpd_bind("string1");
		libpd_bind("string2");
		libpd_bind("string3");
		libpd_bind("string4");
		libpd_bind("string5");
		libpd_bind("string6");
	/*********/

	// open patch       [; pd open file folder(
	void* patch = libpd_openfile(file, folder);
	if(patch == NULL){
		printf("Error: file %s/%s is corrupted.\n", folder, file); 
		return false;
	}
	return true;
}

// render() is called regularly at the highest priority by the audio engine.
// Input and output are given from the audio hardware and the other
// ADCs and DACs (if available). If only audio is available, numMatrixFrames
// will be 0.



void render(BelaContext *context, void *userData)
{
	
	 while (oscServer.messageWaiting()){
	 	
       parseMessage(oscServer.popMessage());
        oscClient.queueMessage(oscClient.newMessage.to("/osc-acknowledge").add(5).add(4.2f).add(std::string("OSC message received")).end());
    }
    
	int num;
	// the safest thread-safe option to handle MIDI input is to process the MIDI buffer
	// from the audio thread.
#ifdef PARSE_MIDI
	while((num = midi.getParser()->numAvailableMessages()) > 0){
		static MidiChannelMessage message;
		message = midi.getParser()->getNextChannelMessage();
		//message.prettyPrint(); // use this to print beautified message (channel, data bytes)
		switch(message.getType()){
			case kmmNoteOn:
			{
				int noteNumber = message.getDataByte(0);
				int velocity = message.getDataByte(1);
				int channel = message.getChannel();
				libpd_noteon(channel, noteNumber, velocity);
				break;
			}
			case kmmNoteOff:
			{
				/* PureData does not seem to handle noteoff messages as per the MIDI specs,
				 * so that the noteoff velocity is ignored. Here we convert them to noteon
				 * with a velocity of 0.
				 */
				int noteNumber = message.getDataByte(0);
//				int velocity = message.getDataByte(1); // would be ignored by Pd
				int channel = message.getChannel();
				libpd_noteon(channel, noteNumber, 0);
				break;
			}
			case kmmControlChange:
			{
				int channel = message.getChannel();
				int controller = message.getDataByte(0);
				int value = message.getDataByte(1);
				libpd_controlchange(channel, controller, value);
				break;
			}
			case kmmProgramChange:
			{
				int channel = message.getChannel();
				int program = message.getDataByte(0);
				libpd_programchange(channel, program);
				break;
			}
			case kmmPolyphonicKeyPressure:
			{
				int channel = message.getChannel();
				int pitch = message.getDataByte(0);
				int value = message.getDataByte(1);
				libpd_polyaftertouch(channel, pitch, value);
				break;
			}
			case kmmChannelPressure:
			{
				int channel = message.getChannel();
				int value = message.getDataByte(0);
				libpd_aftertouch(channel, value);
				break;
			}
			case kmmPitchBend:
			{
				int channel = message.getChannel();
				int value =  ((message.getDataByte(1) << 7)| message.getDataByte(0)) - 8192;
				libpd_pitchbend(channel, value);
				break;
			}
			case kmmNone:
			case kmmAny:
				break;
		}
	}
#else
	int input;
	while((input = midi.getInput()) >= 0){
		libpd_midibyte(0, input);
	}
#endif /* PARSE_MIDI */

	static unsigned int numberOfPdBlocksToProcess = gBufLength / gLibpdBlockSize;

	for(unsigned int tick = 0; tick < numberOfPdBlocksToProcess; ++tick){
		unsigned int audioFrameBase = gLibpdBlockSize * tick;
		unsigned int j;
		unsigned int k;
		float* p0;
		float* p1;
		for (j = 0, p0 = gInBuf; j < gLibpdBlockSize; j++, p0++) {
			for (k = 0, p1 = p0; k < context->audioInChannels; k++, p1 += gLibpdBlockSize) {
				*p1 = audioRead(context, audioFrameBase + j, k);
			}
		}
		// then analogs
		// this loop resamples by ZOH, as needed, using m
		if(context->analogInChannels == 8 ){ //hold the value for two frames
			for (j = 0, p0 = gInBuf; j < gLibpdBlockSize; j++, p0++) {
				for (k = 0, p1 = p0 + gLibpdBlockSize * gFirstAnalogChannel; k < gAnalogChannelsInUse; ++k, p1 += gLibpdBlockSize) {
					unsigned int analogFrame = (audioFrameBase + j) / 2;
					*p1 = analogRead(context, analogFrame, k);
				}
			}
		} else if(context->analogInChannels == 4){ //write every frame
			for (j = 0, p0 = gInBuf; j < gLibpdBlockSize; j++, p0++) {
				for (k = 0, p1 = p0 + gLibpdBlockSize * gFirstAnalogChannel; k < gAnalogChannelsInUse; ++k, p1 += gLibpdBlockSize) {
					unsigned int analogFrame = audioFrameBase + j;
					*p1 = analogRead(context, analogFrame, k);
				}
			}
		} else if(context->analogInChannels == 2){ //drop every other frame
			for (j = 0, p0 = gInBuf; j < gLibpdBlockSize; j++, p0++) {
				for (k = 0, p1 = p0 + gLibpdBlockSize * gFirstAnalogChannel; k < gAnalogChannelsInUse; ++k, p1 += gLibpdBlockSize) {
					unsigned int analogFrame = (audioFrameBase + j) * 2;
					*p1 = analogRead(context, analogFrame, k);
				}
			}
		}

		// Bela digital input
		// note: in multiple places below we assume that the number of digitals is same as number of audio
		// digital in at message-rate
		dcm.processInput(&context->digital[audioFrameBase], gLibpdBlockSize);

		// digital in at signal-rate
		for (j = 0, p0 = gInBuf; j < gLibpdBlockSize; j++, p0++) {
			unsigned int digitalFrame = audioFrameBase + j;
			for (k = 0, p1 = p0 + gLibpdBlockSize * gFirstDigitalChannel;
					k < 16; ++k, p1 += gLibpdBlockSize) {
				if(dcm.isSignalRate(k) && dcm.isInput(k)){ // only process input channels that are handled at signal rate
					*p1 = digitalRead(context, digitalFrame, k);
				}
			}
		}

		libpd_process_sys(); // process the block

		//digital out
		// digital out at signal-rate
		for (j = 0, p0 = gOutBuf; j < gLibpdBlockSize; ++j, ++p0) {
			unsigned int digitalFrame = (audioFrameBase + j);
			for (k = 0, p1 = p0  + gLibpdBlockSize * gFirstDigitalChannel;
					k < context->digitalChannels; k++, p1 += gLibpdBlockSize) {
				if(dcm.isSignalRate(k) && dcm.isOutput(k)){ // only process output channels that are handled at signal rate
					digitalWriteOnce(context, digitalFrame, k, *p1 > 0.5);
				}
			}
		}

		// digital out at message-rate
		dcm.processOutput(&context->digital[audioFrameBase], gLibpdBlockSize);

		//audio
		for (j = 0, p0 = gOutBuf; j < gLibpdBlockSize; j++, p0++) {



			for (k = 0, p1 = p0; k < context->audioOutChannels; k++, p1 += gLibpdBlockSize) {
				// audioWrite(context, audioFrameBase + j, k, *p1 * lfo); // MODIFICATION (* lfo)
				audioWrite(context, audioFrameBase + j, k, *p1); // MODIFICATION (* lfo)
			}
		}

		//scope
		for (j = 0, p0 = gOutBuf; j < gLibpdBlockSize; ++j, ++p0) {
			for (k = 0, p1 = p0  + gLibpdBlockSize * gFirstScopeChannel; k < gScopeChannelsInUse; k++, p1 += gLibpdBlockSize) {
				gScopeOut[k] = *p1;
			}
			scope.log(gScopeOut[0], gScopeOut[1], gScopeOut[2], gScopeOut[3]);
		}


		//analog
		if(context->analogOutChannels == 8){
			for (j = 0, p0 = gOutBuf; j < gLibpdBlockSize; j += 2, p0 += 2) { //write every two frames
				unsigned int analogFrame = (audioFrameBase + j) / 2;
				for (k = 0, p1 = p0 + gLibpdBlockSize * gFirstAnalogChannel; k < gAnalogChannelsInUse; k++, p1 += gLibpdBlockSize) {
					analogWriteOnce(context, analogFrame, k, *p1);
				}
			}
		} else if(context->analogOutChannels == 4){ //write every frame
			for (j = 0, p0 = gOutBuf; j < gLibpdBlockSize; ++j, ++p0) {
				unsigned int analogFrame = (audioFrameBase + j);
				for (k = 0, p1 = p0  + gLibpdBlockSize * gFirstAnalogChannel; k < gAnalogChannelsInUse; k++, p1 += gLibpdBlockSize) {
					analogWriteOnce(context, analogFrame, k, *p1);
				}
			}
		} else if(context->analogOutChannels == 2){ //write every frame twice
			for (j = 0, p0 = gOutBuf; j < gLibpdBlockSize; j++, p0++) {
				for (k = 0, p1 = p0 + gLibpdBlockSize * gFirstAnalogChannel; k < gAnalogChannelsInUse; k++, p1 += gLibpdBlockSize) {
					int analogFrame = audioFrameBase * 2 + j * 2;
					analogWriteOnce(context, analogFrame, k, *p1);
					analogWriteOnce(context, analogFrame + 1, k, *p1);
				}
			}
		}
	}
}

// cleanup() is called once at the end, after the audio has stopped.
// Release any resources that were allocated in setup().

void cleanup(BelaContext *context, void *userData)
{
	delete [] gScopeOut;
}
