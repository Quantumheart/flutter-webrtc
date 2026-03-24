#include "flutter_screen_capture.h"

#include <stdexcept>

#include "rtc_audio_source.h"
#include "rtc_audio_track.h"

namespace flutter_webrtc_plugin {

FlutterScreenCapture::FlutterScreenCapture(FlutterWebRTCBase* base)
    : base_(base) {}

bool FlutterScreenCapture::BuildDesktopSourcesList(const EncodableList& types,
                                                   bool force_reload) {
  size_t size = types.size();
  sources_.clear();
  for (size_t i = 0; i < size; i++) {
    std::string type_str = GetValue<std::string>(types[i]);
    DesktopType desktop_type = DesktopType::kScreen;
    if (type_str == "screen") {
      desktop_type = DesktopType::kScreen;
    } else if (type_str == "window") {
      desktop_type = DesktopType::kWindow;
    } else {
      return false;
    }
    scoped_refptr<RTCDesktopMediaList> source_list;
    auto it = medialist_.find(desktop_type);
    if (it != medialist_.end()) {
      source_list = (*it).second;
    } else {
      source_list = base_->desktop_device_->GetDesktopMediaList(desktop_type);
      source_list->RegisterMediaListObserver(this);
      medialist_[desktop_type] = source_list;
    }
#ifdef __linux__
    try {
      source_list->UpdateSourceList(force_reload, false);
    } catch (...) {
      continue;
    }
#else
    source_list->UpdateSourceList(force_reload, false);
#endif
    int count = source_list->GetSourceCount();
    for (int j = 0; j < count; j++) {
      sources_.push_back(source_list->GetSource(j));
    }
  }
  return true;
}

void FlutterScreenCapture::GetDesktopSources(
    const EncodableList& types,
    std::unique_ptr<MethodResultProxy> result) {
  if (!BuildDesktopSourcesList(types, true)) {
    result->Error("Bad Arguments", "Failed to get desktop sources");
    return;
  }

  EncodableList sources;
  for (auto source : sources_) {
    EncodableMap info;
    info[EncodableValue("id")] = EncodableValue(source->id().std_string());
    info[EncodableValue("name")] = EncodableValue(source->name().std_string());
    info[EncodableValue("type")] =
        EncodableValue(source->type() == kWindow ? "window" : "screen");
    // TODO "thumbnailSize"
    info[EncodableValue("thumbnailSize")] = EncodableMap{
        {EncodableValue("width"), EncodableValue(0)},
        {EncodableValue("height"), EncodableValue(0)},
    };
    sources.push_back(EncodableValue(info));
  }

  auto map = EncodableMap();
  map[EncodableValue("sources")] = sources;
  result->Success(EncodableValue(map));
}

void FlutterScreenCapture::UpdateDesktopSources(
    const EncodableList& types,
    std::unique_ptr<MethodResultProxy> result) {
  if (!BuildDesktopSourcesList(types, false)) {
    result->Error("Bad Arguments", "Failed to update desktop sources");
    return;
  }
  auto map = EncodableMap();
  map[EncodableValue("result")] = true;
  result->Success(EncodableValue(map));
}

void FlutterScreenCapture::OnMediaSourceAdded(
    scoped_refptr<MediaSource> source) {
  EncodableMap info;
  info[EncodableValue("event")] = "desktopSourceAdded";
  info[EncodableValue("id")] = EncodableValue(source->id().std_string());
  info[EncodableValue("name")] = EncodableValue(source->name().std_string());
  info[EncodableValue("type")] =
      EncodableValue(source->type() == kWindow ? "window" : "screen");
  // TODO "thumbnailSize"
  info[EncodableValue("thumbnailSize")] = EncodableMap{
      {EncodableValue("width"), EncodableValue(0)},
      {EncodableValue("height"), EncodableValue(0)},
  };
  base_->event_channel()->Success(EncodableValue(info));
}

void FlutterScreenCapture::OnMediaSourceRemoved(
    scoped_refptr<MediaSource> source) {
  EncodableMap info;
  info[EncodableValue("event")] = "desktopSourceRemoved";
  info[EncodableValue("id")] = EncodableValue(source->id().std_string());
  base_->event_channel()->Success(EncodableValue(info));
}

void FlutterScreenCapture::OnMediaSourceNameChanged(
    scoped_refptr<MediaSource> source) {
  EncodableMap info;
  info[EncodableValue("event")] = "desktopSourceNameChanged";
  info[EncodableValue("id")] = EncodableValue(source->id().std_string());
  info[EncodableValue("name")] = EncodableValue(source->name().std_string());
  base_->event_channel()->Success(EncodableValue(info));
}

void FlutterScreenCapture::OnMediaSourceThumbnailChanged(
    scoped_refptr<MediaSource> source) {
  EncodableMap info;
  info[EncodableValue("event")] = "desktopSourceThumbnailChanged";
  info[EncodableValue("id")] = EncodableValue(source->id().std_string());
  info[EncodableValue("thumbnail")] =
      EncodableValue(source->thumbnail().std_vector());
  base_->event_channel()->Success(EncodableValue(info));
}

void FlutterScreenCapture::OnStart(scoped_refptr<RTCDesktopCapturer> capturer) {
}

void FlutterScreenCapture::OnPaused(
    scoped_refptr<RTCDesktopCapturer> capturer) {
}

void FlutterScreenCapture::OnStop(scoped_refptr<RTCDesktopCapturer> capturer) {
}

void FlutterScreenCapture::OnError(scoped_refptr<RTCDesktopCapturer> capturer) {
}

void FlutterScreenCapture::GetDesktopSourceThumbnail(
    std::string source_id,
    int width,
    int height,
    std::unique_ptr<MethodResultProxy> result) {
  scoped_refptr<MediaSource> source;
  for (auto src : sources_) {
    if (src->id().std_string() == source_id) {
      source = src;
    }
  }
  if (source.get() == nullptr) {
    result->Error("Bad Arguments", "Failed to get desktop source thumbnail");
    return;
  }
  source->UpdateThumbnail();
  result->Success(EncodableValue(source->thumbnail().std_vector()));
}

void FlutterScreenCapture::GetDisplayMedia(
    const EncodableMap& constraints,
    std::unique_ptr<MethodResultProxy> result) {
  std::string source_id = "0";
  // DesktopType source_type = kScreen;
  double fps = 30.0;

  const EncodableMap video = findMap(constraints, "video");
  if (video != EncodableMap()) {
    const EncodableMap deviceId = findMap(video, "deviceId");
    if (deviceId != EncodableMap()) {
      source_id = findString(deviceId, "exact");
      if (source_id.empty()) {
        result->Error("Bad Arguments", "Incorrect video->deviceId->exact");
        return;
      }
      if (source_id != "0") {
        // source_type = DesktopType::kWindow;
      }
    }
    const EncodableMap mandatory = findMap(video, "mandatory");
    if (mandatory != EncodableMap()) {
      double frameRate = findDouble(mandatory, "frameRate");
      if (frameRate != 0.0) {
        fps = frameRate;
      }
    }
  }

  std::string uuid = base_->GenerateUUID();

  scoped_refptr<RTCMediaStream> stream =
      base_->factory_->CreateStream(uuid.c_str());

  EncodableMap params;
  params[EncodableValue("streamId")] = EncodableValue(uuid);

  // VIDEO

  EncodableMap video_constraints;
  auto it = constraints.find(EncodableValue("video"));
  if (it != constraints.end() && TypeIs<EncodableMap>(it->second)) {
    video_constraints = GetValue<EncodableMap>(it->second);
  }

  scoped_refptr<MediaSource> source;
  for (auto src : sources_) {
    if (src->id().std_string() == source_id) {
      source = src;
    }
  }

#ifdef __linux__
  if (!source.get() && !sources_.empty()) {
    source = sources_.front();
  }
  if (!source.get()) {
    EncodableList types;
    types.push_back(EncodableValue(std::string("screen")));
    BuildDesktopSourcesList(types, true);
    for (auto src : sources_) {
      if (src->id().std_string() == source_id) {
        source = src;
      }
    }
    if (!source.get() && !sources_.empty()) {
      source = sources_.front();
    }
  }
#endif

  if (!source.get()) {
    result->Error("Bad Arguments", "source not found!");
    return;
  }

  scoped_refptr<RTCDesktopCapturer> desktop_capturer =
      base_->desktop_device_->CreateDesktopCapturer(source);

  if (!desktop_capturer.get()) {
    result->Error("Bad Arguments", "CreateDesktopCapturer failed!");
    return;
  }

  desktop_capturer->RegisterDesktopCapturerObserver(this);

  const char* video_source_label = "screen_capture_input";

  scoped_refptr<RTCVideoSource> video_source =
      base_->factory_->CreateDesktopSource(
          desktop_capturer, video_source_label,
          base_->ParseMediaConstraints(video_constraints));

  scoped_refptr<RTCVideoTrack> video_track =
      base_->factory_->CreateVideoTrack(video_source, uuid.c_str());

  EncodableList videoTracks;
  EncodableMap video_info;
  video_info[EncodableValue("id")] = EncodableValue(video_track->id().std_string());
  video_info[EncodableValue("label")] = EncodableValue(video_track->id().std_string());
  video_info[EncodableValue("kind")] = EncodableValue(video_track->kind().std_string());
  video_info[EncodableValue("enabled")] = EncodableValue(video_track->enabled());
  videoTracks.push_back(EncodableValue(video_info));
  params[EncodableValue("videoTracks")] = EncodableValue(videoTracks);

  stream->AddTrack(video_track);
  base_->local_tracks_[video_track->id().std_string()] = video_track;

  ScreenShareSession session;
  session.desktop_capturer = desktop_capturer;
  session.video_track_id = video_track->id().std_string();

  // AUDIO

#ifdef __linux__
  std::string audio_uuid = base_->GenerateUUID();

  scoped_refptr<RTCAudioSource> audio_source =
      base_->factory_->CreateAudioSource(
          "screen_audio", RTCAudioSource::kCustom);

  scoped_refptr<RTCAudioTrack> audio_track =
      base_->factory_->CreateAudioTrack(audio_source, audio_uuid.c_str());

  auto pulse_capturer = std::make_unique<PulseAudioCapturer>(
      [audio_source](const void* data, size_t frames) {
        audio_source->CaptureFrame(data, 16, 48000, 1, frames);
      },
      48000, 1);

  if (pulse_capturer->Start()) {
    EncodableList audioTracks;
    EncodableMap audio_info;
    audio_info[EncodableValue("id")] = EncodableValue(audio_track->id().std_string());
    audio_info[EncodableValue("label")] = EncodableValue(audio_track->id().std_string());
    audio_info[EncodableValue("kind")] = EncodableValue(audio_track->kind().std_string());
    audio_info[EncodableValue("enabled")] = EncodableValue(audio_track->enabled());
    audioTracks.push_back(EncodableValue(audio_info));
    params[EncodableValue("audioTracks")] = EncodableValue(audioTracks);

    stream->AddTrack(audio_track);
    base_->local_tracks_[audio_track->id().std_string()] = audio_track;

    session.audio_track_id = audio_track->id().std_string();
    session.audio_source = audio_source;
    session.pulse_capturer = std::move(pulse_capturer);
  } else {
    params[EncodableValue("audioTracks")] = EncodableValue(EncodableList());
  }
#else
  params[EncodableValue("audioTracks")] = EncodableValue(EncodableList());
#endif

  std::string video_tid = session.video_track_id;
  std::string audio_tid = session.audio_track_id;
  active_sessions_[video_tid] = std::move(session);

  base_->screen_share_cleanups_[video_tid] = [this, video_tid]() {
    StopScreenShare(video_tid);
  };
  if (!audio_tid.empty()) {
    base_->screen_share_cleanups_[audio_tid] = [this, video_tid]() {
      StopScreenShare(video_tid);
    };
  }

  base_->local_streams_[uuid] = stream;

  desktop_capturer->Start(uint32_t(fps));

  result->Success(EncodableValue(params));
}

void FlutterScreenCapture::StopScreenShare(const std::string& track_id) {
  auto it = active_sessions_.find(track_id);
  if (it == active_sessions_.end()) return;

  auto& session = it->second;

#ifdef __linux__
  if (session.pulse_capturer) {
    session.pulse_capturer->Stop();
  }
#endif

  if (session.desktop_capturer && session.desktop_capturer->IsRunning()) {
    session.desktop_capturer->Stop();
  }

  base_->screen_share_cleanups_.erase(session.video_track_id);
  base_->screen_share_cleanups_.erase(session.audio_track_id);

  active_sessions_.erase(it);
}

}  // namespace flutter_webrtc_plugin
