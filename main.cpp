/*
* main.cpp
*
*  Created on: 2016-7-1
*      Author: Wang Yang
*        Mail: admin@wysaid.org
*/

#define SHOW_CONSOLE

#include "graphics.h"
#include "al.h"
#include "alc.h"

#pragma comment(lib, "OpenAL32.lib")

#include <list>

#define SCR_WIDTH 800
#define SCR_HEIGHT 600

//前置wy宣示主权
#define WY_COMMON_CREATE_FUNC(cls, funcName) \
static inline cls* create() \
{\
cls* instance = new cls(); \
if(!instance->funcName()) \
{ \
delete instance; \
instance = nullptr; \
WY_LOG_ERROR("create %s failed!", #cls); \
} \
return instance; \
}

#define WY_LOG_INFO(...) printf(__VA_ARGS__)

#define WY_LOG_ERROR(str, ...) \
do{\
fprintf(stderr, "xxxxx");\
fprintf(stderr, str, ##__VA_ARGS__);\
fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);\
fprintf(stderr, "xxxxx\n");\
}while(0)

#define wyCheckALError(tag) _wyCheckALError(tag, __FILE__, __LINE__)

static inline bool _wyCheckALError(const char* tag, const char* file, int line)
{
	int loopCnt = 0;
	for (ALenum error = alGetError(); loopCnt < 32 && error; error = alGetError(), ++loopCnt)
	{
		const char* pMsg;
		switch (error)
		{
		case AL_INVALID_NAME: pMsg = "invalid name"; break;
		case AL_INVALID_ENUM: pMsg = "invalid enum"; break;
		case AL_INVALID_VALUE: pMsg = "invalid value"; break;
		case AL_INVALID_OPERATION: pMsg = "invalid operation"; break;
		case AL_OUT_OF_MEMORY: pMsg = "out of memory"; break;
		default: pMsg = "unknown error";
		}
		WY_LOG_ERROR("After \"%s\" alGetError %s(0x%x) at %s:%d\n", tag, pMsg, error, file, line);
	}

	return loopCnt != 0;
}

void wyPrintALString(const char* name, ALenum em)
{
	WY_LOG_INFO("OpenAL info %s = %s\n", name, alGetString(em));
}

void wyPrintALInfo()
{
	wyPrintALString("Vendor", AL_VENDOR);
	wyPrintALString("Version", AL_VERSION);
	wyPrintALString("Renderer", AL_RENDERER);
	wyPrintALString("Extension", AL_EXTENSIONS);
}

//////////////////////////////////////////////////////////////////////////

class WYPlayBack
{
public:
	WYPlayBack() : m_context(nullptr), m_device(nullptr), m_source(0), m_buffer()
	{

	}

	~WYPlayBack()
	{
		clear();
	}

	enum { MAX_CACHE = 32 };

	WY_COMMON_CREATE_FUNC(WYPlayBack, init);

	bool init()
	{
		m_device = alcOpenDevice(nullptr);
		if (m_device == nullptr)
		{
			WY_LOG_INFO("Create device failed!\n");;
			return false;
		}

		m_context = alcCreateContext(m_device, nullptr);
		if (m_context == nullptr)
		{
			WY_LOG_INFO("Create context failed!\n");;
			return false;
		}

		alcMakeContextCurrent(m_context);
		alGenBuffers(MAX_CACHE, m_buffer);
		alGenSources(1, &m_source);

		if (wyCheckALError("CGEALPlayBack::init"))
			return false;

		alGenSources(1, &m_source);
		alGenBuffers(MAX_CACHE, m_buffer);

		if (wyCheckALError("CGEALPlayBack::init"))
			return false;

		for (auto buf : m_buffer)
		{
			m_bufferQueue.push_back(buf);
		}

		return true;
	}

	void clear()
	{
		if (m_context != nullptr)
		{
			makeALCurrent();
			alDeleteSources(1, &m_source);
			m_source = 0;
			alDeleteBuffers(MAX_CACHE, m_buffer);
			memset(m_buffer, 0, sizeof(m_buffer));
			alcMakeContextCurrent(nullptr);
			alcDestroyContext(m_context);
			m_context = nullptr;
			alcCloseDevice(m_device);
			m_device = nullptr;
			m_bufferQueue.clear();
		}
	}

	void play(const void* data, int dataSize, int freq, ALenum format)
	{
		if (data != nullptr && !m_bufferQueue.empty())
		{
			ALuint buffer = m_bufferQueue.front();
			m_bufferQueue.pop_front();
			alBufferData(buffer, format, data, dataSize, freq);
			alSourceQueueBuffers(m_source, 1, &buffer);
		}

		resume();
	}

	void resume()
	{
		if (!isPlaying())
		{
			ALint bufferQueued;
			alGetSourcei(m_source, AL_BUFFERS_QUEUED, &bufferQueued);
			if (bufferQueued != 0)
				alSourcePlay(m_source);
		}
	}

	void pause()
	{
		if (isPlaying())
		{
			alSourcePause(m_source);
		}
	}

	void stop()
	{
		if (isPlaying())
		{
			alSourceStop(m_source);
		}
	}

	bool isPlaying()
	{
		return playState() == AL_PLAYING;
	}

	ALint playState()
	{
		ALint playState = 0;
		alGetSourcei(m_source, AL_SOURCE_STATE, &playState);
		return playState;
	}

	void recycle()
	{
		ALint procBuffNum;
		alGetSourcei(m_source, AL_BUFFERS_PROCESSED, &procBuffNum);
		if (procBuffNum > 0)
		{
			ALuint buffers[MAX_CACHE];
			alSourceUnqueueBuffers(m_source, procBuffNum, buffers);
			for (int i = 0; i != procBuffNum; ++i)
			{
				m_bufferQueue.push_back(buffers[i]);
			}
		}
	}

	void makeALCurrent()
	{
		alcMakeContextCurrent(m_context);
	}

	inline ALCdevice* getDevice() { return m_device; }
	inline ALCcontext* getContext() { return m_context; }
	inline std::list<ALuint>& getBufferQueue() { return m_bufferQueue; }

	////////////////////////////////////

// 	void setSourceReverb(ALfloat level);
// 	void setSourceOcclusion(ALfloat level);
// 	void setSourceObstruction(ALfloat level);
// 	void setGlobalReverb(ALfloat level);
// 
// 	//status 取值0或1
// 	static void setReverbOn(ALint status);
// 	//        static bool isReverbOn();
// 	static void setReverbEQGain(ALfloat level);
// 	static void setReverbEQBandwidth(ALfloat level);
// 	static void setReverbEQFrequency(ALfloat level);

	//内建13种方式
	/* reverb room type presets for the ALC_ASA_REVERB_ROOM_TYPE property
	#define ALC_ASA_REVERB_ROOM_TYPE_SmallRoom			0
	#define ALC_ASA_REVERB_ROOM_TYPE_MediumRoom			1
	#define ALC_ASA_REVERB_ROOM_TYPE_LargeRoom			2
	#define ALC_ASA_REVERB_ROOM_TYPE_MediumHall			3
	#define ALC_ASA_REVERB_ROOM_TYPE_LargeHall			4
	#define ALC_ASA_REVERB_ROOM_TYPE_Plate				5
	#define ALC_ASA_REVERB_ROOM_TYPE_MediumChamber		6
	#define ALC_ASA_REVERB_ROOM_TYPE_LargeChamber		7
	#define ALC_ASA_REVERB_ROOM_TYPE_Cathedral			8
	#define ALC_ASA_REVERB_ROOM_TYPE_LargeRoom2			9
	#define ALC_ASA_REVERB_ROOM_TYPE_MediumHall2		10
	#define ALC_ASA_REVERB_ROOM_TYPE_MediumHall3		11
	#define ALC_ASA_REVERB_ROOM_TYPE_LargeHall2			12
	*/
//	static void setReverbRoomType(ALuint roomIndex);

protected:
	ALCcontext* m_context;
	ALCdevice* m_device;
	ALuint m_source;
	ALuint m_buffer[MAX_CACHE];

	std::list<ALuint> m_bufferQueue;
};

int main()
{
	WYPlayBack* playback = WYPlayBack::create();

	if (playback == nullptr)
	{
		//未检测到输出设备, 你的电脑可能没有插耳机或者音响。
		WY_LOG_ERROR("Create playback failed! Please check if your pc has any output audio device");
		return -1;
	}

	playback->makeALCurrent();

	wyPrintALInfo();

	static const int FREQ = 44100; //音频采样率
	static const int CAPTURE_SIZE = 512; //缓冲区大小
	ALCdevice* micDevice = alcCaptureOpenDevice(nullptr, FREQ, AL_FORMAT_MONO16, FREQ / 10);
	if (micDevice == nullptr || alcGetError(micDevice))
	{
		//未检测到输入设备， 你的电脑可能没有插麦克风
		WY_LOG_ERROR("Start microphone failed! Please check if you have an input device!");
		return -1;
	}

	alcCaptureStart(micDevice);

	//取消ege界面logo
	setinitmode(0);
	initgraph(SCR_WIDTH, SCR_HEIGHT);
	setrendermode(RENDER_MANUAL);
	setlinewidth(2.0f);

	ALint samples;
	bool mute = false, shouldRedraw = false;
	const float waveScalingX = SCR_WIDTH / (float)CAPTURE_SIZE;
	const float waveScalingY = 0.008f;
	short audioBuffer[CAPTURE_SIZE];
	int colorLoop = 0;

	auto clearFunc = [&] {
		if (micDevice != nullptr)
		{
			alcCaptureStop(micDevice);
			alcCaptureCloseDevice(micDevice);
			micDevice = nullptr;
		}
		
		if (playback != nullptr)
		{
			delete playback;
			playback = nullptr;
		}
	};

	for (; is_run(); delay_fps(60))
	{
		if (kbhit())
		{
			int ch = getch();
			switch (ch)
			{
			case key_space:
				mute = !mute;
				WY_LOG_INFO("mute: %d\n", (int)mute);
				break;
			case key_esc:
				clearFunc();
				return 0;
			default:;
			}
		}

		shouldRedraw = false;

		while (!mute)
		{
			alcGetIntegerv(micDevice, ALC_CAPTURE_SAMPLES, 1, &samples);
//			WY_LOG_INFO("sample: %d\n", samples);
			if (samples > CAPTURE_SIZE)
			{
				alcCaptureSamples(micDevice, audioBuffer, CAPTURE_SIZE);
				playback->recycle();
				playback->play(audioBuffer, sizeof(audioBuffer), FREQ, AL_FORMAT_MONO16);
				shouldRedraw = true;
			}
			else
			{
				break;
			}
		}

		if (shouldRedraw)
		{
			imagefilter_blurring(nullptr, 0x100, 0x100);

			++colorLoop;
			colorLoop %= 360;
			color_t c = hsv2rgb(colorLoop, 1.0f, 1.0f);
			setcolor(c);

			for (int i = 1; i != CAPTURE_SIZE; ++i)
			{
				int bufValue1 = audioBuffer[i - 1] * waveScalingY + (SCR_HEIGHT / 2);
				int bufValue2 = audioBuffer[i] * waveScalingY + (SCR_HEIGHT / 2);
				line((i - 1) * waveScalingX, bufValue1, i * waveScalingX, bufValue2);
			}
		}
		
	}

	clearFunc();

	return 0;
}