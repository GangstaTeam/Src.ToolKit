#include "ui/AudioPlayer.hh"

#include <windows.h>
#include <mmsystem.h>

#define STB_VORBIS_HEADER_ONLY
#include "stb/stb_vorbis.c"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <thread>

namespace tk
{

namespace
{

const std::string kNoError;
const std::vector<float> kNoPeaks;

uint32_t U32(const Bytes& bytes, size_t offset)
{
    if (offset + 4 > bytes.size())
    {
        return 0;
    }

    uint32_t value = 0;
    std::memcpy(&value, bytes.data() + offset, 4);
    return value;
}

uint16_t U16(const Bytes& bytes, size_t offset)
{
    if (offset + 2 > bytes.size())
    {
        return 0;
    }

    uint16_t value = 0;
    std::memcpy(&value, bytes.data() + offset, 2);
    return value;
}

bool HasTag(const Bytes& bytes, size_t offset, const char* tag)
{
    size_t length = std::strlen(tag);
    return offset + length <= bytes.size() && std::memcmp(bytes.data() + offset, tag, length) == 0;
}

size_t FindTag(const Bytes& bytes, size_t from, size_t limit, const char* tag)
{
    size_t length = std::strlen(tag);
    size_t end = std::min(bytes.size(), limit);

    for (size_t i = from; i + length <= end; ++i)
    {
        if (std::memcmp(bytes.data() + i, tag, length) == 0)
        {
            return i;
        }
    }

    return std::string::npos;
}

bool IsRsd(const Bytes& bytes)
{
    return HasTag(bytes, 0, "RSD") && bytes.size() >= kRsdNameOffset;
}

bool IsWave(const Bytes& bytes)
{
    return HasTag(bytes, 0, "RIFF") && HasTag(bytes, 8, "WAVE");
}

std::string CodecName(const std::string& tag)
{
    if (tag == "OOGV")
    {
        return "Ogg Vorbis";
    }

    if (tag == "PCM " || tag == "PCMB")
    {
        return "PCM";
    }

    return tag;
}

std::string TrimOriginal(std::string name)
{
    std::string lower = name;

    for (char& c : lower)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    size_t root = lower.find("scarface\\");

    if (root != std::string::npos)
    {
        name.erase(0, root + std::strlen("scarface\\"));
    }

    return name;
}

bool FindWaveChunk(const Bytes& bytes, const char* tag, size_t& offset, size_t& size)
{
    size_t at = 12;

    while (at + 8 <= bytes.size())
    {
        uint32_t length = U32(bytes, at + 4);

        if (HasTag(bytes, at, tag))
        {
            offset = at + 8;
            size = std::min<size_t>(length, bytes.size() - offset);
            return true;
        }

        at += 8 + length + (length & 1);
    }

    return false;
}

void DecodeVorbis(const Bytes& bytes, size_t offset, std::vector<int16_t>& pcm,
    uint32_t& channels, uint32_t& sampleRate, std::string& error)
{
    int outChannels = 0, outRate = 0;
    short* output = nullptr;
    int frames = stb_vorbis_decode_memory(bytes.data() + offset,
        static_cast<int>(bytes.size() - offset),
        &outChannels,
        &outRate,
        &output);

    if (frames <= 0 || !output || outChannels <= 0)
    {
        error = "The Ogg Vorbis stream could not be decoded.";
        std::free(output);
        return;
    }

    pcm.assign(output, output + static_cast<size_t>(frames) * outChannels);
    std::free(output);
    channels = static_cast<uint32_t>(outChannels);
    sampleRate = static_cast<uint32_t>(outRate);
}

void Decode(const Bytes& bytes, std::vector<int16_t>& pcm, uint32_t& channels, uint32_t& sampleRate,
    std::string& error)
{
    AudioInfo info;
    ReadAudioInfo(bytes, info);

    if (IsRsd(bytes))
    {
        if (info.codec != "Ogg Vorbis")
        {
            error = "This clip uses the " + info.codec + " codec, which the player does not decode yet.";
            return;
        }

        size_t ogg = FindTag(bytes, kRsdNameOffset, kAudioHeaderBytes, "OggS");

        if (ogg == std::string::npos)
        {
            error = "No Ogg stream found after the RSD header.";
            return;
        }

        return DecodeVorbis(bytes, ogg, pcm, channels, sampleRate, error);
    }

    if (HasTag(bytes, 0, "OggS"))
    {
        return DecodeVorbis(bytes, 0, pcm, channels, sampleRate, error);
    }

    if (IsWave(bytes))
    {
        size_t fmt = 0, fmtSize = 0, data = 0, dataSize = 0;

        if (!FindWaveChunk(bytes, "fmt ", fmt, fmtSize) || !FindWaveChunk(bytes, "data", data, dataSize) ||
            fmtSize < 16)
        {
            error = "Broken WAV file (missing fmt or data chunk).";
            return;
        }

        if (U16(bytes, fmt) != 1 || U16(bytes, fmt + 14) != 16 || U16(bytes, fmt + 2) == 0)
        {
            error = "Only 16-bit PCM WAV files can be played.";
            return;
        }

        channels = U16(bytes, fmt + 2);
        sampleRate = U32(bytes, fmt + 4);
        size_t samples = dataSize / 2;
        samples -= samples % channels;
        pcm.resize(samples);
        std::memcpy(pcm.data(), bytes.data() + data, samples * 2);
        return;
    }

    error = "Unknown audio format.";
}

std::vector<float> BuildPeaks(const std::vector<int16_t>& pcm, uint32_t channels)
{
    std::vector<float> peaks(kAudioPeaks, 0.0f);
    size_t frames = channels ? pcm.size() / channels : 0;

    if (!frames)
    {
        return peaks;
    }

    for (size_t frame = 0; frame < frames; ++frame)
    {
        size_t bucket = frame * kAudioPeaks / frames;

        for (uint32_t channel = 0; channel < channels; ++channel)
        {
            float value = std::abs(static_cast<float>(pcm[frame * channels + channel])) / 32768.0f;
            peaks[bucket] = std::max(peaks[bucket], value);
        }
    }

    return peaks;
}

}

bool ReadAudioInfo(const Bytes& bytes, AudioInfo& out)
{
    out = AudioInfo();

    if (IsRsd(bytes))
    {
        out.container = std::string(reinterpret_cast<const char*>(bytes.data()), 4);
        out.codec = CodecName(std::string(reinterpret_cast<const char*>(bytes.data()) + 4, 4));
        out.channels = U32(bytes, 8);
        out.bits = U32(bytes, 12);
        out.sampleRate = U32(bytes, 16);
        size_t end = std::min(bytes.size(), static_cast<size_t>(kRsdNameOffset + kRsdNameBytes));
        size_t length = 0;

        while (kRsdNameOffset + length < end && bytes[kRsdNameOffset + length] != 0)
        {
            ++length;
        }

        out.originalName =
            TrimOriginal(std::string(reinterpret_cast<const char*>(bytes.data()) + kRsdNameOffset, length));
        return true;
    }

    if (HasTag(bytes, 0, "OggS"))
    {
        out.container = "Ogg";
        out.codec = "Ogg Vorbis";
        size_t id = FindTag(bytes, 0, kAudioHeaderBytes, "\x01vorbis");

        if (id != std::string::npos && id + 16 <= bytes.size())
        {
            out.channels = bytes[id + 11];
            out.sampleRate = U32(bytes, id + 12);
            out.bits = 16;
        }

        return true;
    }

    if (IsWave(bytes))
    {
        size_t fmt = 0, fmtSize = 0;
        out.container = "WAV";

        if (FindWaveChunk(bytes, "fmt ", fmt, fmtSize) && fmtSize >= 16)
        {
            uint16_t format = U16(bytes, fmt);
            out.codec = format == 1 ? "PCM" : "format " + std::to_string(format);
            out.channels = U16(bytes, fmt + 2);
            out.sampleRate = U32(bytes, fmt + 4);
            out.bits = U16(bytes, fmt + 14);
        }

        return true;
    }

    return false;
}

struct AudioPlayer::Device
{
    HWAVEOUT handle = nullptr;
    WAVEHDR header = {};
};

AudioPlayer::AudioPlayer() = default;

AudioPlayer::~AudioPlayer()
{
    Close();
}

void AudioPlayer::Load(uint32_t key, Bytes bytes, bool autoPlay)
{
    Clear();
    m_key = key;
    m_autoPlay = autoPlay;
    auto state = std::make_shared<Decoded>();
    m_pending = state;

    std::thread(
        [state, data = std::move(bytes)]
        {
            Decode(data, state->pcm, state->channels, state->sampleRate, state->error);

            if (state->error.empty() && state->pcm.empty())
            {
                state->error = "The clip is empty.";
            }

            if (state->error.empty())
            {
                state->peaks = BuildPeaks(state->pcm, state->channels);
            }

            state->done = true;
        })
        .detach();
}

void AudioPlayer::Clear()
{
    Close();
    m_key = 0;
    m_pending.reset();
    m_decoded.reset();
    m_cursor = 0;
    m_autoPlay = false;
}

void AudioPlayer::Tick()
{
    if (m_pending && m_pending->done)
    {
        m_decoded = std::move(m_pending);
        m_pending.reset();

        if (m_autoPlay && Ready())
        {
            Play();
        }
    }

    if (m_device && (m_device->header.dwFlags & WHDR_DONE))
    {
        Close();
        m_cursor = 0;

        if (m_loop)
        {
            Start(0);
        }
    }
}

size_t AudioPlayer::Frames() const
{
    return m_decoded && m_decoded->channels ? m_decoded->pcm.size() / m_decoded->channels : 0;
}

size_t AudioPlayer::CurrentFrame() const
{
    size_t frames = Frames();

    if (!m_device || !frames)
    {
        return m_cursor;
    }

    MMTIME time = {};
    time.wType = TIME_SAMPLES;

    if (waveOutGetPosition(m_device->handle, &time, sizeof(time)) != MMSYSERR_NOERROR || time.wType != TIME_SAMPLES)
    {
        return m_startFrame;
    }

    size_t frame = m_startFrame + time.u.sample;
    return m_device->header.dwLoops > 1 ? frame % frames : std::min(frame, frames);
}

double AudioPlayer::Position() const
{
    uint32_t rate = SampleRate();
    return rate ? static_cast<double>(CurrentFrame()) / rate : 0.0;
}

double AudioPlayer::Duration() const
{
    uint32_t rate = SampleRate();
    return rate ? static_cast<double>(Frames()) / rate : 0.0;
}

const std::string& AudioPlayer::Error() const
{
    return m_decoded ? m_decoded->error : kNoError;
}

const std::vector<float>& AudioPlayer::Peaks() const
{
    return m_decoded ? m_decoded->peaks : kNoPeaks;
}

void AudioPlayer::Start(size_t frame)
{
    Close();
    size_t frames = Frames();

    if (!Ready() || frame >= frames)
    {
        m_cursor = 0;
        return;
    }

    uint32_t channels = m_decoded->channels;
    WAVEFORMATEX format = {};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = static_cast<WORD>(channels);
    format.nSamplesPerSec = m_decoded->sampleRate;
    format.wBitsPerSample = 16;
    format.nBlockAlign = static_cast<WORD>(channels * 2);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

    auto device = std::make_unique<Device>();

    if (waveOutOpen(&device->handle, WAVE_MAPPER, &format, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
    {
        m_decoded->error = "No audio output device could be opened.";
        return;
    }

    device->header.lpData = reinterpret_cast<LPSTR>(m_decoded->pcm.data() + frame * channels);
    device->header.dwBufferLength = static_cast<DWORD>((frames - frame) * channels * 2);

    if (m_loop && frame == 0)
    {
        device->header.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
        device->header.dwLoops = 0xFFFFFFFF;
    }

    waveOutPrepareHeader(device->handle, &device->header, sizeof(WAVEHDR));
    DWORD level = static_cast<DWORD>(std::clamp(m_volume, 0.0f, 1.0f) * 0xFFFF);
    waveOutSetVolume(device->handle, level | (level << 16));
    waveOutWrite(device->handle, &device->header, sizeof(WAVEHDR));
    m_device = std::move(device);
    m_startFrame = frame;
}

void AudioPlayer::Close()
{
    if (!m_device)
    {
        return;
    }

    waveOutReset(m_device->handle);
    waveOutUnprepareHeader(m_device->handle, &m_device->header, sizeof(WAVEHDR));
    waveOutClose(m_device->handle);
    m_device.reset();
}

void AudioPlayer::Play()
{
    if (m_pending)
    {
        m_autoPlay = true;
        return;
    }

    if (!m_device)
    {
        Start(m_cursor);
    }
}

void AudioPlayer::Pause()
{
    m_autoPlay = false;

    if (m_device)
    {
        m_cursor = CurrentFrame();
        Close();
    }
}

void AudioPlayer::Toggle()
{
    Playing() ? Pause() : Play();
}

void AudioPlayer::Stop()
{
    m_autoPlay = false;
    Close();
    m_cursor = 0;
}

void AudioPlayer::Seek(double seconds)
{
    uint32_t rate = SampleRate();
    size_t frames = Frames();

    if (!rate || !frames)
    {
        return;
    }

    size_t frame = std::min(static_cast<size_t>(std::max(0.0, seconds) * rate), frames - 1);

    if (m_device)
    {
        Start(frame);
    }
    else
    {
        m_cursor = frame;
    }
}

void AudioPlayer::SetLoop(bool loop)
{
    if (m_loop == loop)
    {
        return;
    }

    m_loop = loop;

    if (m_device)
    {
        Start(CurrentFrame());
    }
}

void AudioPlayer::SetVolume(float volume)
{
    m_volume = std::clamp(volume, 0.0f, 1.0f);

    if (m_device)
    {
        DWORD level = static_cast<DWORD>(m_volume * 0xFFFF);
        waveOutSetVolume(m_device->handle, level | (level << 16));
    }
}

}
