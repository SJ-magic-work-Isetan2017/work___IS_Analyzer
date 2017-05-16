/************************************************************
How to use
	Line input volumeが-1〜1の間でどの辺りで動作しているのかを設定
			ofMap(AudioSample_Rev[i], -AudioSample_Amp, AudioSample_Amp, -BarHeight, BarHeight)
		によってGraphの高さが決まっている。
		b_GainAdjustをonにしてBarHeightで規定されるSquareを表示した上で、このSquareに丁度いい(小さ過ぎず、はみ出し過ぎず)ように
		Audio I/F GainとAudioSample_Amp(Gui)を設定する。
		
	GraphのBar高さを設定
		BarHeight(Gui)で表示高さを設定。
		上の工程は、BarHeight内を、ほぼ100%効率良く使用するための設定で、こちらは実際の表示高さ。
	
note on Gui Parameters
	全体
		ofxToggle b_DispGain;
			時間空間でなく、周波数空間にてGain表示.
		
		ofxIntSlider ofs_x, ofs_y;
			表示位置
			
		ofxToggle b_GainAdjust;
			Audio I/F, AudioSample_Amp調整時にBarHeightで規定されるSquareを表示する。
			
		ofxFloatSlider AudioSample_Amp;
			Line input volumeが-1〜1の間で、どの辺りで動作しているかを設定するのに使用。
			Audio I/Fと合わせて設定。
			Graph Heightが上記Square内に、ほぼ100%となるように設定すればOK.
	
	ofxGuiGroup GuiGroup_GraphDispSetting;
		ofxToggle b_LineGraph;
			GraphをLineで表示
			
		ofxIntSlider BarHeight, BarWidth, BarSpace;
			Graph size.
		
		ofxToggle b_abs;
			Graphを±で表示するか、上半分に折り返して表示するか。
			
		ofxToggle b_AlphaBlend_Add;
			Graphのoverlay方法。
			OF_BLENDMODE_ADD or OF_BLENDMODE_ALPHA 
		
	ofxGuiGroup GuiGroup_FilterSetting;
		ofxIntSlider CutOff_From, CutOff_To;
			fft結果の内、from <= [i] <= to の成分を残し、他をzero clearする。
			[i]について、
			44.1kHz, 512 sampleの場合、512sampleは、11.6ms.
			x[1], y[1]は、1/11.6ms = 86Hzの成分。
			x[2], y[2]は、86 x 2 Hzの成分。
			
		ofxFloatSlider k_Smoothing;
			fftで周波数軸に写影した後、Filteringを施してから再び時間軸に逆変換で戻す。
			これに掛けるLPFの係数。
			
		ofxFloatSlider NonLinear_Range;
			同じくNonLinear Filterに使用。
			zeroの時、NonLinear Filterなし。
			
		ofxFloatSlider k_Smoothing_Gain;
			周波数空間上でのGain用LPF係数.
		
		ofxToggle b_phaseRotation;
		ofxFloatSlider phase_deg;
		ofxFloatSlider phaseRotaion_Speed;
			fftで周波数軸に写影した後、Filteringを施してから再び時間軸に逆変換で戻す。
			ここで、Filteringを施す時、phaseを揃えることで、時間軸に戻した後のGraphを整えている。
			phaseが固定値だと、動きが少なくなるので、
				b_phaseRotation = on
			の時、phaseを回すことにした。
		
		ofxFloatSlider PhaseNoise_Amp;
		ofxFloatSlider PhaseNoise_Speed_sec;
			b_phaseRotation = onの時、phaseを揃えつつも、回転させていくが、やはり綺麗に回転していくだけだと、退屈感が出てします。
			そこでperlin noiseを使い、phaseを各瞬間で揺らすこととした。
			これらは、そのためのparameter.
			
			PhaseNoise_Speed_secは、この時間ごとにnoise Level = 0となる値。つまりnoise周期と考えることができる。
			大きくすると、noise変化が時間方向に緩やかになる。
		
	ofxColorSlider BarColor;
		Graph color.
************************************************************/
#pragma once

/************************************************************
************************************************************/
#include "ofMain.h"
#include "ofxGui.h"
#include "ofxNetwork.h"

#include "sj_common.h"

/************************************************************
************************************************************/
// #define SJ_DEBUG__MEASTIME

/************************************************************
************************************************************/

/**************************************************
**************************************************/
class FFT : public ofThread{
private:
	/****************************************
	****************************************/
	enum{
		THREAD_SLEEP_MS = 1,
	};
	
	/****************************************
	****************************************/
	/********************
	********************/
#ifdef SJ_DEBUG__MEASTIME
	FILE* fp_Log;
#endif
	
	/********************
	********************/
	ofxPanel gui;
	
	/* */
	ofxToggle b_DispGain;
	
	/* */
	ofxIntSlider ofs_x;
	ofxIntSlider ofs_y;
	ofxToggle b_GainAdjust;
	ofxFloatSlider AudioSample_Amp;
	
	/* */
	ofxGuiGroup GuiGroup_GraphDispSetting;
	ofxToggle b_LineGraph;
	ofxIntSlider BarHeight;
	ofxIntSlider BarWidth;
	ofxIntSlider BarSpace;
	
	ofxToggle b_abs;
	ofxToggle b_AlphaBlend_Add;
	
	/* */
	ofxGuiGroup GuiGroup_FilterSetting;
	ofxIntSlider CutOff_From;
	ofxIntSlider CutOff_To;
	ofxFloatSlider k_Smoothing;
	ofxFloatSlider NonLinear_Range;
	ofxFloatSlider k_Smoothing_Gain;
	
	ofxToggle b_phaseRotation;
	ofxFloatSlider phase_deg;
	ofxFloatSlider phaseRotaion_Speed;
	
	ofxFloatSlider PhaseNoise_Amp;
	ofxFloatSlider PhaseNoise_Speed_sec;
	
	/* */
	ofxColorSlider BarColor;
	
	/********************
	********************/
	bool b_DispGui;
	
	const int N;
	
	ofVbo Vbo;
	vector<ofVec3f> VboVerts;
	vector<ofFloatColor> VboColor;
	
	vector<float> fft_Gain;
	vector<float> Last_fft_Gain;
	vector<float> AudioSample_Rev;
	vector<float> fft_window;
	
	vector<double> sintbl;
	vector<int> bitrev;
	
	float LastInt;
	
	/********************
	********************/
	ofxUDPManager udp_Vj;
	ofxUDPManager udp_Dmx;
	
	/****************************************
	****************************************/
	void RefreshVerts();
	void Refresh_BarColor();
	
	float cal_vector_ave(const vector<float> &data, int from, int num);
	
	int fft(double x[], double y[], int IsReverse = 0);
	void make_bitrev(void);
	void make_sintbl(void);
	
	double deg2rad(double deg);
	
	void SendUdp();
	
	
public:
	/****************************************
	****************************************/
	FFT();
	~FFT();
	
	void setup();
	void update();
	void update_fftGain(const vector<float> &AudioSample);
	void draw();
	
	void threadedFunction();
	
	void Toggle_DispGui();
	void save_GuiSetting();
	void load_GuiSetting();
};

