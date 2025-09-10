#include "visual_bridge.h"

#include <flutter/method_result_functions.h>

namespace {
constexpr auto kMethodSetFpsLimit = "setFpsLimit";
constexpr auto kMethodSetSize = "setSize"; // [width, height, scale]
}

VisualBridge::VisualBridge(flutter::BinaryMessenger* messenger,
                           flutter::TextureRegistrar* texture_registrar,
                           GraphicsContext* graphics_context,
                           ABI::Windows::UI::Composition::IVisual* visual)
    : texture_registrar_(texture_registrar) {
  root_visual_ = visual;
  texture_bridge_ = std::make_unique<TextureBridgeGpu>(graphics_context, visual);

  flutter_texture_ = std::make_unique<flutter::TextureVariant>(
      flutter::GpuSurfaceTexture(
          kFlutterDesktopGpuSurfaceTypeDxgiSharedHandle,
          [bridge = texture_bridge_.get()](size_t width, size_t height)
              -> const FlutterDesktopGpuSurfaceDescriptor* {
            return static_cast<TextureBridgeGpu*>(bridge)->GetSurfaceDescriptor(
                width, height);
          }));

  texture_id_ = texture_registrar_->RegisterTexture(flutter_texture_.get());
  texture_bridge_->SetOnFrameAvailable([this]() {
    texture_registrar_->MarkTextureFrameAvailable(texture_id_);
  });

  const auto method_channel_name =
      std::format("io.jns.webview.win/visual/{}", texture_id_);
  method_channel_ =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          messenger, method_channel_name,
          &flutter::StandardMethodCodec::GetInstance());
  method_channel_->SetMethodCallHandler(
      [this](const auto& call, auto result) {
        HandleMethodCall(call, std::move(result));
      });

  // Start capture immediately so frames are produced.
  texture_bridge_->Start();
}

VisualBridge::~VisualBridge() {
  if (method_channel_) {
    method_channel_->SetMethodCallHandler(nullptr);
  }
  if (texture_registrar_ && texture_id_ != 0) {
    texture_registrar_->UnregisterTexture(texture_id_);
  }
}

void VisualBridge::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  if (method_call.method_name().compare(kMethodSetFpsLimit) == 0) {
    if (const auto max_fps = std::get_if<int>(method_call.arguments())) {
      texture_bridge_->SetFpsLimit(*max_fps > 0 ? std::optional<int>(*max_fps)
                                                : std::nullopt);
      return result->Success();
    }
    return result->Error("invalidArguments");
  }

  if (method_call.method_name().compare(kMethodSetSize) == 0) {
    const auto args = std::get_if<flutter::EncodableList>(method_call.arguments());
    if (!args || args->size() < 2) {
      return result->Error("invalidArguments");
    }
    // Update visual size to match widget, then notify capture to recreate buffers.
    const auto w = std::get_if<double>(&(*args)[0]);
    const auto h = std::get_if<double>(&(*args)[1]);
    if (w && h && root_visual_) {
      root_visual_->put_Size({static_cast<float>(*w), static_cast<float>(*h)});
    }
    texture_bridge_->NotifySurfaceSizeChanged();
    return result->Success();
  }

  result->NotImplemented();
}
