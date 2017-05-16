/************************************************************
************************************************************/
#include "fft.h"

/************************************************************
************************************************************/

/******************************
C++ Vector size is returning zero
	http://stackoverflow.com/questions/31372809/c-vector-size-is-returning-zero
	
	contents
		v1.reserve(30);
		does not change the number of elements in the vector, it just makes sure the vector has enough space to save 30 elements without reallocation.
		so now, v1.size() returns zero.
		
		Use
		v1.resize(30);
		to change to number of elements in the vector.
******************************/
FFT::FFT()
: N(AUDIO_BUF_SIZE / COMPRESS)
, LastInt(0)
, b_DispGui(true)
{
	/********************
	********************/
#ifdef SJ_DEBUG__MEASTIME
	fp_Log = fopen("../../../Log.csv", "w");
#endif	
	
	/********************
	********************/
	VboVerts.resize(N * 4); // square
	VboColor.resize(N * 4); // square
	
	fft_Gain.resize(N);
	Last_fft_Gain.resize(N);

	AudioSample_Rev.resize(N);
	
	/* 窓関数 */
	fft_window.resize(N);
	for(int i = 0; i < N; i++)	fft_window[i] = 0.5 - 0.5 * cos(2 * PI * i / N);
	
	sintbl.resize(N + N/4);
	bitrev.resize(N);
	
	/********************
	********************/
	make_bitrev();
	make_sintbl();
}

/******************************
******************************/
FFT::~FFT()
{
	/********************
	********************/
#ifdef SJ_DEBUG__MEASTIME
	fclose(fp_Log);
#endif	
}

/******************************
******************************/
void FFT::Toggle_DispGui()
{
	b_DispGui = !b_DispGui;
}

/******************************
******************************/
void FFT::setup()
{
	/********************
	********************/
	gui.setup();
	
	/* */
	gui.add(b_DispGain.setup("b_DispGain", false));

	/* */
	gui.add(ofs_x.setup("ofs_x", 115, -500, 500));
	gui.add(ofs_y.setup("ofs_y", 450, 0, HEIGHT));
	gui.add(b_GainAdjust.setup("b_GainAdjust", false));
	gui.add(AudioSample_Amp.setup("AudioSample_Amp", 0.1, 0, 0.2));
	
	/* */
	GuiGroup_GraphDispSetting.setup("GraphDisp");
	
	GuiGroup_GraphDispSetting.add(b_LineGraph.setup("b_LineGraph", false));
	
	GuiGroup_GraphDispSetting.add(BarHeight.setup("BarHeight", 180, 50, 2000));
	GuiGroup_GraphDispSetting.add(BarWidth.setup("BarWidth", 1, 1, 10));
	GuiGroup_GraphDispSetting.add(BarSpace.setup("BarSpace", 3, 0, 10));
	
	GuiGroup_GraphDispSetting.add(b_abs.setup("b_abs", true));
	GuiGroup_GraphDispSetting.add(b_AlphaBlend_Add.setup("b_AlphaBlend_Add", true));
	
	gui.add(&GuiGroup_GraphDispSetting);
	
	/* */
	GuiGroup_FilterSetting.setup("FilterSetting");
	
	GuiGroup_FilterSetting.add(CutOff_From.setup("CutOff_From", 1, 1, N/2 - 1));
	GuiGroup_FilterSetting.add(CutOff_To.setup("CutOff_To", 20, 1, N/2 - 1));
	GuiGroup_FilterSetting.add(k_Smoothing.setup("k_LPF", 0.065, 0.02, 0.1));
	GuiGroup_FilterSetting.add(NonLinear_Range.setup("NonLinear_Range", 0, 0, 0.1));
	GuiGroup_FilterSetting.add(k_Smoothing_Gain.setup("k_LPF_Gain", 0.065, 0.02, 0.1));
	
	GuiGroup_FilterSetting.add(b_phaseRotation.setup("b_phaseRotation", true));
	GuiGroup_FilterSetting.add(phase_deg.setup("phase_deg", 270, 0, 360));
	GuiGroup_FilterSetting.add(phaseRotaion_Speed.setup("Speed", 6, 0, 60));
	
	GuiGroup_FilterSetting.add(PhaseNoise_Amp.setup("PhaseNoise_Amp", 360, 0, 360));
	GuiGroup_FilterSetting.add(PhaseNoise_Speed_sec.setup("PhaseNoise_sec", 5, 0.1, 20));
	
	gui.add(&GuiGroup_FilterSetting);
	
	/* */
	ofColor initColor = ofColor(255, 255, 255, 200);
	ofColor minColor = ofColor(0, 0, 0, 0);
	ofColor maxColor = ofColor(255, 255, 255, 255);
	gui.add(BarColor.setup("BarColor", initColor, minColor, maxColor));


	/********************
	********************/
	for(int i = 0; i < fft_Gain.size(); i++)			fft_Gain[i] = 0;
	for(int i = 0; i < Last_fft_Gain.size(); i++)		Last_fft_Gain[i] = 0;
	for(int i = 0; i < AudioSample_Rev.size(); i++)		AudioSample_Rev[i] = 0;
	
	RefreshVerts();
	Refresh_BarColor();
	
	/********************
	********************/
	Vbo.setVertexData(&VboVerts[0], VboVerts.size(), GL_DYNAMIC_DRAW);
	Vbo.setColorData(&VboColor[0], VboColor.size(), GL_DYNAMIC_DRAW);
	
	/********************
	********************/
	udp_Vj.Create();
	udp_Vj.Connect("127.0.0.1", 12349/* UDP_SEND_PORT */);
	udp_Vj.SetNonBlocking(true);
	
	udp_Dmx.Create();
	udp_Dmx.Connect("127.0.0.1", 12351/* UDP_SEND_PORT */);
	udp_Dmx.SetNonBlocking(true);
}

