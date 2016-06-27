#include "StdAfx.h"
#include "wav.h"

wav::wav(void)
{
/*	strcpy(fact_block_buffer,"0123456789ABCDEF000");
	fact_block_buffer[0]=0x20;
	fact_block_buffer[1]=0x8a;
	fact_block_buffer[2]=0x00;
	fact_block_buffer[3]=0x00;
	pcmFACT.dwFactSize = 4;
	pcmFACT.szFactID[0] = 'f';
	pcmFACT.szFactID[1] = 'a';
	pcmFACT.szFactID[2] = 'c';
	pcmFACT.szFactID[3] = 't';
	*/
}

wav::~wav(void)
{
}
int wav::makeHead(unsigned short wFormatTag ,unsigned long dwSamplesPerSec ,
		unsigned short uiBitsPerSample ,unsigned short wChannels , 
		unsigned short wBlockAlign )
{
	strcpy(pcmHEADER.fccID, "RIFF");                   
	strcpy(pcmHEADER.fccType, "WAVE"); 
	strcpy(pcmFMT.fccID, "fmt "); 
	strcpy(pcmDATA.fccID, "data"); 

	pcmFMT.dwSamplesPerSec	=	dwSamplesPerSec;
	pcmFMT.uiBitsPerSample	=	uiBitsPerSample;
	pcmFMT.wChannels		=	wChannels;
	pcmFMT.wFormatTag		=	wFormatTag;
	pcmFMT.wBlockAlign		=	wBlockAlign; 
	if(wFormatTag == WAVE_FORMAT_PCM)
	{
		pcmFMT.dwAvgBytesPerSec	=	pcmFMT.dwSamplesPerSec*sizeof(unsigned short);
		pcmFMT.dwSize			=	0x10; 
	}
	else if(wFormatTag == WAVE_FORMAT_A_LAW)
	{
		pcmFMT.dwAvgBytesPerSec=pcmFMT.dwSamplesPerSec;//*sizeof(unsigned char);
		pcmFMT.dwSize			=	0x12; 
	}
	else if(wFormatTag == WAVE_FORMAT_U_LAW)
	{
		pcmFMT.dwAvgBytesPerSec=pcmFMT.dwSamplesPerSec;//*sizeof(unsigned char);
		pcmFMT.dwSize			=	0x12; 
	}
	else
	{
		return WAV_FORMAT_ERROR;
	}
	//pcmDATA.dwSize			=	0;
	return 0;
}
/*int wav::getHead(FILE *fp )
{
	long file_pos = ftell(fp);
	//Read RIFF_HEADER
	fread(&pcmHEADER, sizeof(HEADER), 1, fp);
	if(	memcmp(pcmHEADER.fccID, "RIFF", 4) != 0 ||
		memcmp(pcmHEADER.fccType, "WAVE", 4) != 0 )
	{
		return(WAV_FLAG_HEADER_ERROR);
	}

	file_pos = ftell(fp);
	//Read pcmFMT
	fread(&pcmFMT, sizeof(FMT), 1, fp);
	if(	memcmp(pcmFMT.fccID, "fmt ", 4) !=0 
		//|| pcmFMT.dwSize != 16
		//|| pcmFMT.wFormatTag != 0x1 
		//|| pcmFMT.wChannels != 0x1 
		//|| pcmFMT.dwSamplesPerSec != 16000 
		//|| pcmFMT.uiBitsPerSample != 16
		)
	{
		return(WAV_FLAG_FMT_ERROR);
	}

	file_pos = ftell(fp);
	if(pcmFMT.wFormatTag == WAVE_FORMAT_MU_LAW
		|| pcmFMT.wFormatTag == WAVE_FORMAT_A_LAW)
	{
	//read FACTdata
		file_pos+=2;
		fseek(fp, file_pos, SEEK_SET);
		fread(&pcmFACT, sizeof(FACT), 1, fp);
		if( memcmp(pcmFACT.szFactID, "fact", 4) != 0 )
		{	
			return(WAV_FLAG_FACT_ERROR);
		}
		//has_fact_block =1;
		fread(&fact_block_buffer,pcmFACT.dwFactSize, 1, fp);
	}
	file_pos = ftell(fp);
	//read PCMdata
	fread(&pcmDATA, sizeof(DATA), 1, fp);
	if (memcmp(pcmDATA.fccID, "data", 4) != 0)
	{
		return(WAV_FLAG_DATA_ERROR);
	}
	return 0;
}

int wav::getHead(const char * in ,unsigned short &wFormatTag,unsigned long &dwSamplesPerSec,
		unsigned short &uiBitsPerSample,unsigned short &wChannels, 
		unsigned short &wBlockAlign)
{
	int nRet = 0 ;
	FILE *fp = fopen(in,"rb");
	if(fp==NULL)
	{
		return WAV_IN_FILE_ERROR;
	}
	nRet = getHead(fp);
	if(nRet != 0)
	{
		fclose(fp);
		return nRet;
	}
	dwSamplesPerSec	=	pcmFMT.dwSamplesPerSec;
	uiBitsPerSample	=	pcmFMT.uiBitsPerSample;
	wChannels		=	pcmFMT.wChannels;
	wFormatTag		=	pcmFMT.wFormatTag;
	wBlockAlign		=	pcmFMT.wBlockAlign; 
	fclose(fp);
	return 0;
}
*/
int wav::pcm2wav(const char * in , const char * out)
{
	unsigned   short   m_pcmData;                               //读入.pcm和写入文件.wav数据变量 
	FILE   *fp,*fpCpy; 
	int nRet = 0;
	if((fp=fopen( in , "rb"))   ==   NULL)	 //读取文件 
	{ 
		return PCM_IN_FILE_ERROR;
	}   

	if((fpCpy=fopen( out , "wb"))   ==   NULL)	 //为转换建立一个新文件 
	{ 
		fclose(fp);
		return WAV_OUT_FILE_ERROR;
	}
	
//以下是创建wav头的HEADER;但.dwsize未定，因为不知道Data的长度。 
//	strcpy(pcmHEADER.fccID, "RIFF");                   
//	strcpy(pcmHEADER.fccType, "WAVE"); 
//以上是创建wav头的HEADER; 

//以下是创建wav头的FMT; 
//	pcmFMT.dwSamplesPerSec=16000; 
//	pcmFMT.dwAvgBytesPerSec=pcmFMT.dwSamplesPerSec*sizeof(m_pcmData); 
//	pcmFMT.uiBitsPerSample=16; 

//	strcpy(pcmFMT.fccID, "fmt "); 
//	pcmFMT.dwSize=16; 
//	pcmFMT.wBlockAlign=2; 
//	pcmFMT.wChannels=1; 
//	pcmFMT.wFormatTag=WAVE_FORMAT_PCM; 
//以上是创建wav头的FMT;  

//	strcpy(pcmDATA.fccID, "data"); 
//以上是创建wav头的DATA; 

//	pcmDATA.dwSize=0;	 //给pcmDATA.dwsize   0以便于下面给它赋值 

	nRet = makeHead(WAVE_FORMAT_PCM,16000,16,1,2);
	if(nRet!=0)
	{
		fclose(fp);
		fclose(fpCpy);
		return nRet;
	}
	fseek(fpCpy,sizeof(HEADER),1);	 //跳过HEADER的长度，以便下面继续写入wav文件的数据; 
	fseek(fpCpy,sizeof(FMT),1);	 //跳过FMT,因为FMT已经写入 
	fseek(fpCpy,sizeof(DATA),1);	 //跳过DATA的长度，以便以后再写入wav头的DATA; 
	pcmDATA.dwSize=0;
	while(!feof(fp))	 //在.pcm文件结束前将他的数据转化并赋给.wav; 
	{ 
		fread(&m_pcmData,sizeof(unsigned   short),1,fp);	 //从.pcm中读入数据 
		pcmDATA.dwSize+=2;	 //计算数据的长度；每读入一个数据，长度就加一； 
		fwrite(&m_pcmData,sizeof(unsigned   short),1,fpCpy);	//将数据写入.wav文件; 
	} 

	pcmHEADER.dwSize=44+pcmDATA.dwSize;	   //根据pcmDATA.dwsize得出pcmHEADER.dwsize的值 
	rewind(fpCpy);	 //将fpCpy变为.wav的头，以便于写入HEADER和DATA; 
	fwrite(&pcmHEADER,sizeof(HEADER),1,fpCpy);	 //写入HEADER 
	fwrite(&pcmFMT,sizeof(FMT),1,fpCpy);	 //将FMT写入.wav文件; 
	fwrite(&pcmDATA,sizeof(DATA),1,fpCpy);	   //写入DATA; 
	fclose(fp);	 //关闭文件 
	fclose(fpCpy);	   //关闭文件
	return WAV_SUCCESS;
}
/*
int wav::wav2pcm(const char * in , const char * out)
{
	unsigned short m_pcmData;                               //读入.wav和写入文件.pcm数据变量 

	int nRet = 0;
	FILE   *fp,*fpCpy;
	if((fp=fopen( in , "rb"))   ==   NULL)	 //读取文件 
	{ 
		return WAV_IN_FILE_ERROR;
	}   

	if((fpCpy=fopen( out , "wb"))   ==   NULL)	 //为转换建立一个新文件 
	{ 
		fclose(fp);
		return PCM_OUT_FILE_ERROR;
	}
	nRet = getHead(fp);
	if(nRet!=0)
	{
		fclose(fp);
		fclose(fpCpy);
		return nRet;
	}
	if(pcmFMT.wFormatTag!=WAVE_FORMAT_PCM)
	{
		return WAV_FORMAT_ERROR;
	}

	while(pcmDATA.dwSize>0)//!feof(fp))	 //在.wav文件结束前将他的数据转化并赋给.pcm;
	{ 
		fread(&m_pcmData,sizeof(unsigned short),1,fp);	 //从.wav中读入数据 
		pcmDATA.dwSize-=2;	 //计算数据的剩余长度；每读入一个数据，长度就减一； 
		fwrite(&m_pcmData,sizeof(unsigned short),1,fpCpy);	//将数据写入.pcm文件; 
	} 
	fclose(fp);	 //关闭文件 
	fclose(fpCpy);	   //关闭文件
	return WAV_SUCCESS;
}


int wav::wav2a(const char * in , const char * out)
{
	unsigned short m_pcmData;                               //读入.pcm和写入文件.wav数据变量 
	unsigned char m_aData;
	int nRet = 0;
	FILE   *fp,*fpCpy; 
	if((fp=fopen( in , "rb"))   ==   NULL)	 //读取文件 
	{ 
		return PCM_IN_FILE_ERROR;
	}   

	if((fpCpy=fopen( out , "wb"))   ==   NULL)	 //为转换建立一个新文件 
	{ 
		return WAV_OUT_FILE_ERROR;
	}
	nRet = getHead(fp);
	if(nRet!=0)
	{
		fclose(fp);
		fclose(fpCpy);
		return nRet;
	}
	if(pcmFMT.wFormatTag!=WAVE_FORMAT_PCM)
	{
		return WAV_FORMAT_ERROR;
	}
	unsigned long pcmSize = pcmDATA.dwSize;
	pcmDATA.dwSize = 0;
	nRet = makeHead(WAVE_FORMAT_A_LAW,16000,8,1,2);//make只对部分属性进行操作，原来的pcmDATA.dwSize没有受到影响
	if(nRet!=0)
	{
		fclose(fp);
		fclose(fpCpy);
		return nRet;
	}
	
	fseek(fpCpy,sizeof(HEADER),1);	 //跳过HEADER的长度，以便下面继续写入wav文件的数据; 
	fseek(fpCpy,sizeof(FMT),1);	 //跳过FMT
	fseek(fpCpy,2,1);	 //fmt最后两个字节
	fseek(fpCpy,sizeof(FACT),1);	 //跳过FACT
	fseek(fpCpy,pcmFACT.dwFactSize,1);	 //跳过FACT
	fseek(fpCpy,sizeof(DATA),1);	 //跳过DATA的长度，以便以后再写入wav头的DATA; 
	fread(&m_pcmData,sizeof(unsigned   short),1,fp);	 //从.pcm中读入数据
	
	while(pcmSize>0)	 //在.pcm文件结束前将他的数据转化并赋给.wav; 
	{ 
		pcmSize-=2;
		pcmDATA.dwSize+=1;	 //计算数据的长度；每读入一个数据，长度就加一； 
		m_aData = G711.linear2alaw(m_pcmData);
		//m_aData = G711.ALawEncode(m_pcmData);
		fwrite(&m_aData,sizeof(unsigned char),1,fpCpy);	//将数据写入.wav文件; 
		fread(&m_pcmData,sizeof(unsigned short),1,fp);	 //从.pcm中读入数据 
	}
	fclose(fp);	 //关闭文件 

	pcmHEADER.dwSize=58+pcmDATA.dwSize;	   //根据pcmDATA.dwsize得出pcmHEADER.dwsize的值 
	rewind(fpCpy);	 //将fpCpy变为.wav的头，以便于写入HEADER和DATA; 
	fwrite(&pcmHEADER,sizeof(HEADER),1,fpCpy);	 //写入HEADER 
	fwrite(&pcmFMT,sizeof(FMT),1,fpCpy);	 //将FMT写入.wav文件; 
	fseek(fpCpy,2,1);	 //fmt最后两个字节
	fwrite(&pcmFACT,sizeof(FACT),1,fpCpy);	   //写入FACT; 
	fwrite(fact_block_buffer,pcmFACT.dwFactSize,1,fpCpy);	   //写入FACT; 
	fwrite(&pcmDATA,sizeof(DATA),1,fpCpy);	   //写入DATA; 
	fclose(fpCpy);	   //关闭文件
	return WAV_SUCCESS;
}

int wav::a2wav(const char * in , const char * out)
{
	unsigned short m_pcmData;                               //读入.pcm和写入文件.wav数据变量 
	unsigned char m_aData;
	int nRet = 0;
	FILE   *fp,*fpCpy; 
	if((fp=fopen( in , "rb"))   ==   NULL)	 //读取文件 
	{ 
		return PCM_IN_FILE_ERROR;
	}   

	if((fpCpy=fopen( out , "wb"))   ==   NULL)	 //为转换建立一个新文件 
	{ 
		return WAV_OUT_FILE_ERROR;
	}
	nRet = getHead(fp);
	if(nRet!=0)
	{
		fclose(fp);
		fclose(fpCpy);
		return nRet;
	}
	if(pcmFMT.wFormatTag!=WAVE_FORMAT_A_LAW)
	{
		return WAV_FORMAT_ERROR;
	}
	unsigned long pcmSize = pcmDATA.dwSize;
	pcmDATA.dwSize = 0;
	nRet = makeHead(WAVE_FORMAT_PCM,16000,16,1,2);//make只对部分属性进行操作，原来的pcmDATA.dwSize没有受到影响
	if(nRet!=0)
	{
		fclose(fp);
		fclose(fpCpy);
		return nRet;
	}
	
	fseek(fpCpy,sizeof(HEADER),1);	 //跳过HEADER的长度，以便下面继续写入wav文件的数据; 
	fseek(fpCpy,sizeof(FMT),1);	 //跳过FMT
	fseek(fpCpy,sizeof(DATA),1);	 //跳过DATA的长度，以便以后再写入wav头的DATA; 
	fread(&m_aData,sizeof(unsigned char),1,fp);	 //从a-law中读入数据
	
	while(pcmSize>0)	 //在.pcm文件结束前将他的数据转化并赋给.wav; 
	{ 
		pcmSize-=1;
		pcmDATA.dwSize+=2;	 //计算数据的长度；每读入一个数据，长度就加一； 
		m_pcmData = G711.alaw2linear(m_aData);
		//m_aData = G711.ALawEncode(m_pcmData);
		fwrite(&m_pcmData,sizeof(unsigned short),1,fpCpy);	//将数据写入wav文件; 
		fread(&m_aData,sizeof(unsigned char),1,fp);	 //从a-low中读入数据 
	}
	fclose(fp);	 //关闭文件 

	pcmHEADER.dwSize=44+pcmDATA.dwSize;	   //根据pcmDATA.dwsize得出pcmHEADER.dwsize的值 
	rewind(fpCpy);	 //将fpCpy变为.wav的头，以便于写入HEADER和DATA; 
	fwrite(&pcmHEADER,sizeof(HEADER),1,fpCpy);	 //写入HEADER 
	fwrite(&pcmFMT,sizeof(FMT),1,fpCpy);	 //将FMT写入.wav文件; 
	fwrite(&pcmDATA,sizeof(DATA),1,fpCpy);	   //写入DATA; 
	fclose(fpCpy);	   //关闭文件
	return WAV_SUCCESS;
}


int wav::wav2u(const char * in , const char * out)
{
	unsigned short m_pcmData;                               //读入.pcm和写入文件.wav数据变量 
	unsigned char m_uData;
	int nRet = 0;
	FILE   *fp,*fpCpy; 
	if((fp=fopen( in , "rb"))   ==   NULL)	 //读取文件 
	{ 
		return PCM_IN_FILE_ERROR;
	}   

	if((fpCpy=fopen( out , "wb"))   ==   NULL)	 //为转换建立一个新文件 
	{ 
		return WAV_OUT_FILE_ERROR;
	}
	nRet = getHead(fp);
	if(nRet!=0)
	{
		fclose(fp);
		fclose(fpCpy);
		return nRet;
	}
	if(pcmFMT.wFormatTag!=WAVE_FORMAT_PCM)
	{
		return WAV_FORMAT_ERROR;
	}
	unsigned long pcmSize = pcmDATA.dwSize;
	pcmDATA.dwSize = 0;
	nRet = makeHead(WAVE_FORMAT_U_LAW,16000,8,1,2);//make只对部分属性进行操作，原来的pcmDATA.dwSize没有受到影响
	if(nRet!=0)
	{
		fclose(fp);
		fclose(fpCpy);
		return nRet;
	}
	
	fseek(fpCpy,sizeof(HEADER),1);	 //跳过HEADER的长度，以便下面继续写入wav文件的数据; 
	fseek(fpCpy,sizeof(FMT),1);	 //跳过FMT
	fseek(fpCpy,2,1);	 //fmt最后两个字节
	fseek(fpCpy,sizeof(FACT),1);	 //跳过FACT
	fseek(fpCpy,pcmFACT.dwFactSize,1);	 //跳过FACT
	fseek(fpCpy,sizeof(DATA),1);	 //跳过DATA的长度，以便以后再写入wav头的DATA; 
	fread(&m_pcmData,sizeof(unsigned   short),1,fp);	 //从.pcm中读入数据
	
	while(pcmSize>0)	 //在.pcm文件结束前将他的数据转化并赋给.wav; 
	{ 
		pcmSize-=2;
		pcmDATA.dwSize+=1;	 //计算数据的长度；每读入一个数据，长度就加一； 
		m_uData = G711.linear2ulaw(m_pcmData);
		//m_uData = G711.ALawEncode(m_pcmData);
		fwrite(&m_uData,sizeof(unsigned char),1,fpCpy);	//将数据写入.wav文件; 
		fread(&m_pcmData,sizeof(unsigned short),1,fp);	 //从.pcm中读入数据 
	}
	fclose(fp);	 //关闭文件 

	pcmHEADER.dwSize=58+pcmDATA.dwSize;	   //根据pcmDATA.dwsize得出pcmHEADER.dwsize的值 
	rewind(fpCpy);	 //将fpCpy变为.wav的头，以便于写入HEADER和DATA; 
	fwrite(&pcmHEADER,sizeof(HEADER),1,fpCpy);	 //写入HEADER 
	fwrite(&pcmFMT,sizeof(FMT),1,fpCpy);	 //将FMT写入.wav文件; 
	fseek(fpCpy,2,1);	 //fmt最后两个字节
	fwrite(&pcmFACT,sizeof(FACT),1,fpCpy);	   //写入FACT; 
	fwrite(fact_block_buffer,pcmFACT.dwFactSize,1,fpCpy);	   //写入FACT; 
	fwrite(&pcmDATA,sizeof(DATA),1,fpCpy);	   //写入DATA; 
	fclose(fpCpy);	   //关闭文件
	return WAV_SUCCESS;
}

int wav::u2wav(const char * in , const char * out)
{
	unsigned short m_pcmData;                               //读入.pcm和写入文件.wav数据变量 
	unsigned char m_uData;
	int nRet = 0;
	FILE   *fp,*fpCpy; 
	if((fp=fopen( in , "rb"))   ==   NULL)	 //读取文件 
	{ 
		return PCM_IN_FILE_ERROR;
	}   

	if((fpCpy=fopen( out , "wb"))   ==   NULL)	 //为转换建立一个新文件 
	{ 
		return WAV_OUT_FILE_ERROR;
	}
	nRet = getHead(fp);
	if(nRet!=0)
	{
		fclose(fp);
		fclose(fpCpy);
		return nRet;
	}
	if(pcmFMT.wFormatTag!=WAVE_FORMAT_U_LAW)
	{
		return WAV_FORMAT_ERROR;
	}
	unsigned long pcmSize = pcmDATA.dwSize;
	pcmDATA.dwSize = 0;
	nRet = makeHead(WAVE_FORMAT_PCM,16000,16,1,2);//make只对部分属性进行操作，原来的pcmDATA.dwSize没有受到影响
	if(nRet!=0)
	{
		fclose(fp);
		fclose(fpCpy);
		return nRet;
	}
	
	fseek(fpCpy,sizeof(HEADER),1);	 //跳过HEADER的长度，以便下面继续写入wav文件的数据; 
	fseek(fpCpy,sizeof(FMT),1);	 //跳过FMT
	fseek(fpCpy,sizeof(DATA),1);	 //跳过DATA的长度，以便以后再写入wav头的DATA; 
	fread(&m_uData,sizeof(unsigned char),1,fp);	 //从a-law中读入数据
	
	while(pcmSize>0)	 //在.pcm文件结束前将他的数据转化并赋给.wav; 
	{ 
		pcmSize-=1;
		pcmDATA.dwSize+=2;	 //计算数据的长度；每读入一个数据，长度就加一； 
		m_pcmData = G711.ulaw2linear(m_uData);
		//m_uData = G711.ALawEncode(m_pcmData);
		fwrite(&m_pcmData,sizeof(unsigned short),1,fpCpy);	//将数据写入wav文件; 
		fread(&m_uData,sizeof(unsigned char),1,fp);	 //从a-low中读入数据 
	}
	fclose(fp);	 //关闭文件 

	pcmHEADER.dwSize=44+pcmDATA.dwSize;	   //根据pcmDATA.dwsize得出pcmHEADER.dwsize的值 
	rewind(fpCpy);	 //将fpCpy变为.wav的头，以便于写入HEADER和DATA; 
	fwrite(&pcmHEADER,sizeof(HEADER),1,fpCpy);	 //写入HEADER 
	fwrite(&pcmFMT,sizeof(FMT),1,fpCpy);	 //将FMT写入.wav文件; 
	fwrite(&pcmDATA,sizeof(DATA),1,fpCpy);	   //写入DATA; 
	fclose(fpCpy);	   //关闭文件
	return WAV_SUCCESS;
}

int wav::u2a(const char * in , const char * out)
{
	unsigned char m_uData;                               //读入.pcm和写入文件.wav数据变量 
	unsigned char m_aData;
	int nRet = 0;
	FILE   *fp,*fpCpy; 
	if((fp=fopen( in , "rb"))   ==   NULL)	 //读取文件 
	{ 
		return PCM_IN_FILE_ERROR;
	}   

	if((fpCpy=fopen( out , "wb"))   ==   NULL)	 //为转换建立一个新文件 
	{ 
		return WAV_OUT_FILE_ERROR;
	}
	nRet = getHead(fp);
	if(nRet!=0)
	{
		fclose(fp);
		fclose(fpCpy);
		return nRet;
	}
	if(pcmFMT.wFormatTag!=WAVE_FORMAT_U_LAW)
	{
		return WAV_FORMAT_ERROR;
	}
	unsigned long pcmSize = pcmDATA.dwSize;
	pcmDATA.dwSize = 0;
	nRet = makeHead(WAVE_FORMAT_A_LAW,16000,8,1,2);//make只对部分属性进行操作，原来的pcmDATA.dwSize没有受到影响
	if(nRet!=0)
	{
		fclose(fp);
		fclose(fpCpy);
		return nRet;
	}
	
	
	fseek(fpCpy,sizeof(HEADER),1);	 //跳过HEADER的长度，以便下面继续写入wav文件的数据; 
	fseek(fpCpy,sizeof(FMT),1);	 //跳过FMT
	fseek(fpCpy,2,1);	 //fmt最后两个字节
	fseek(fpCpy,sizeof(FACT),1);	 //跳过FACT
	fseek(fpCpy,pcmFACT.dwFactSize,1);	 //跳过FACT
	fseek(fpCpy,sizeof(DATA),1);	 //跳过DATA的长度，以便以后再写入wav头的DATA; 
	fread(&m_uData,sizeof(unsigned char),1,fp);	 //从a-law中读入数据
	
	while(pcmSize>0)	 //在.pcm文件结束前将他的数据转化并赋给.wav; 
	{ 
		pcmSize-=1;
		pcmDATA.dwSize+=1;	 //计算数据的长度；每读入一个数据，长度就加一； 
		m_aData = G711.ulaw2alaw(m_uData);
		//m_aData = G711.ALawEncode(m_pcmData);
		fwrite(&m_aData,sizeof(unsigned char),1,fpCpy);	//将数据写入wav文件; 
		fread(&m_uData,sizeof(unsigned char),1,fp);	 //从a-low中读入数据 
	}
	fclose(fp);	 //关闭文件 

	pcmHEADER.dwSize=58+pcmDATA.dwSize;	   //根据pcmDATA.dwsize得出pcmHEADER.dwsize的值 
	rewind(fpCpy);	 //将fpCpy变为.wav的头，以便于写入HEADER和DATA; 
	fwrite(&pcmHEADER,sizeof(HEADER),1,fpCpy);	 //写入HEADER 
	fwrite(&pcmFMT,sizeof(FMT),1,fpCpy);	 //将FMT写入.wav文件; 
	fseek(fpCpy,2,1);	 //fmt最后两个字节
	fwrite(&pcmFACT,sizeof(FACT),1,fpCpy);	   //写入FACT; 
	fwrite(fact_block_buffer,pcmFACT.dwFactSize,1,fpCpy);	   //写入FACT; 
	fwrite(&pcmDATA,sizeof(DATA),1,fpCpy);	   //写入DATA; 
	fclose(fpCpy);	   //关闭文件
	return WAV_SUCCESS;
}

int wav::a2u(const char * in , const char * out)
{
	unsigned char m_uData;                               //读入.pcm和写入文件.wav数据变量 
	unsigned char m_aData;
	int nRet = 0;
	FILE   *fp,*fpCpy; 
	if((fp=fopen( in , "rb"))   ==   NULL)	 //读取文件 
	{ 
		return PCM_IN_FILE_ERROR;
	}   

	if((fpCpy=fopen( out , "wb"))   ==   NULL)	 //为转换建立一个新文件 
	{ 
		return WAV_OUT_FILE_ERROR;
	}
	nRet = getHead(fp);
	if(nRet!=0)
	{
		fclose(fp);
		fclose(fpCpy);
		return nRet;
	}
	if(pcmFMT.wFormatTag!=WAVE_FORMAT_A_LAW)
	{
		return WAV_FORMAT_ERROR;
	}
	unsigned long pcmSize = pcmDATA.dwSize;
	pcmDATA.dwSize = 0;
	nRet = makeHead(WAVE_FORMAT_U_LAW,16000,8,1,2);//make只对部分属性进行操作，原来的pcmDATA.dwSize没有受到影响
	if(nRet!=0)
	{
		fclose(fp);
		fclose(fpCpy);
		return nRet;
	}
	
	
	fseek(fpCpy,sizeof(HEADER),1);	 //跳过HEADER的长度，以便下面继续写入wav文件的数据; 
	fseek(fpCpy,sizeof(FMT),1);	 //跳过FMT
	fseek(fpCpy,2,1);	 //fmt最后两个字节
	fseek(fpCpy,sizeof(FACT),1);	 //跳过FACT
	fseek(fpCpy,pcmFACT.dwFactSize,1);	 //跳过FACT
	fseek(fpCpy,sizeof(DATA),1);	 //跳过DATA的长度，以便以后再写入wav头的DATA; 
	fread(&m_aData,sizeof(unsigned char),1,fp);	 //从a-law中读入数据
	
	while(pcmSize>0)	 //在.pcm文件结束前将他的数据转化并赋给.wav; 
	{ 
		pcmSize-=1;
		pcmDATA.dwSize+=1;	 //计算数据的长度；每读入一个数据，长度就加一； 
		m_uData = G711.alaw2ulaw(m_aData);
		//m_aData = G711.ALawEncode(m_pcmData);
		fwrite(&m_uData,sizeof(unsigned char),1,fpCpy);	//将数据写入wav文件; 
		fread(&m_aData,sizeof(unsigned char),1,fp);	 //从a-low中读入数据 
	}
	fclose(fp);	 //关闭文件 

	pcmHEADER.dwSize=58+pcmDATA.dwSize;	   //根据pcmDATA.dwsize得出pcmHEADER.dwsize的值 
	rewind(fpCpy);	 //将fpCpy变为.wav的头，以便于写入HEADER和DATA; 
	fwrite(&pcmHEADER,sizeof(HEADER),1,fpCpy);	 //写入HEADER 
	fwrite(&pcmFMT,sizeof(FMT),1,fpCpy);	 //将FMT写入.wav文件; 
	fseek(fpCpy,2,1);	 //fmt最后两个字节
	fwrite(&pcmFACT,sizeof(FACT),1,fpCpy);	   //写入FACT; 
	fwrite(fact_block_buffer,pcmFACT.dwFactSize,1,fpCpy);	   //写入FACT; 
	fwrite(&pcmDATA,sizeof(DATA),1,fpCpy);	   //写入DATA; 
	fclose(fpCpy);	   //关闭文件
	return WAV_SUCCESS;
}
*/