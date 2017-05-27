/************************************************************
************************************************************/
#include "ofApp.h"


/************************************************************
************************************************************/

/******************************
******************************/
ofApp::ofApp(int _soundStream_Input_DeviceId, int _soundStream_Output_DeviceId)
: soundStream_Input_DeviceId(_soundStream_Input_DeviceId)
, soundStream_Output_DeviceId(_soundStream_Output_DeviceId)
, Osc_MusicPlayer("127.0.0.1", 12354, 12353)
{
}

/******************************
******************************/
ofApp::~ofApp()
{
}

//--------------------------------------------------------------
void ofApp::setup(){
	/********************
	********************/
	ofSetWindowTitle("FFT");
	ofSetVerticalSync(true);
	ofSetFrameRate(30);
	ofSetWindowShape(WIDTH, HEIGHT);
	ofSetEscapeQuitsApp(false);
	
	ofEnableBlendMode(OF_BLENDMODE_ADD);
	// ofEnableSmoothing();
	
	/********************
	********************/
	soundStream.listDevices();
	if( (soundStream_Input_DeviceId == -1) || (soundStream_Output_DeviceId == -1) ){
		ofExit();
		return;
	}
	// soundStream.setDeviceID(soundStream_DeviceId);
	/* set in & out respectively. */
	soundStream.setInDeviceID(soundStream_Input_DeviceId);  
	soundStream.setOutDeviceID(soundStream_Output_DeviceId);
	
	// soundStream.setup(this, 2/* out */, 2/* in */, AUDIO_SAMPLERATE, AUDIO_BUF_SIZE, AUDIO_BUFFERS);
	soundStream.setup(this, 0/* out */, 2/* in */, AUDIO_SAMPLERATE, AUDIO_BUF_SIZE, AUDIO_BUFFERS);
	
	AudioSample.resize(AUDIO_BUF_SIZE);
	
	/********************
	********************/
	img_back.load("stage.jpg");
	
	fft_Filter.setup();
}

/******************************
******************************/
void ofApp::exit()
{
	/********************
	ofAppとaudioが別threadなので、ここで止めておくのが安全.
	********************/
	soundStream.stop();
	soundStream.close();
	
	/********************
	********************/
	printf("\n> Good bye\n");
}

//--------------------------------------------------------------
void ofApp::update(){
	/********************
	********************/
	while(Osc_MusicPlayer.OscReceive.hasWaitingMessages()){
		ofxOscMessage m_receive;
		Osc_MusicPlayer.OscReceive.getNextMessage(&m_receive);
		
		if(m_receive.getAddress() == "/Quit"){
			ofExit();
			return;
			// std::exit(1);
		}
	}

	/********************
	********************/
	fft_Filter.lock();
	fft_Filter.update();
	fft_Filter.unlock();
}

//--------------------------------------------------------------
void ofApp::draw(){
	/********************
	********************/
	ofBackground(0);
	
	/********************
	********************/
	img_back.draw(0, 0, ofGetWidth(), ofGetHeight());
	
	/********************
	********************/
	fft_Filter.lock();
	fft_Filter.draw();
	fft_Filter.unlock();
}

/******************************
audioIn/ audioOut
	同じthreadで動いている様子。
	また、audioInとaudioOutは、同時に呼ばれることはない(多分)。
	つまり、ofAppからaccessがない限り、変数にaccessする際にlock/unlock する必要はない。
	ofApp側からaccessする時は、threadを立てて、安全にpassする仕組みが必要(cf:NotUsed__thread_safeAccess.h)
******************************/
void ofApp::audioIn(float *input, int bufferSize, int nChannels)
{
    for (int i = 0; i < bufferSize; i++) {
        AudioSample.Left[i] = input[2*i];
		AudioSample.Right[i] = input[2*i+1];
    }
	
	/********************
	FFT Filtering
	1 process / block.
	********************/
	fft_Filter.lock();
	fft_Filter.update_fftGain(AudioSample.Left);
	fft_Filter.unlock();
}  

/******************************
******************************/
void ofApp::audioOut(float *output, int bufferSize, int nChannels)
{
#if 0
	/********************
	********************/
	float now_sec = ofGetElapsedTimef();
	
	/********************
	input -> output
	********************/
    for (int i = 0; i < bufferSize; i++) {
		/* */
		output[2*i] = AudioSample.Left[i];
		output[2*i+1] = AudioSample.Right[i];
    }
	
	/********************
	FFT Filtering
	1 process / block.
	********************/
	fft_Filter.lock();
	fft_Filter.update_fftGain(AudioSample.Left);
	fft_Filter.unlock();
#endif
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	switch(key){
		case ' ':
			ofSaveScreen("image.png");
			printf("image saved\n");
			
			break;
			
		case 'd':
			fft_Filter.lock();
			fft_Filter.Toggle_DispGui();
			fft_Filter.unlock();
			
			break;
			
		case 'l':
			fft_Filter.lock();
			fft_Filter.load_GuiSetting();
			fft_Filter.unlock();
			break;
			
		case 's':
			fft_Filter.lock();
			fft_Filter.save_GuiSetting();
			fft_Filter.unlock();
			break;
			
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
