#define _USE_MATH_DEFINES
#include <math.h>

//TODO: remove this
#define MaxReadSize SAMP_RATE/4

typedef struct {
	u64         MemorySize;
	bool        Initialized;
	bool        ShouldExit;
	bool        Paused;
	bool        MaxHold;

	memoryArena TempArena;

	renderCtx   RenderCtx;
	uiCtx   	UiCtx;

	r32 CursorVal;

	u32 ReadSize;
	u32 PrevReads;

	bool DecayReset;

	r32 Buffer[SAMP_RATE];

	r32 FftAvgMagDb[MaxReadSize/2+1];
	r32 FftDecayMagDb[MaxReadSize/2+1];
	r32 FftDecayTime[MaxReadSize/2+1];
} program;

void DrawSpectrum(r32 *FftMagDb, r32 *FftDecayDb, u32 Count, renderCtx *RenderCtx, memoryArena *Arena){
	r32 *Points = ArenaAllocArray(Arena, r32, MAX(2*Count, 4*SAMP_RATE/2000));

	static const r32 DbScale = 160.0f;

	//Draw Axis labels
	u32 DrawWidth = RenderCtx->Width;
	u32 DrawHeight = RenderCtx->Height;
#define LABEL_HEIGHT 20
#define LABEL_WIDTH  60
#define LABEL_HEIGHT_N LABEL_HEIGHT*1.0f/RenderCtx->Height
#define LABEL_WIDTH_N  LABEL_WIDTH*1.0f/RenderCtx->Width

	char Label[16];
	u8 FontSize = (DrawWidth > 1000 && DrawHeight > 600) ? 3 : 2;
	for (int i = 1; i < SAMP_RATE / 2000; i++) {
		snprintf(Label, 8, "%ikHz", i);
		DrawString(Label, -1.0f+2.0f*i/(SAMP_RATE / 2000)*(1.0f-LABEL_WIDTH_N)+LABEL_WIDTH_N*2.0f, -1.0f + LABEL_HEIGHT_N, FontSize, 0, Arena, RenderCtx);
	}

	for (int i = 0; i < DbScale/10; i++) {
		snprintf(Label, 8, "%idBFS", (int)-(DbScale-i*10));
		DrawString(Label, -1.0f+LABEL_WIDTH*1.0f/DrawWidth, -1.0f+i*20.0f/DbScale*(1.0f-LABEL_HEIGHT_N)+LABEL_HEIGHT_N*2.0f, FontSize, 0, Arena, RenderCtx);
	}

	Resize(LABEL_WIDTH, LABEL_HEIGHT, DrawWidth-LABEL_WIDTH, DrawHeight-LABEL_HEIGHT, RenderCtx);

	//Draw sustain and decay graph:
	for(int i = 0; i < Count; i++){
		Points[2*i] = -1.0f+2.0f/(Count-1)*i;
		Points[2*i+1] = 1.0f+2.0f*FftDecayDb[i]/DbScale;
	}
	DrawLineStrip(Points, Count, 1.0, 0.0, 0.0, RenderCtx);

	//Draw fft magnitude graph:
	for(int i = 0; i < Count; i++){
		Points[2*i] = -1.0f+2.0f/(Count-1)*i;
		Points[2*i+1] = 1.0f+2.0f*FftMagDb[i]/DbScale;
	}
	DrawLineStrip(Points, Count, 0.0, 1.0, 0.0, RenderCtx);
	
	//Draw grid:
	for(int i = 0; i < SAMP_RATE/2000; i++){
		Points[4*i] =   -1.0+i*1.0f/(SAMP_RATE/4000);
		Points[4*i+1] = 1.0f;
		Points[4*i+2] = -1.0+i*1.0f/(SAMP_RATE/4000);
		Points[4*i+3] = -1.0f;
	}
	DrawLines(Points, 2*SAMP_RATE/2000, 0.2, 0.2, 0.2, RenderCtx);

	for(int i = 0; i < DbScale/10; i++){
		Points[4*i]   = -1.0f;
		Points[4*i+1] = -1.0+i*20.0f/(DbScale);
		Points[4*i+2] = 1.0;
		Points[4*i+3] = -1.0+i*20.0/(DbScale);
	}
	DrawLines(Points, 2*DbScale/10, 0.2, 0.2, 0.2, RenderCtx);

}

