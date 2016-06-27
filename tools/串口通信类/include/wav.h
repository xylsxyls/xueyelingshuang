#pragma once

#include <mmsystem.h>
//#include "g711.h"
//#define WAVE_FORMAT_PCM 0x01
#define WAVE_FORMAT_A_LAW 0x06
#define WAVE_FORMAT_U_LAW 0x07
#define WAVE_FORMAT_MU_LAW 0x07

typedef   struct
{ 
char         fccID[4];       /*   should   be   "RIFF"   */ 
unsigned   long       dwSize;           /*   byte_number   behind   it   */ 
char         fccType[4];   /*   should   be   "WAVE"   */ 
}HEADER;

typedef   struct
{ 
char         fccID[4];       /*   should   be   "fmt "   */ 
unsigned   long       dwSize;           /*   should   be   0x10   */ 
unsigned   short     wFormatTag;   /*   should   be   1   */ 
unsigned   short     wChannels; 
unsigned   long       dwSamplesPerSec; 
unsigned   long       dwAvgBytesPerSec; 
unsigned   short     wBlockAlign; 
unsigned   short     uiBitsPerSample; 
}FMT; 

typedef struct 
{
	unsigned char	szFactID[4]; // 'f','a','c','t'
	unsigned long	dwFactSize;
}FACT;

typedef   struct
{ 
char         fccID[4];         /*   should   be   "data"   */ 
unsigned   long       dwSize;             /*   byte_number   of   PCM   data   in   byte*/ 
}DATA; 

enum wavError
{
	WAV_SUCCESS = 9000,
	WAV_IN_FILE_ERROR = 9001,
	WAV_OUT_FILE_ERROR = 9002,
	PCM_IN_FILE_ERROR = 9003,
	PCM_OUT_FILE_ERROR = 9004,
	WAV_FORMAT_ERROR = 9005,
	WAV_FLAG_HEADER_ERROR = 9006,
	WAV_FLAG_FMT_ERROR = 9007,
	WAV_FLAG_DATA_ERROR = 9008,
	WAV_FLAG_FACT_ERROR = 9009,
};

class wav
{
public:
	wav(void);
	int pcm2wav(const char * in , const char * out);
//	int wav2pcm(const char * in , const char * out);
//	int wav2a(const char * in , const char * out);
//	int a2wav(const char * in , const char * out);
//	int wav2u(const char * in , const char * out);
//	int u2wav(const char * in , const char * out);
//	int a2u(const char * in , const char * out);
//	int u2a(const char * in , const char * out);
	int makeHead(unsigned short wFormatTag = WAVE_FORMAT_PCM,unsigned long dwSamplesPerSec = 16000
		,unsigned short uiBitsPerSample = 16,unsigned short wChannels = 1
		,unsigned short wBlockAlign = 2);
//	int getHead(FILE *fp );
//	int getHead(const char * in,unsigned short &wFormatTag,unsigned long &dwSamplesPerSec
//		,unsigned short &uiBitsPerSample,unsigned short &wChannels 
//		,unsigned short &wBlockAlign);
		
public:
	~wav(void);
private:
	HEADER	pcmHEADER; 
	FMT		pcmFMT; 
	//FACT	pcmFACT;
	DATA	pcmDATA; 
	//g711	G711;
	//char   fact_block_buffer[20];		//20 should be enough
};
