#pragma once

#include "core/Bytes.hh"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace tk
{

enum EAudioPlayer : size_t
{
    kAudioPeaks = 512,
    kAudioHeaderBytes = 0x1000,
    kRsdNameOffset = 0x18,
    kRsdNameBytes = 0x8C,
};

struct AudioInfo
{
    std::string container;
    std::string codec;
    uint32_t channels = 0;
    uint32_t bits = 0;
    uint32_t sampleRate = 0;
    std::string originalName;
};

bool ReadAudioInfo(const Bytes& bytes, AudioInfo& out);

class AudioPlayer
{
public:
    AudioPlayer();
    ~AudioPlayer();
    AudioPlayer(const AudioPlayer&) = delete;
    AudioPlayer& operator=(const AudioPlayer&) = delete;

    void Load(uint32_t key, Bytes bytes, bool autoPlay);
    void Clear();
    void Tick();

    void Play();
    void Pause();
    void Toggle();
    void Stop();
    void Seek(double seconds);

    void SetLoop(bool loop);
    void SetVolume(float volume);

    uint32_t Key() const { return m_key; }
    bool Loop() const { return m_loop; }
    float Volume() const { return m_volume; }
    bool Decoding() const { return m_pending != nullptr; }
    bool Ready() const { return m_decoded && m_decoded->error.empty() && !m_decoded->pcm.empty(); }
    bool Playing() const { return m_device != nullptr; }
    double Position() const;
    double Duration() const;
    uint32_t Channels() const { return m_decoded ? m_decoded->channels : 0; }
    uint32_t SampleRate() const { return m_decoded ? m_decoded->sampleRate : 0; }
    const std::string& Error() const;
    const std::vector<float>& Peaks() const;

private:
    struct Decoded
    {
        std::atomic<bool> done{false};
        std::vector<int16_t> pcm;
        uint32_t channels = 0;
        uint32_t sampleRate = 0;
        std::vector<float> peaks;
        std::string error;
    };

    struct Device;

    size_t Frames() const;
    size_t CurrentFrame() const;
    void Start(size_t frame);
    void Close();

    uint32_t m_key = 0;
    std::shared_ptr<Decoded> m_pending;
    std::shared_ptr<Decoded> m_decoded;
    std::unique_ptr<Device> m_device;
    size_t m_cursor = 0;
    size_t m_startFrame = 0;
    bool m_autoPlay = false;
    bool m_loop = false;
    float m_volume = 1.0f;
};

}
