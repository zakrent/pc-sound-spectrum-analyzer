#ifndef __linux__
#define ArenaAlloc(Arena, Type) ArenaAllocRaw(Arena, sizeof(Type), _alignof(Type))
#define ArenaAllocArray(Arena, Type, n) ArenaAllocRaw(Arena, sizeof(Type)*n, _alignof(Type))
#else
#define ArenaAlloc(Arena, Type) ArenaAllocRaw(Arena, sizeof(Type), __alignof__(Type))
#define ArenaAllocArray(Arena, Type, n) ArenaAllocRaw(Arena, sizeof(Type)*n, __alignof__(Type))
#endif

typedef struct {
	u8 *Data;
	u64 Size;	
	u64 Cursor;
} memoryArena;

void* ArenaAllocRaw(memoryArena *Arena, u64 Size, u8 Alignment){
	u8 *EmptyStart = Arena->Data + Size;
	u8 *AlignedStart = EmptyStart + ((u64)EmptyStart % Alignment);
	u64 RequiredSize = Size + AlignedStart - EmptyStart;
	if(Arena->Cursor + RequiredSize < Arena->Size){
		Arena->Cursor += RequiredSize;
		return AlignedStart;
	}
	else{
		INVALID_CODE_PATH;
		return NULL;
	}
}

void ClearArena(memoryArena *Arena){
	Arena->Cursor = 0;
}
