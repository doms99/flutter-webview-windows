#pragma once

#include <flutter/event_channel.h>
#include <flutter/method_channel.h>
#include <flutter/standard_method_codec.h>
#include <flutter/texture_registrar.h>

#include <memory>

#include "graphics_context.h"
#include "texture_bridge_gpu.h"

class VisualBridge {
 public:
  VisualBridge(flutter::BinaryMessenger* messenger,
               flutter::TextureRegistrar* texture_registrar,
               GraphicsContext* graphics_context,
               ABI::Windows::UI::Composition::IVisual* visual);
  ~VisualBridge();

  int64_t texture_id() const { return texture_id_; }
  TextureBridge* texture_bridge() const { return texture_bridge_.get(); }

 private:
  std::unique_ptr<flutter::TextureVariant> flutter_texture_;
  std::unique_ptr<TextureBridgeGpu> texture_bridge_;

  std::unique_ptr<flutter::MethodChannel<flutter::EncodableValue>>
      method_channel_;

  flutter::TextureRegistrar* texture_registrar_;
  int64_t texture_id_ = 0;

  winrt::com_ptr<ABI::Windows::UI::Composition::IVisual> root_visual_;

  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
};
