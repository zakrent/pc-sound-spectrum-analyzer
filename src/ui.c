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

enum ButtonFlags {
	BF_NONE = 0,
	BF_HIGHLIGHTED = 1 << 0,
	BF_INACTIVE    = 1 << 1,
};

#define InsideRect(Rect, _x, _y) (_x > Rect.x && _x < Rect.x+Rect.w && _y > Rect.y && _y < Rect.y+Rect.h)

void Cursor(u32 Id, r32 *Value, rect Rect, uiCtx *Ctx, renderCtx *RenderCtx){
	bool Inside = InsideRect(Rect, Ctx->MouseX, Ctx->MouseY);
	if(Ctx->Active == Id){
		if(Ctx->MouseEvent == ME_LUp){
			Ctx->Active = 0;
		}
		else if(Ctx->Hot == Id){
			*Value = CLAMP((Ctx->MouseX-Rect.x)/Rect.w, 0.0f, 1.0f);
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

	//Draw cursor
	r32 Points[4];
	Points[0] = Rect.x+Rect.w*(*Value);//-1.0f+x*1.0f/(SAMP_RATE/4);
	Points[1] = Rect.y;
	Points[2] = Rect.x+Rect.w*(*Value);//-1.0f+x*1.0f/(SAMP_RATE/4);
	Points[3] = Rect.y+Rect.h;
	DrawLineStrip(Points, 2, (Ctx->Active == Id)*1.0f, 1.0, 1.0, RenderCtx);
}

bool Button(u32 Id, char *Text, rect Rect, u8 Flags, uiCtx *Ctx, memoryArena *Arena, renderCtx *RenderCtx){
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

	if(Flags & BF_HIGHLIGHTED){
		Lightness += 0.4f;
	}

	if(Ctx->Active == Id && !(Flags & BF_INACTIVE)){
		Lightness += 0.15f;
	}
	else if(Ctx->Hot == Id && !(Flags & BF_INACTIVE)){
		Lightness += 0.1f;
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

	DrawString(Text, Rect.x+0.5*Rect.w, Rect.y+0.5*Rect.h, 5.0f, 0, Arena, RenderCtx);

	if(Flags & BF_INACTIVE){
		r32 CrossPoints[8] = {
			Rect.x,        Rect.y,
			Rect.x+Rect.w, Rect.y+Rect.h,
			Rect.x+Rect.w, Rect.y,
			Rect.x       , Rect.y+Rect.h,
		};
		DrawLines(CrossPoints, 4, 1.0f, 0.5f, 0.0f, RenderCtx);
	}
	
	return RetVal && !(Flags & BF_INACTIVE);
}

