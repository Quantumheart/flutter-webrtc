#ifdef __linux__

#include "pulse_audio_capturer.h"

#include <pulse/simple.h>
#include <pulse/error.h>

#include <cstdio>
#include <vector>

namespace flutter_webrtc_plugin {

PulseAudioCapturer::PulseAudioCapturer(AudioCallback callback,
                                       int sample_rate,
                                       int channels)
    : callback_(std::move(callback)),
      sample_rate_(sample_rate),
      channels_(channels) {}

PulseAudioCapturer::~PulseAudioCapturer() {
  Stop();
}

bool PulseAudioCapturer::Start() {
  if (running_.load()) return true;
  running_.store(true);
  capture_thread_ = std::make_unique<std::thread>(&PulseAudioCapturer::CaptureLoop, this);
  return true;
}

void PulseAudioCapturer::Stop() {
  running_.store(false);
  if (capture_thread_ && capture_thread_->joinable()) {
    capture_thread_->join();
  }
  capture_thread_.reset();
}

void PulseAudioCapturer::CaptureLoop() {
  pa_sample_spec spec{};
  spec.format = PA_SAMPLE_S16LE;
  spec.rate = static_cast<uint32_t>(sample_rate_);
  spec.channels = static_cast<uint8_t>(channels_);

  int error = 0;
  pa_simple* pa = pa_simple_new(
      nullptr,
      "Lattice",
      PA_STREAM_RECORD,
      "@DEFAULT_MONITOR@",
      "Screen Share Audio",
      &spec,
      nullptr,
      nullptr,
      &error);

  if (!pa) {
    fprintf(stderr, "[flutter_webrtc] PulseAudio open failed: %s\n",
            pa_strerror(error));
    running_.store(false);
    return;
  }

  const size_t frames_per_read = static_cast<size_t>(sample_rate_) / 100;
  const size_t bytes_per_read = frames_per_read * channels_ * sizeof(int16_t);
  std::vector<int16_t> buffer(frames_per_read * channels_);

  while (running_.load()) {
    if (pa_simple_read(pa, buffer.data(), bytes_per_read, &error) < 0) {
      fprintf(stderr, "[flutter_webrtc] PulseAudio read failed: %s\n",
              pa_strerror(error));
      break;
    }
    callback_(buffer.data(), frames_per_read);
  }

  pa_simple_free(pa);
}


}  // namespace flutter_webrtc_plugin

#endif  // __linux__
