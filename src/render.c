
typedef struct {
	u32 Shader;
	u32 TextShader;
	u32 VBO;
	u32 IBO;
	u32 Width;
	u32 Height;
} renderCtx;

enum TextFlags{
	TF_None = 0,
	TF_AlignLeft  = 1<<1,
	TF_AlignRight = 1<<2,
};

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

char *TextVertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec2 Pos;\n"
"uniform vec2 WindowSize;\n"
"uniform float Scale;\n"
"void main(){\n"
"	float AR = WindowSize.x/WindowSize.y;\n"
"	float ScaleNorm = Scale/WindowSize.x;\n"
"	gl_Position = vec4(Pos.x*ScaleNorm, -Pos.y*ScaleNorm*AR, 0.0, 1.0f);\n"
"}\0";

char *TextFragmentShaderSource = "#version 330 core\n"
"out vec4 OutColor;\n"
"uniform vec3 Color;\n"
"void main(){\n"
"	OutColor = vec4(Color.r, Color.g, Color.b, 1.0f);\n"
"}\0";

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

renderCtx InitRendering(){
	renderCtx Ctx;
	Ctx.Shader = LoadShader(VertexShaderSource, FragmentShaderSource);
	Ctx.TextShader = LoadShader(TextVertexShaderSource, TextFragmentShaderSource);
	glGenBuffers(1, &Ctx.VBO);
	glGenBuffers(1, &Ctx.IBO);

	stb_easy_font_spacing(-1.0f);
	return Ctx;
}

void Resize(u32 x, u32 y, u32 Width, u32 Height, renderCtx *RenderCtx){
	RenderCtx->Height = Height;
	RenderCtx->Width = Width;
	glViewport(x, y, Width, Height);
}

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

void DrawTriangles(r32 *Points, u32 NPoints, r32 r, r32 g, r32 b, renderCtx *RenderCtx){
	glBindBuffer(GL_ARRAY_BUFFER, RenderCtx->VBO);
	glBufferData(GL_ARRAY_BUFFER, NPoints*sizeof(r32)*2, Points, GL_STREAM_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glUseProgram(RenderCtx->Shader);

	u32 ColorUniform = glGetUniformLocation(RenderCtx->Shader, "Color");
	glUniform3f(ColorUniform, r, g, b);

	glDrawArrays(GL_TRIANGLES, 0, NPoints);
}

void DrawString(char *Text, r32 xC, r32 yC, r32 Scale, u8 Flags, memoryArena *Arena, renderCtx *RenderCtx){
	i32 Size = strlen(Text);
	i32 BufferSize = 1000*Size;
	r32 *Points = ArenaAllocArray(Arena, r32, BufferSize);

	r32 x = xC/Scale*RenderCtx->Width+0.5f;
	r32 y = -yC/Scale*RenderCtx->Height-stb_easy_font_height(Text)*0.5f+2.5f;

	if(Flags & TF_AlignRight){
		x -= stb_easy_font_width(Text)*1.0f;
	}
	else if(!(Flags & TF_AlignLeft)){
		x -= stb_easy_font_width(Text)*0.5f;
	}

	i32 NumQuads = stb_easy_font_print(x, y, Text, NULL, Points, BufferSize);

	u32 *Indexes = ArenaAllocArray(Arena, r32, NumQuads*6);

	for(int i = 0; i < NumQuads; i++){
		Indexes[i*6]   = i*4;
		Indexes[i*6+1] = i*4+1;
		Indexes[i*6+2] = i*4+2;

		Indexes[i*6+3] = i*4;
		Indexes[i*6+4] = i*4+3;
		Indexes[i*6+5] = i*4+2;
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, RenderCtx->IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, NumQuads*6*sizeof(u32), Indexes, GL_STREAM_DRAW);
	
	glBindBuffer(GL_ARRAY_BUFFER, RenderCtx->VBO);
	glBufferData(GL_ARRAY_BUFFER, NumQuads*4*4*sizeof(r32), Points, GL_STREAM_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(r32), 0);
	glEnableVertexAttribArray(0);

	glUseProgram(RenderCtx->TextShader);

	i32 ScaleUniform = glGetUniformLocation(RenderCtx->TextShader, "Scale");
	glUniform1f(ScaleUniform, Scale);

	i32 ColorUniform = glGetUniformLocation(RenderCtx->TextShader, "Color");
	glUniform3f(ColorUniform, 1.0f*0.8f, 0.5f*0.8f, 0.0f);

	i32 WindowSizeUniform = glGetUniformLocation(RenderCtx->TextShader, "WindowSize");
	glUniform2f(WindowSizeUniform, RenderCtx->Width, RenderCtx->Height);

	glDrawElements(GL_TRIANGLES, NumQuads*6, GL_UNSIGNED_INT, 0);
}
