#ifndef FLUTTER_SCRREN_CAPTURE_HXX
#define FLUTTER_SCRREN_CAPTURE_HXX

#include "flutter_common.h"
#include "flutter_webrtc_base.h"

#include "rtc_desktop_capturer.h"
#include "rtc_desktop_media_list.h"

#ifdef __linux__
#include "pulse_audio_capturer.h"
#endif

namespace flutter_webrtc_plugin {

struct ScreenShareSession {
  scoped_refptr<RTCDesktopCapturer> desktop_capturer;
  std::string video_track_id;
  std::string audio_track_id;
#ifdef __linux__
  scoped_refptr<RTCAudioSource> audio_source;
  std::unique_ptr<PulseAudioCapturer> pulse_capturer;
#endif
};

class FlutterScreenCapture : public MediaListObserver,
                             public DesktopCapturerObserver {
 public:
  FlutterScreenCapture(FlutterWebRTCBase* base);

  void GetDisplayMedia(const EncodableMap& constraints,
                       std::unique_ptr<MethodResultProxy> result);

  void GetDesktopSources(const EncodableList& types,
                         std::unique_ptr<MethodResultProxy> result);

  void UpdateDesktopSources(const EncodableList& types,
                            std::unique_ptr<MethodResultProxy> result);

  void GetDesktopSourceThumbnail(std::string source_id,
                                 int width,
                                 int height,
                                 std::unique_ptr<MethodResultProxy> result);

  void StopScreenShare(const std::string& track_id);

 protected:
  void OnMediaSourceAdded(scoped_refptr<MediaSource> source) override;

  void OnMediaSourceRemoved(scoped_refptr<MediaSource> source) override;

  void OnMediaSourceNameChanged(scoped_refptr<MediaSource> source) override;

  void OnMediaSourceThumbnailChanged(
      scoped_refptr<MediaSource> source) override;

  void OnStart(scoped_refptr<RTCDesktopCapturer> capturer) override;

  void OnPaused(scoped_refptr<RTCDesktopCapturer> capturer) override;

  void OnStop(scoped_refptr<RTCDesktopCapturer> capturer) override;

  void OnError(scoped_refptr<RTCDesktopCapturer> capturer) override;

 private:
  bool BuildDesktopSourcesList(const EncodableList& types, bool force_reload);

 private:
  FlutterWebRTCBase* base_;
  std::map<DesktopType, scoped_refptr<RTCDesktopMediaList>> medialist_;
  std::vector<scoped_refptr<MediaSource>> sources_;
  std::map<std::string, ScreenShareSession> active_sessions_;
};

}  // namespace flutter_webrtc_plugin

#endif  // FLUTTER_SCRREN_CAPTURE_HXX