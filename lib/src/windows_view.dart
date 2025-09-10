import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';
import 'package:flutter/widgets.dart';

class WindowsViewController extends ChangeNotifier {
  static const MethodChannel _pluginChannel = MethodChannel('io.jns.webview.win');

  int? _textureId;
  late MethodChannel _methodChannel;

  bool get isInitialized => _textureId != null;
  int get textureId => _textureId!;

  Future<void> initialize() async {
    final reply = await _pluginChannel.invokeMapMethod<String, dynamic>('initializeVisual');
    _textureId = reply!['textureId'] as int;
    _methodChannel = MethodChannel('io.jns.webview.win/visual/$_textureId');
    notifyListeners();
  }

  Future<void> setFpsLimit(int? maxFps) async {
    if (!isInitialized) return;
    return _methodChannel.invokeMethod('setFpsLimit', maxFps ?? 0);
  }

  Future<void> setSize(Size size, double scaleFactor) async {
    if (!isInitialized) return;
    return _methodChannel.invokeMethod('setSize', [size.width, size.height, scaleFactor]);
  }

  Future<void> disposeView() async {
    if (!isInitialized) return;
    await _pluginChannel.invokeMethod('disposeVisual', _textureId);
    _textureId = null;
  }
}

class WindowsView extends StatelessWidget {
  final WindowsViewController controller;
  final double? width;
  final double? height;
  final double? scaleFactor;
  final FilterQuality filterQuality;

  const WindowsView(this.controller,
      {Key? key, this.width, this.height, this.scaleFactor, this.filterQuality = FilterQuality.none})
      : super(key: key);

  @override
  Widget build(BuildContext context) {
    if (!controller.isInitialized) {
      return const SizedBox.shrink();
    }
    final texture = Texture(textureId: controller.textureId, filterQuality: filterQuality);
    if (width != null || height != null) {
      return SizedBox(
        width: width,
        height: height,
        child: texture,
      );
    }
    return texture;
  }
}