/******************************
******************************/
void FFT::RefreshVerts()
{
	if(b_DispGain){
		for(int i = 0; i < N; i++){
			VboVerts[i * 4 + 0].set( (BarWidth + BarSpace) * i           , 0 );
			VboVerts[i * 4 + 1].set( (BarWidth + BarSpace) * i           , ofMap(fft_Gain[i], 0, AudioSample_Amp, 0, BarHeight) );
			VboVerts[i * 4 + 2].set( (BarWidth + BarSpace) * i + BarWidth, ofMap(fft_Gain[i], 0, AudioSample_Amp, 0, BarHeight) );
			VboVerts[i * 4 + 3].set( (BarWidth + BarSpace) * i + BarWidth, 0 );
		}
	}else{
		if(b_LineGraph){
			for(int i = 0; i < N; i++){
				VboVerts[i * 4 + 0].set( (BarWidth + BarSpace) * i           , 0 );
				VboVerts[i * 4 + 1].set( (BarWidth + BarSpace) * i + BarWidth, 0 );
				
				if(b_abs){
					VboVerts[i * 4 + 2].set( (BarWidth + BarSpace) * i           , abs( ofMap(AudioSample_Rev[i], -AudioSample_Amp, AudioSample_Amp, -BarHeight, BarHeight)) );
					VboVerts[i * 4 + 3].set( (BarWidth + BarSpace) * i + BarWidth, abs( ofMap(AudioSample_Rev[i], -AudioSample_Amp, AudioSample_Amp, -BarHeight, BarHeight)) );
				}else{
					VboVerts[i * 4 + 2].set( (BarWidth + BarSpace) * i           , ( ofMap(AudioSample_Rev[i], -AudioSample_Amp, AudioSample_Amp, -BarHeight, BarHeight)) );
					VboVerts[i * 4 + 3].set( (BarWidth + BarSpace) * i + BarWidth, ( ofMap(AudioSample_Rev[i], -AudioSample_Amp, AudioSample_Amp, -BarHeight, BarHeight)) );
				}
			}
		}else{
			for(int i = 0; i < N; i++){
				VboVerts[i * 4 + 0].set( (BarWidth + BarSpace) * i           , 0 );
				
				if(b_abs){
					VboVerts[i * 4 + 1].set( (BarWidth + BarSpace) * i           , abs( ofMap(AudioSample_Rev[i], -AudioSample_Amp, AudioSample_Amp, -BarHeight, BarHeight)) );
					VboVerts[i * 4 + 2].set( (BarWidth + BarSpace) * i + BarWidth, abs( ofMap(AudioSample_Rev[i], -AudioSample_Amp, AudioSample_Amp, -BarHeight, BarHeight)) );
				}else{
					VboVerts[i * 4 + 1].set( (BarWidth + BarSpace) * i           , ( ofMap(AudioSample_Rev[i], -AudioSample_Amp, AudioSample_Amp, -BarHeight, BarHeight)) );
					VboVerts[i * 4 + 2].set( (BarWidth + BarSpace) * i + BarWidth, ( ofMap(AudioSample_Rev[i], -AudioSample_Amp, AudioSample_Amp, -BarHeight, BarHeight)) );
				}
				
				VboVerts[i * 4 + 3].set( (BarWidth + BarSpace) * i + BarWidth, 0 );
			}
		}
	}
}

