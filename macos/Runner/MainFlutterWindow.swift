import Cocoa
import FlutterMacOS
import MediaPlayer

class OsMediaControlsHandler: NSObject {
  private var channel: FlutterMethodChannel?

  static func register(with messenger: FlutterBinaryMessenger) {
    let handler = OsMediaControlsHandler()
    handler.channel = FlutterMethodChannel(
      name: "com.tramp/os_media_controls",
      binaryMessenger: messenger
    )
    handler.channel?.setMethodCallHandler(handler.handle)
    handler.setupRemoteCommands()
  }

  private func setupRemoteCommands() {
    let commandCenter = MPRemoteCommandCenter.shared()

    commandCenter.playCommand.addTarget { [weak self] _ in
      self?.sendMediaKey("play")
      return .success
    }
    commandCenter.pauseCommand.addTarget { [weak self] _ in
      self?.sendMediaKey("pause")
      return .success
    }
    commandCenter.togglePlayPauseCommand.addTarget { [weak self] _ in
      self?.sendMediaKey("toggle")
      return .success
    }
    commandCenter.nextTrackCommand.addTarget { [weak self] _ in
      self?.sendMediaKey("next")
      return .success
    }
    commandCenter.previousTrackCommand.addTarget { [weak self] _ in
      self?.sendMediaKey("previous")
      return .success
    }
    commandCenter.stopCommand.addTarget { [weak self] _ in
      self?.sendMediaKey("stop")
      return .success
    }
  }

  private func sendMediaKey(_ action: String) {
    channel?.invokeMethod("onMediaKey", arguments: ["action": action])
  }

  private func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
    switch call.method {
    case "updateState":
      guard let args = call.arguments as? [String: Any] else {
        result(FlutterError(code: "bad_args", message: "Expected map", details: nil))
        return
      }

      var info = [String: Any]()
      if let title = args["title"] as? String, !title.isEmpty {
        info[MPMediaItemPropertyTitle] = title
      }
      if let artist = args["artist"] as? String, !artist.isEmpty {
        info[MPMediaItemPropertyArtist] = artist
      }
      if let album = args["album"] as? String, !album.isEmpty {
        info[MPMediaItemPropertyAlbumTitle] = album
      }
      if let duration = args["duration"] as? Int, duration > 0 {
        info[MPMediaItemPropertyPlaybackDuration] = Double(duration) / 1000.0
      }
      if let position = args["position"] as? Int {
        info[MPNowPlayingInfoPropertyElapsedPlaybackTime] = Double(position) / 1000.0
      }
      if let playing = args["playing"] as? Bool {
        info[MPNowPlayingInfoPropertyPlaybackRate] = playing ? 1.0 : 0.0
      }

      MPNowPlayingInfoCenter.default().nowPlayingInfo = info
      result(nil)
    case "clearState":
      MPNowPlayingInfoCenter.default().nowPlayingInfo = nil
      result(nil)
    default:
      result(FlutterMethodNotImplemented)
    }
  }
}

class MainFlutterWindow: NSWindow {
  override func awakeFromNib() {
    let flutterViewController = FlutterViewController()
    let windowFrame = self.frame
    self.contentViewController = flutterViewController
    self.setFrame(windowFrame, display: true)

    RegisterGeneratedPlugins(registry: flutterViewController)

    OsMediaControlsHandler.register(with: flutterViewController.engine.binaryMessenger)

    super.awakeFromNib()
  }
}