#define GUI_WIDTH 300.0f
void UpdateGUI(program *Program){
	r32 SF = 2.0f*GUI_WIDTH/Program->RenderCtx.Width;
	r32 AR = Program->RenderCtx.Width*1.0f/Program->RenderCtx.Height;

	DrawString("Samples:", 1.0f-0.5f*SF, 0.2f*SF*AR, 5.0f, 0,&Program->TempArena, &Program->RenderCtx);

	char Buffer[32] = {0};

	snprintf(Buffer, 32, "%u", Program->ReadSize);
	DrawString(Buffer, 1.0f-0.5f*SF, 0.15f*0.5f*SF*AR, 5.0f, 0,&Program->TempArena, &Program->RenderCtx);

	DrawString("Cursor:",         1.0f-0.5f*SF, 1.0f-0.1f*SF*AR, 5.0f, 0           , &Program->TempArena, &Program->RenderCtx);
	DrawString("Frequency:",      1.0f-0.9f*SF, 1.0f-0.2f*SF*AR, 4.0f, TF_AlignLeft, &Program->TempArena, &Program->RenderCtx);
	DrawString("Magnitude:",      1.0f-0.9f*SF, 1.0f-0.3f*SF*AR, 4.0f, TF_AlignLeft, &Program->TempArena, &Program->RenderCtx);
	DrawString("Hold Magnitude:", 1.0f-0.9f*SF, 1.0f-0.4f*SF*AR, 4.0f, TF_AlignLeft, &Program->TempArena, &Program->RenderCtx);

	snprintf(Buffer, 32, "%8.2f Hz", Program->CursorVal*SAMP_RATE/2);
	DrawString(Buffer, 1.0f-0.05f*SF, 1.0f-0.2f*SF*AR, 4.0f, TF_AlignRight, &Program->TempArena, &Program->RenderCtx);

	r32 CursorMagnitude = Program->FftAvgMagDb[(int)(Program->CursorVal*Program->ReadSize/2)];
	snprintf(Buffer, 32, "%8.2f dBFS", CursorMagnitude);
	DrawString(Buffer, 1.0f-0.05f*SF, 1.0f-0.3f*SF*AR, 4.0f, TF_AlignRight, &Program->TempArena, &Program->RenderCtx);

	r32 CursorHoldMagnitude = Program->FftDecayMagDb[(int)(Program->CursorVal*Program->ReadSize/2)];
	snprintf(Buffer, 32, "%8.2f dBFS", CursorHoldMagnitude);
	DrawString(Buffer, 1.0f-0.05f*SF, 1.0f-0.4f*SF*AR, 4.0f, TF_AlignRight, &Program->TempArena, &Program->RenderCtx);

	u8 FFTUpButtonFlags   = Program->Paused || (Program->ReadSize*2 > MaxReadSize) ? BF_INACTIVE : 0;
	u8 FFTDownButtonFlags = Program->Paused || (Program->ReadSize/2 <= 16) ? BF_INACTIVE : 0;
	if(Button(1, "+", (rect){1.0f-0.25*SF, 0.0f, 0.15*SF, 0.15*SF*AR}, FFTUpButtonFlags, &Program->UiCtx, &Program->TempArena, &Program->RenderCtx)){
		Program->ReadSize *= 2;
		Program->DecayReset = 0;
	}

	if(Button(2, "-", (rect){1.0f-0.9*SF, 0.0f, 0.15*SF, 0.15*SF*AR}, FFTDownButtonFlags, &Program->UiCtx, &Program->TempArena, &Program->RenderCtx)){
		Program->ReadSize /= 2;
		Program->DecayReset = 0;
	}

	u8 MaxHoldButtonFlags = Program->MaxHold ? BF_HIGHLIGHTED : 0;	
	if(Button(4, "MAX HOLD", (rect){1.0f-0.9f*SF, -1.0f+0.5f*SF*AR, 0.8*SF, 0.15*SF*AR}, MaxHoldButtonFlags, &Program->UiCtx, &Program->TempArena, &Program->RenderCtx)){
		Program->MaxHold = !Program->MaxHold;
	}

	u8 PauseButtonFlags = Program->Paused ? BF_HIGHLIGHTED : 0;	
	if(Button(5, "PAUSE", (rect){1.0f-0.9f*SF, -1.0f+0.3f*SF*AR, 0.8*SF, 0.15*SF*AR}, PauseButtonFlags, &Program->UiCtx, &Program->TempArena, &Program->RenderCtx)){
		Program->Paused = !Program->Paused;
	}

	if(Button(6, "EXIT", (rect){1.0f-0.9f*SF, -1.0f+0.1f*SF*AR, 0.8*SF, 0.15*SF*AR}, NULL, &Program->UiCtx, &Program->TempArena, &Program->RenderCtx)){
		Program->ShouldExit = true;
	}

	renderCtx *RenderCtx = &Program->RenderCtx;

	Cursor(7, &Program->CursorVal, (rect){-1.0f+LABEL_WIDTH_N*2.0f, -1.0f+LABEL_HEIGHT_N*2.0f, 2.0f-SF-LABEL_WIDTH_N*2.0f, 2.0f-LABEL_HEIGHT_N*2.0f}, &Program->UiCtx, &Program->RenderCtx);
	r32 LM = 2.0f*LABEL_WIDTH*1.0f/Program->RenderCtx.Width;
	//Program->CursorFreq = MIN(MAX((Program->UiCtx.MouseX+1.0f-LM)/(1.0f-0.5f*SF-0.5f*LM)*SAMP_RATE/4, 1.0f), SAMP_RATE/2);
}