/******************************
******************************/
void FFT::Refresh_BarColor()
{
	ofColor color = BarColor;
	for(int i = 0; i < VboColor.size(); i++) { VboColor[i].set( double(color.r)/255, double(color.g)/255, double(color.b)/255, double(color.a)/255); }
}

/******************************
******************************/
void FFT::update()
{
	/********************
	********************/
	RefreshVerts();
	Refresh_BarColor();
	
	/********************
	以下は、audioOutからの呼び出しだと segmentation fault となってしまった.
	********************/
	Vbo.updateVertexData(&VboVerts[0], VboVerts.size());
	Vbo.updateColorData(&VboColor[0], VboColor.size());
}

/******************************
******************************/
float FFT::cal_vector_ave(const vector<float> &data, int from, int num)
{
	/********************
	********************/
	if(num == 0) return 0;
	
	/********************
	********************/
	float sum = 0;
	
	for(int i = from; i < from + num; i++){
		sum += data[i];
	}
	
	return sum / num;
}

/******************************
******************************/
void FFT::update_fftGain(const vector<float> &AudioSample)
{
	/********************
	********************/
	float now = ofGetElapsedTimef();
	
#ifdef SJ_DEBUG__MEASTIME
	fprintf(fp_Log, "%f,", now);
#endif
	
	float dt = ofClamp(now - LastInt, 0, 0.1);
	
	if(b_phaseRotation){
		phase_deg = phase_deg + phaseRotaion_Speed * dt;
		if(360 <= phase_deg) phase_deg = phase_deg - 360;
	}
	
	/********************
	********************/
	if(AudioSample.size() != N * COMPRESS) { ERROR_MSG(); ofExit(); }
	
	/********************
	********************/
	double x[N], y[N];
	
	for(int i = 0; i < N; i++){
		x[i] = cal_vector_ave(AudioSample, i * COMPRESS, COMPRESS) * fft_window[i];
		y[i] = 0;
	}
	
	fft(x, y);
	
	/********************
	********************/
	int _CutOff_From	= CutOff_From;
	int _CutOff_To		= CutOff_To;
	if(_CutOff_To < _CutOff_From){
		_CutOff_To = _CutOff_From;
		CutOff_To = _CutOff_From; // CutOff_To = CutOff_From; とすると、何故かクラッシュ
	}
	
	double phase_Noise;
	if(PhaseNoise_Speed_sec == 0)	phase_Noise = 0;
	else							phase_Noise = PhaseNoise_Amp * ofNoise(now / PhaseNoise_Speed_sec + 1.234);
	
	x[0] = 0; y[0] = 0;
	x[N/2] = 0; y[N/2] = 0;
	for(int i = 1; i < N/2; i++){
		fft_Gain[i] = sqrt(x[i] * x[i] + y[i] * y[i]);
		
		if( (_CutOff_From <= i) && (i <= _CutOff_To) ){
			x[i] = fft_Gain[i] * cos( deg2rad(phase_deg + phase_Noise) );
			y[i] = fft_Gain[i] * sin( deg2rad(phase_deg + phase_Noise) );
			
			/********************
			共役
			********************/
			x[N - i] = x[i];
			y[N - i] = -y[i];
			
		}else{
			x[i] = 0;		y[i] = 0;
			x[N - i] = 0;	y[N - i] = 0;
		}
	}
	
	fft(x, y, true); // reverse to time.
	
	
	/********************
	********************/
	/* Gain of Freq */
	for(int i = 0; i < fft_Gain.size(); i++){
		fft_Gain[i] = (1 - k_Smoothing_Gain) * Last_fft_Gain[i] + k_Smoothing_Gain * fft_Gain[i];
		Last_fft_Gain[i] = fft_Gain[i];
	}
	
	/* TimeBased */
	for(int i = 0; i < AudioSample_Rev.size(); i++){
		float _AudioSample_Rev;
		
		/* LPF */
		_AudioSample_Rev = (1 - k_Smoothing) * AudioSample_Rev[i] + k_Smoothing * x[i];
		
		/* Non Linear */
		if(NonLinear_Range == 0){
			AudioSample_Rev[i] = _AudioSample_Rev;
		}else{
			float diff =  _AudioSample_Rev - AudioSample_Rev[i];
			const double p = AudioSample_Amp * NonLinear_Range;
			if(p == 0) { ERROR_MSG(); ofExit(); return; }
			const double k = 1/p;
			if( (0 < diff) && (diff < p) ){
				diff = k * diff * diff;
			}else if( (-p < diff) && (diff < 0) ){
				diff = -k * diff * diff;
			}
			
			AudioSample_Rev[i] = AudioSample_Rev[i] + diff;
		}
	}
	
	/********************
	********************/
	LastInt = now;
	
#ifdef SJ_DEBUG__MEASTIME
	fprintf(fp_Log, "%f\n", ofGetElapsedTimef());
#endif
}

