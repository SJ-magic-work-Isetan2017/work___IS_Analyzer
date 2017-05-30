#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main(int argc, char** argv){
	ofSetupOpenGL(1024,768,OF_WINDOW);			// <-------- setup the GL context

	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	
	int soundStream_Input_DeviceId = -1;
	int soundStream_Output_DeviceId = -1;
	
	/********************
	********************/
	if(3 <= argc){
		soundStream_Input_DeviceId = atoi(argv[1]);
		soundStream_Output_DeviceId = atoi(argv[2]);
		
	}else if(argc == 2){
		soundStream_Input_DeviceId = -1;
		soundStream_Output_DeviceId = -1;
		
		printf("*.exe InputId OutputId\n\n");
		
	}else{
		FILE* fp = fopen("../../../data/config.txt", "r");
		if(fp == NULL){
			printf("config not exists\n");
			std::exit(1);
		}
		
		enum{
			BUF_SIZE = 512,
		};
		char buf[BUF_SIZE];
		
		while(fscanf(fp, "%s", buf) != EOF){
			if(strcmp(buf, "<input>") == 0){
				fscanf(fp, "%s", buf);
				soundStream_Input_DeviceId = atoi(buf);
			}else if(strcmp(buf, "<output>") == 0){
				fscanf(fp, "%s", buf);
				soundStream_Output_DeviceId = atoi(buf);
			}
		}
		
		fclose(fp);
		
	}
	
	/********************
	********************/
	ofRunApp(new ofApp(soundStream_Input_DeviceId, soundStream_Output_DeviceId));
}