static program *Program;
bool Frame(void *Memory, u64 MemorySize, u32 WindowWidth, u32 WindowHeight, r32 MouseX, r32 MouseY, u32 MouseEvent){
	ASSERT(sizeof(Program) > MemorySize);

	Program = Memory;

	if(!Program->Initialized){
		Program->TempArena = (memoryArena){
			.Data = (u8*)Memory + sizeof(program),
			.Size = MemorySize - sizeof(program),
		};

		Program->RenderCtx = InitRendering();

		Program->ReadSize = 2048;
		Program->Initialized = true;
	}

	Program->UiCtx.MouseX = MouseX;
	Program->UiCtx.MouseY = MouseY;
	Program->UiCtx.MouseEvent = MouseEvent;

	u32 FftSize = Program->ReadSize/2+1;	

	Resize(0, 0, WindowWidth-GUI_WIDTH, WindowHeight, &Program->RenderCtx);

	//Get size of memory needed for fft
	u64 FftMemNeeded = 1;
	kiss_fftr_alloc(Program->ReadSize, 0, 0, &FftMemNeeded);

	//Alocate and initialize fft memory
	i16 *FftMemory = ArenaAllocArray(&Program->TempArena, u8, FftMemNeeded);

	kiss_fftr_cfg FftCfg = kiss_fftr_alloc(Program->ReadSize, 0, FftMemory, &FftMemNeeded);
	r32C *FftOut = ArenaAllocArray(&Program->TempArena, r32C, FftSize);
	r32  *FftIn  = ArenaAllocArray(&Program->TempArena, r32, Program->ReadSize);

	int Reads = 0;	
	if(!Program->Paused){
		//Temporary buffer for capture buffer data
		i16 *TempBuffer = ArenaAllocArray(&Program->TempArena, i16, Program->ReadSize);
		while(ReadCaptureBuffer(TempBuffer, Program->ReadSize*sizeof(i16))){
			//Convert to r32
			for(int i = 0; i < Program->ReadSize; i++){
				FftIn[i] = -TempBuffer[i]*1.0/INT16_MIN;
				FftIn[i] *= 0.5*(1-cos(2*M_PI*i/Program->ReadSize));
			}

			//Calculate fft
			kiss_fftr(FftCfg, FftIn, (kiss_fft_cpx*)FftOut);

			//Add magnitude of all fft results together
			if(Reads == 0){
				for(int i = 0; i < FftSize; i++){
					Program->FftAvgMagDb[i] = sqrt(FftOut[i].r*FftOut[i].r+FftOut[i].i*FftOut[i].i);
				}
			}
			else{
				for(int i = 0; i < FftSize; i++){
					Program->FftAvgMagDb[i] += sqrt(FftOut[i].r*FftOut[i].r+FftOut[i].i*FftOut[i].i);
				}
			}

			Reads++;
		}
	}

	//Calculate average of magnitudes from sum of them
	if(Reads > 0){
		for(int i = 0; i < FftSize; i++){
			Program->FftAvgMagDb[i] /= Reads*FftSize*0.5f;
		}
	}

	//Convert to dBs
	if(Reads != 0){
		for(int i = 0; i < FftSize; i++){
			Program->FftAvgMagDb[i] = 20.0f*log10f(Program->FftAvgMagDb[i]);
		}
	}

	//Handle sustain and decay
	if(!Program->Paused){
		for(int i = 0; i < FftSize; i++){
			if(Program->FftDecayMagDb[i] < Program->FftAvgMagDb[i] || !Program->DecayReset){
				Program->FftDecayMagDb[i] = Program->FftAvgMagDb[i];
				Program->FftDecayTime[i] = 0.1;
			}
			else{
				if(!Program->MaxHold){
					if(Program->FftDecayTime[i] < 0.0){
						Program->FftDecayMagDb[i] -= 2.0f;
					}
					else{
						Program->FftDecayTime[i] -= FRAME_TIME;
					}
				}
			}
		}
	}

	if(Reads != 0){
		Program->DecayReset = 1;
	}

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	DrawSpectrum(Program->FftAvgMagDb, Program->FftDecayMagDb, FftSize, &Program->RenderCtx, &Program->TempArena);

	Resize(0, 0, WindowWidth, WindowHeight, &Program->RenderCtx);

	UpdateGUI(Program);

	ClearArena(&Program->TempArena);
	return Program->ShouldExit;
}