/******************************
description
	lock()/unlock()
	処理が止まってしまうので、Dialogでなく、File名固定にしようかと思ったが、
	実験の結果、Processの前後で問題なく動作したので、Dialogにしておく.
******************************/
void FFT::save_GuiSetting()
{
	/*
	gui.saveToFile("Gui.xml");
	printf("save:Gui setting\n");
	*/
	
	ofFileDialogResult res;
	res = ofSystemSaveDialog("GuiSetting.xml", "Save");
	if(res.bSuccess) gui.saveToFile(res.filePath);
}

/******************************
description
	lock()/unlock()
	処理が止まってしまうので、Dialogでなく、File名固定にしようかと思ったが、
	実験の結果、Processの前後で問題なく動作したので、Dialogにしておく.
******************************/
void FFT::load_GuiSetting()
{
	ofFileDialogResult res;
	res = ofSystemLoadDialog("Load");
	if(res.bSuccess) gui.loadFromFile(res.filePath);
}

/******************************
******************************/
double FFT::deg2rad(double deg)
{
	return deg * PI / 180;
}

/******************************
******************************/
void FFT::SendUdp()
{
	/********************
	********************/
	string message = "";
	
	for(int i = 0; i < N; i++){
		message += ofToString(AudioSample_Rev[i]) + ",";
	}
	
	message += "</p>";
	
	for(int i = 0; i < N; i++){
		message += ofToString(fft_Gain[i]) + ",";
	}
	
	/********************
	送り先のIPが不在だと、Errorとなり、関数の向こう側でError message表示し続ける.
	processが不在でも、同一PC内の場合は、IP自体は存在するのでOK.
	********************/
	if(udp_Vj.Send(message.c_str(),message.length()) == -1){
		ERROR_MSG();
		std::exit(1);
	}
	if(udp_Dmx.Send(message.c_str(),message.length()) == -1){
		ERROR_MSG();
		std::exit(1);
	}
}

