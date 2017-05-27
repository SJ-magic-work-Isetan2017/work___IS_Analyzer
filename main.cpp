#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main(int argc, char** argv){
	ofSetupOpenGL(1024,768,OF_WINDOW);			// <-------- setup the GL context

	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	
	int soundStream_Input_DeviceId;
	int soundStream_Output_DeviceId;
	if(argc < 3){
		/*
		soundStream_Input_DeviceId = -1;
		soundStream_Output_DeviceId = -1;
		*/
		soundStream_Input_DeviceId	= 2;
		soundStream_Output_DeviceId	= 1;
		
		printf("*.exe InputId OutputId\n\n");
		
	}else{
		soundStream_Input_DeviceId = atoi(argv[1]);
		soundStream_Output_DeviceId = atoi(argv[2]);
		
	}
	ofRunApp(new ofApp(soundStream_Input_DeviceId, soundStream_Output_DeviceId));
}
