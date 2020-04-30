#define _USE_MATH_DEFINES
#include <math.h>

char *VertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec2 Pos;\n"
"void main(){\n"
"	gl_Position = vec4(Pos.x, Pos.y, 0.0, 1.0f);\n"
"}\0";

char *FragmentShaderSource = "#version 330 core\n"
"out vec4 OutColor;\n"
"uniform vec3 Color;\n"
"void main(){\n"
"	OutColor = vec4(Color.r, Color.g, Color.b, 1.0f);\n"
"}\0";

//TODO: redefine as variables
#define ReadSize (2048)
#define FftSize ReadSize/2+1

typedef struct {
	u32 Shader;
	u32 VBO;
} renderCtx;

typedef struct {
	u64         MemorySize;
	renderCtx   RenderCtx;
	bool        Initialized;
	u32         PrevReads;
	memoryArena TempArena;

	bool DecayReset;

	r32 Buffer[SAMP_RATE];

	r32 FftAvgMagDb[FftSize];
	r32 FftDecayMagDb[FftSize];
	r32 FftDecayTime[FftSize];
} program;

void DrawLineStrip(r32 *Points, u32 NPoints, r32 r, r32 g, r32 b, renderCtx *RenderCtx){
	glBindBuffer(GL_ARRAY_BUFFER, RenderCtx->VBO);
	glBufferData(GL_ARRAY_BUFFER, NPoints*sizeof(r32)*2, Points, GL_STREAM_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glUseProgram(RenderCtx->Shader);

	u32 ColorUniform = glGetUniformLocation(RenderCtx->Shader, "Color");
	glUniform3f(ColorUniform, r, g, b);

	glDrawArrays(GL_LINE_STRIP, 0, NPoints);
}

void DrawLines(r32 *Points, u32 NPoints, r32 r, r32 g, r32 b, renderCtx *RenderCtx){
	glBindBuffer(GL_ARRAY_BUFFER, RenderCtx->VBO);
	glBufferData(GL_ARRAY_BUFFER, NPoints*sizeof(r32)*2, Points, GL_STREAM_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glUseProgram(RenderCtx->Shader);

	u32 ColorUniform = glGetUniformLocation(RenderCtx->Shader, "Color");
	glUniform3f(ColorUniform, r, g, b);

	glDrawArrays(GL_LINES, 0, NPoints);
}

/*
void DrawOscilloscope(i16 *Buffer, u32 Count, renderCtx *RenderCtx, memoryArena *Arena){
	r32 *Points = ArenaAllocArray(Arena, r32, 2*Count);
	for(int i = 0; i < Count; i++){
		Points[2*i] = -1.0+2.0/(Count-1)*i;
		Points[2*i+1] = -Buffer[i]*1.0/INT16_MIN;
	}
	DrawLineStrip(Points, Count, 0.0, 1.0, 0.0, RenderCtx);
}
*/

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

u32 LoadShader(char *VertexShaderSource, char *FragmentShaderSource){
	u32 VertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(VertexShader, 1, &VertexShaderSource, 0);
	glCompileShader(VertexShader);

	char Log[512];
	i32 status;
	glGetShaderiv(VertexShader, GL_COMPILE_STATUS, &status);
	if(!status){
		glGetShaderInfoLog(VertexShader, 512, NULL, Log);
		OutputDebugStringA(Log);
	}

	u32 FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(FragmentShader, 1, &FragmentShaderSource, 0);
	glCompileShader(FragmentShader);
	glGetShaderiv(FragmentShader, GL_COMPILE_STATUS, &status);
	if(!status){
		glGetShaderInfoLog(VertexShader, 512, NULL, Log);
		OutputDebugStringA(Log);
	}

	u32 Shader = glCreateProgram();
	glAttachShader(Shader, VertexShader);
	glAttachShader(Shader, FragmentShader);
	glLinkProgram(Shader);
		
	glGetProgramiv(Shader, GL_LINK_STATUS, &status);
	if(!status) {
		glGetProgramInfoLog(Shader, 512, NULL, Log);
		OutputDebugStringA(Log);
	}
	
	glDeleteShader(VertexShader);
	glDeleteShader(FragmentShader);  

	return Shader;
}

static program *Program;
void Frame(void *Memory, u64 MemorySize, u32 WindowWidth, u32 WindowHeight){
	ASSERT(sizeof(Program) > MemorySize);

	Program = Memory;

	if(!Program->Initialized){
		Program->TempArena = (memoryArena){
			.Data = (u8*)Memory + sizeof(program),
			.Size = MemorySize - sizeof(program),
		};

		Program->RenderCtx.Shader = LoadShader(VertexShaderSource, FragmentShaderSource);
		glGenBuffers(1, &Program->RenderCtx.VBO);
		Program->Initialized = true;
	}

	glViewport(0, 0, WindowWidth, WindowHeight);

	//Get size of memory needed for fft
	u64 FftMemNeeded = 1;
	kiss_fftr_alloc(ReadSize, 0, 0, &FftMemNeeded);

	//Alocate and initialize fft memory
	i16 *FftMemory = ArenaAllocArray(&Program->TempArena, u8, FftMemNeeded);

	kiss_fftr_cfg FftCfg = kiss_fftr_alloc(ReadSize, 0, FftMemory, &FftMemNeeded);
	r32C *FftOut = ArenaAllocArray(&Program->TempArena, r32C, FftSize);
	r32  *FftIn  = ArenaAllocArray(&Program->TempArena, r32, ReadSize);

	//Temporary buffer for capture buffer data
	i16 *TempBuffer = ArenaAllocArray(&Program->TempArena, i16, ReadSize);
	int Reads = 0;	
	while(ReadCaptureBuffer(TempBuffer, ReadSize*sizeof(i16))){
		//Convert to r32
		for(int i = 0; i < ReadSize; i++){
			FftIn[i] = -TempBuffer[i]*1.0/INT16_MIN;
			FftIn[i] *= 0.5*(1-cos(2*M_PI*i/ReadSize));
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
		Program->DecayReset = 1;
	}

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	DrawSpectrum(Program->FftAvgMagDb, Program->FftDecayMagDb, FftSize, &Program->RenderCtx, &Program->TempArena);

	ClearArena(&Program->TempArena);
}