/******************************
******************************/
void FFT::draw()
{
	/********************
	********************/
	SendUdp();
	
	/********************
	********************/
	ofPushStyle();
	ofPushMatrix();
		ofEnableAlphaBlending();
		if(b_AlphaBlend_Add)	ofEnableBlendMode(OF_BLENDMODE_ADD);
		else					ofEnableBlendMode(OF_BLENDMODE_ALPHA);
		
		ofTranslate(ofs_x, ofs_y);
		ofScale(1, -1, 1);
		
		/********************
		********************/
		/* */
		if(b_DispGain){
			glPointSize(1.0);
			glLineWidth(1);
			Vbo.draw(GL_QUADS, 0, VboVerts.size()/2);
		}else{
			if(b_LineGraph){
				glPointSize(1.0);
				glLineWidth(2);
				Vbo.draw(GL_LINES, 0, VboVerts.size());
			}else{
				glPointSize(1.0);
				glLineWidth(1);
				Vbo.draw(GL_QUADS, 0, VboVerts.size());
			}
		}
		
		/********************
		1. Squareを埋めるように調整
			Audio I/F Gain or AudioSample_Amp.
			
		2. Squareのsizeを調整
			BarHeight
		********************/
		if(b_GainAdjust){
			ofSetColor(0, 0, 255, 100);
			ofDrawRectangle(0, -BarHeight, (BarWidth + BarSpace) * N, BarHeight * 2);
		}
		
	ofPopMatrix();
	ofPopStyle();
	
	/********************
	********************/
	if(b_DispGui) gui.draw();
}

/******************************
******************************/
void FFT::threadedFunction()
{
	while(isThreadRunning()) {
		lock();
		
		unlock();
		
		sleep(THREAD_SLEEP_MS);
	}
}

/******************************
******************************/
int FFT::fft(double x[], double y[], int IsReverse)
{
	/*****************
		bit反転
	*****************/
	int i, j;
	for(i = 0; i < N; i++){
		j = bitrev[i];
		if(i < j){
			double t;
			t = x[i]; x[i] = x[j]; x[j] = t;
			t = y[i]; y[i] = y[j]; y[j] = t;
		}
	}

	/*****************
		変換
	*****************/
	int n4 = N / 4;
	int k, ik, h, d, k2;
	double s, c, dx, dy;
	for(k = 1; k < N; k = k2){
		h = 0;
		k2 = k + k;
		d = N / k2;

		for(j = 0; j < k; j++){
			c = sintbl[h + n4];
			if(IsReverse)	s = -sintbl[h];
			else			s = sintbl[h];

			for(i = j; i < N; i += k2){
				ik = i + k;
				dx = s * y[ik] + c * x[ik];
				dy = c * y[ik] - s * x[ik];

				x[ik] = x[i] - dx;
				x[i] += dx;

				y[ik] = y[i] - dy;
				y[i] += dy;
			}
			h += d;
		}
	}

	/*****************
	*****************/
	if(!IsReverse){
		for(i = 0; i < N; i++){
			x[i] /= N;
			y[i] /= N;
		}
	}

	return 0;
}

/******************************
******************************/
void FFT::make_bitrev(void)
{
	int i, j, k, n2;

	n2 = N / 2;
	i = j = 0;

	for(;;){
		bitrev[i] = j;
		if(++i >= N)	break;
		k = n2;
		while(k <= j)	{j -= k; k /= 2;}
		j += k;
	}
}

/******************************
******************************/
void FFT::make_sintbl(void)
{
	for(int i = 0; i < N + N/4; i++){
		sintbl[i] = sin(2 * PI * i / N);
	}
}


