#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#pragma comment(lib, "winmm.lib")

#define BUFFER_NUM	(3)
#define READ_COUNT	(4096)

typedef struct {
	unsigned char id[4];			// R I F F
	unsigned long size;				// file size - 8
	unsigned char format[4];		// W A V E
	unsigned char fmtId[4];			// fmt<sp>
	unsigned long formatSize;		// Linear PCM:16
	unsigned short soundFormat;		// PCM:1
	unsigned short channel;			// Stereo:2
	unsigned long samplingFreq;		// [Hz]
	unsigned long bytePerSec;		// samplingFreq * channel
	unsigned short blockSize;		// Stereo 16bit:4[byte]
	unsigned short bitPerSample;	// 16bit:16
	unsigned char dataId[4];		// d a t a
	unsigned long dataSize;			// [byte]
									// data[]
} WAV_HEADER;

WAV_HEADER wavHeader;

FILE* fp;
WAVEFORMATEX wf;
HWAVEOUT hWout;
WAVEHDR wh[BUFFER_NUM];
short* buf[BUFFER_NUM];
int buffSelector = 0;

long readWavFile(void *buf) {
	return sizeof(short) * fread(buf, sizeof(short), READ_COUNT, fp);
}

void CALLBACK setBuffer(void) {

	wh[buffSelector].dwBufferLength = readWavFile(buf[buffSelector]);
	if (wh[buffSelector].dwBufferLength == 0) {
		fseek(fp, - (long)wavHeader.dataSize, SEEK_CUR);
		wh[buffSelector].dwBufferLength = readWavFile(buf[buffSelector]);
	}

	waveOutWrite(hWout, &wh[buffSelector], sizeof(WAVEHDR));
	buffSelector = (buffSelector + 1) % BUFFER_NUM;
	printf("buffer[%d] set.\r\n", buffSelector);

	return;
}


int main(void) {
	int dbgret = 0;

	fp = fopen("test.wav", "rb");
	if (fp == NULL) {
		exit(-1);
	}
	fread(&wavHeader, sizeof(WAV_HEADER), 1, fp);


	wf.wFormatTag = WAVE_FORMAT_PCM;
	wf.nChannels = wavHeader.channel;
	wf.nSamplesPerSec = wavHeader.samplingFreq;
	wf.wBitsPerSample = wavHeader.bitPerSample;
	wf.nBlockAlign = wf.nChannels * wf.wBitsPerSample / 8;
	wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
	wf.cbSize = 0;

	for (int i = 0; i < BUFFER_NUM; i++) {
		buf[i] = (short*)malloc(READ_COUNT * sizeof(short));
	}

	dbgret = waveOutOpen(&hWout, WAVE_MAPPER, &wf, setBuffer, 0, CALLBACK_FUNCTION);

	for (int i = 0; i < BUFFER_NUM; i++) {
		
		wh[i].lpData = buf[i];
		wh[i].dwBufferLength = 0;// sizeof(short)* wf.nChannels * 65536 * 1;
		wh[i].dwFlags = 0;
		wh[i].dwLoops = 1;
		wh[i].dwBytesRecorded = 0;
		wh[i].dwUser = 0;
		wh[i].lpNext = NULL;
		wh[i].reserved = 0;
		dbgret = waveOutPrepareHeader(hWout, &wh[i], sizeof(WAVEHDR));
	}
	 
	dbgret = waveOutWrite(hWout, &wh[0], sizeof(WAVEHDR));
	dbgret = waveOutWrite(hWout, &wh[1], sizeof(WAVEHDR));
	buffSelector = (buffSelector + 1) % BUFFER_NUM;
	printf("buffer[%d] set.\r\n", buffSelector);

	while (1) {
		int c;
		c = getchar();

		for (int i = 0; i < BUFFER_NUM; i++) {
			waveOutUnprepareHeader(hWout, &wh[i], sizeof(WAVEHDR));
			free(buf[i]);
		}
		waveOutClose(hWout);
		break;
	}

	return 0;
}