#define _USE_MATH_DEFINES
#include <math.h>

//TODO: remove this
#define MaxFftSize SAMP_RATE

typedef struct {
	u64         MemorySize;
	bool        Initialized;

	memoryArena TempArena;

	renderCtx   RenderCtx;
	uiCtx   	UiCtx;

	u32 ReadSize;
	u32 PrevReads;

	bool DecayReset;

	r32 Buffer[SAMP_RATE];

	r32 FftAvgMagDb[MaxFftSize];
	r32 FftDecayMagDb[MaxFftSize];
	r32 FftDecayTime[MaxFftSize];
} program;

void DrawSpectrum(r32 *FftMagDb, r32 *FftDecayDb, u32 Count, renderCtx *RenderCtx, memoryArena *Arena){
	r32 *Points = ArenaAllocArray(Arena, r32, MAX(2*Count, 4*SAMP_RATE/2000));

	static const r32 DbScale = 16.0f;

	//Draw sustain and decay graph:
	for(int i = 0; i < Count; i++){
		Points[2*i] = -1.0f+2.0f/(Count-1)*i;
		Points[2*i+1] = 1.0f+2.0f*FftDecayDb[i]/16.0f;
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

	for(int i = 0; i < DbScale; i++){
		Points[4*i]   = -1.0f;
		Points[4*i+1] = -1.0+i*2.0f/(DbScale);
		Points[4*i+2] = 1.0;
		Points[4*i+3] = -1.0+i*2.0/(DbScale);
	}
	DrawLines(Points, 2*DbScale, 0.2, 0.2, 0.2, RenderCtx);
}

#define GUI_WIDTH 300.0f
void UpdateGUI(program *Program){
	r32 SF = 2.0f*GUI_WIDTH/Program->RenderCtx.Width;
	r32 AR = Program->RenderCtx.Width*1.0f/Program->RenderCtx.Height;

	DrawString("FFT Size:", 1.0f-0.5f*SF, 0.2f*SF*AR, 5.0f,&Program->TempArena, &Program->RenderCtx);
	char FFTSizeString[32] = {0};
	snprintf(FFTSizeString, 32, "%u", Program->ReadSize);
	DrawString(FFTSizeString, 1.0f-0.5f*SF, 0.15f*0.5f*SF*AR, 5.0f,&Program->TempArena, &Program->RenderCtx);

	if(Button(1, "+", (rect){1.0f-0.25*SF, 0.0f, 0.15*SF, 0.15*SF*AR}, &Program->UiCtx, &Program->TempArena, &Program->RenderCtx)){
		Program->ReadSize *= 2;
		Program->DecayReset = 0;
	}

	if(Button(2, "-", (rect){1.0f-0.9*SF, 0.0f, 0.15*SF, 0.15*SF*AR}, &Program->UiCtx, &Program->TempArena, &Program->RenderCtx)){
		Program->ReadSize /= 2;
		Program->DecayReset = 0;
	}

}

static program *Program;
void Frame(void *Memory, u64 MemorySize, u32 WindowWidth, u32 WindowHeight, r32 MouseX, r32 MouseY, u32 MouseEvent){
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

	//Temporary buffer for capture buffer data
	i16 *TempBuffer = ArenaAllocArray(&Program->TempArena, i16, Program->ReadSize);
	int Reads = 0;	
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
				Program->FftAvgMagDb[i] /= FftSize;
			}
		}
		else{
			for(int i = 0; i < FftSize; i++){
				Program->FftAvgMagDb[i] += sqrt(FftOut[i].r*FftOut[i].r+FftOut[i].i*FftOut[i].i);
				//TODO: BUG?!?!?!?!??!?!
				//TODO: move to later stage?
				Program->FftAvgMagDb[i] /= FftSize;
			}
		}

		Reads++;
	}

	//Calculate average of magnitudes from sum of them
	if(Reads > 1){
		for(int i = 0; i < FftSize; i++){
			Program->FftAvgMagDb[i] /= Reads;
		}
	}

	//Convert to dBs and handle sustain and decay graph
	if(Reads != 0){
		for(int i = 0; i < FftSize; i++){
			Program->FftAvgMagDb[i] = logf(Program->FftAvgMagDb[i]);
		}
	}

	for(int i = 0; i < FftSize; i++){
		if(Program->FftDecayMagDb[i] < Program->FftAvgMagDb[i] || !Program->DecayReset){
			Program->FftDecayMagDb[i] = Program->FftAvgMagDb[i];
			Program->FftDecayTime[i] = 0.1;
		}
		else{
			if(Program->FftDecayTime[i] < 0.0){
				Program->FftDecayMagDb[i] -= 0.1;
			}
			else{
				Program->FftDecayTime[i] -= FRAME_TIME;
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
}
