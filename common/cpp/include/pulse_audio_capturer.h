#ifndef PULSE_AUDIO_CAPTURER_HXX
#define PULSE_AUDIO_CAPTURER_HXX

#ifdef __linux__

#include <atomic>
#include <functional>
#include <memory>
#include <thread>

namespace flutter_webrtc_plugin {

class PulseAudioCapturer {
 public:
  using AudioCallback =
      std::function<void(const void* data, size_t frames)>;

  PulseAudioCapturer(AudioCallback callback,
                     int sample_rate = 48000,
                     int channels = 1);
  ~PulseAudioCapturer();

  bool Start();
  void Stop();
  bool IsRunning() const { return running_.load(); }

  int sample_rate() const { return sample_rate_; }
  int channels() const { return channels_; }

 private:
  void CaptureLoop();

  AudioCallback callback_;
  int sample_rate_;
  int channels_;
  std::atomic<bool> running_{false};
  std::unique_ptr<std::thread> capture_thread_;
};

}  // namespace flutter_webrtc_plugin

#endif  // __linux__
#endif  // PULSE_AUDIO_CAPTURER_HXX
