typedef struct {
	r32 MouseX;
	r32 MouseY;
	u32 MouseEvent;
	
	u32 Hot;
	u32 Active;
} uiCtx;

typedef struct {
	r32 x;
	r32 y;
	r32 w;
	r32 h;
} rect;

#define InsideRect(Rect, _x, _y) (_x > Rect.x && _x < Rect.x+Rect.w && _y < Rect.y && _y > Rect.y-Rect.h)

bool Button(u32 Id, char *Text, rect Rect, uiCtx *Ctx, memoryArena *Arena, renderCtx *RenderCtx){
	bool Inside = InsideRect(Rect, Ctx->MouseX, Ctx->MouseY);
	bool RetVal = false;
	if(Ctx->Active == Id){
		if(Ctx->MouseEvent == ME_LUp){
			Ctx->Active = 0;
			RetVal = Ctx->Hot == Id;
		}
	}
	if(Ctx->Hot == Id){
		if(!Inside){
			Ctx->Hot = 0;
		}
		else if(Ctx->MouseEvent == ME_LDown){
			Ctx->Active = Id;
		}
	}
	else if(Inside && (Ctx->Active == 0 || Ctx->Active == Id)){
		Ctx->Hot = Id;
	}
	
//	r32 *Points = ArenaAllocArray(Arena, r32, MAX(2*Count, 4*SAMP_RATE/2000));
#if 0
	r32 Points[12] = {
		Rect.x,        Rect.y,
		Rect.x+Rect.w, Rect.y,
		Rect.x,        Rect.y+Rect.h,
		Rect.x,        Rect.y+Rect.h,
		Rect.x+Rect.w, Rect.y,
		Rect.x+Rect.w, Rect.y+Rect.h,
	};
	DrawTriangles(Points, 12, 1.0f, Ctx->Hot == Id * 1.0f, Ctx->Active == Id * 1.0f, RenderCtx);
#endif
	r32 Lightness = 0.1f;
	if(Ctx->Active == Id){
		Lightness = 0.25f;
	}
	else if(Ctx->Hot == Id){
		Lightness = 0.2f;
	}

	r32 RectPoints[12] = {
		Rect.x,        Rect.y,
		Rect.x+Rect.w, Rect.y,
		Rect.x,        Rect.y+Rect.h,
		Rect.x,        Rect.y+Rect.h,
		Rect.x+Rect.w, Rect.y,
		Rect.x+Rect.w, Rect.y+Rect.h,
	};
	DrawTriangles(RectPoints, 12, Lightness*1.0f, Lightness*0.5f, 0.0f, RenderCtx);

	r32 Points[10] = {
		Rect.x,        Rect.y,
		Rect.x+Rect.w, Rect.y,
		Rect.x+Rect.w, Rect.y+Rect.h,
		Rect.x,        Rect.y+Rect.h,
		Rect.x,        Rect.y,
	};
	DrawLineStrip(Points, 5, 1.0f, 0.5f, 0.0f, RenderCtx);

	DrawString(Text, Rect.x+0.5*Rect.w, Rect.y+0.5*Rect.h, 5.0f, Arena, RenderCtx);
	
	return RetVal;
}

