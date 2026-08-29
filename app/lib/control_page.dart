import 'dart:convert';

import 'package:flutter/material.dart';
import 'package:flutter_mjpeg/flutter_mjpeg.dart';
import 'package:web_socket_channel/web_socket_channel.dart';

import 'package:boat_controller/telemetry_data.dart';

class ControlPage extends StatefulWidget {
  const ControlPage({super.key, required this.title});

  final String title;

  @override
  State<ControlPage> createState() => _ControlPageState();
}

class _ControlPageState extends State<ControlPage> {
  double _currentThrottle = 0; // -100 - 100 percent
  double _currentRudderAngle = 90; // 0 - 180 degrees
  WebSocketChannel? _wsChannel;
  TelemetryData? _latestTelemetry;

  TelemetryData telemetryData = TelemetryData(
    speed: 0,
    temperature: 0,
    waterLeak: false,
  );

  void connect(Function(TelemetryData) onTelemetryReceived) {
    _wsChannel = WebSocketChannel.connect(Uri.parse('ws://192.168.4.1/ws'));

    // listening to messages from esp
    _wsChannel!.stream.listen(
      (msg) {
        try {
          final Map<String, dynamic> data = jsonDecode(msg);

          final telemetry = TelemetryData.fromJson(data);
          // send data to UI
          onTelemetryReceived(telemetry);
        } catch (err) {
          print('ERR: $err');
        }
      },
      onError: (err) => print('ERR: $err'),
      onDone: () => print('LOG: websocket connection ended'),
    );
  }

  void sendBoatControl(int throttle, int rudderAngle) {
    if (_wsChannel != null) {
      // create map
      Map<String, dynamic> payload = {
        'throttle': throttle,
        'rudder_angle': rudderAngle,
      };

      // convert map to json
      String jsonString = jsonEncode(payload);

      // sending over websocket
      _wsChannel!.sink.add(jsonString);
    }
  }

  void disconnect() {
    _wsChannel?.sink.close();
  }

  @override
  void initState() {
    super.initState();
    connect((telemetry) {
      setState(() {
        _latestTelemetry = telemetry;
      });
    });
  }

  @override
  Widget build(BuildContext context) {
    final screenWidth = MediaQuery.sizeOf(context).width;

    return Scaffold(
      body: Center(
        child: Stack(
          children: [
            // on-board camera stream
            Positioned.fill(
              child: Mjpeg(
                isLive: true,
                error: (context, error, stack) {
                  return Text(
                    error.toString(),
                    style: TextStyle(color: Colors.red),
                  );
                },
                stream: 'http://192.168.4.1/stream',
              ),
            ),
            Row(
              mainAxisAlignment: .center,
              children: [
                // throttle slider column
                Padding(
                  padding: const EdgeInsets.all(16.0),
                  child: Column(
                    children: [
                      SizedBox(
                        width: screenWidth / 12,
                        child: Text(
                          textAlign: TextAlign.center,
                          _currentThrottle.toStringAsFixed(2),
                          style: TextStyle(fontSize: 18),
                        ),
                      ),
                      Expanded(
                        child: RotatedBox(
                          quarterTurns: 3,
                          child: Slider(
                            // throttle slider
                            // ignore: deprecated_member_use
                            year2023: false,
                            value: _currentThrottle,
                            max: 100,
                            min: -100,
                            onChanged: (double value) {
                              setState(() {
                                _currentThrottle = value;
                              });
                              sendBoatControl(
                                _currentThrottle.round(),
                                _currentRudderAngle.round(),
                              );
                            },
                          ),
                        ),
                      ),
                      // reset throttle button
                      IconButton(
                        onPressed: () {
                          setState(() {
                            _currentThrottle = 0;
                          });
                        },
                        icon: Icon(Icons.replay),
                      ),
                    ],
                  ),
                ),
                // parameters display (sensors, time, ...)
                Column(
                  children: [
                    if (_latestTelemetry != null) ...[
                      Text(
                        _latestTelemetry.toString(),
                        style: TextStyle(color: Colors.black),
                      ),
                    ],
                  ],
                ),
                // central view
                Expanded(child: Container()),
                // turn slider column
                Padding(
                  padding: const EdgeInsets.all(10.0),
                  child: Column(
                    children: [
                      SizedBox(
                        width: screenWidth / 12,
                        child: Text(
                          textAlign: TextAlign.center,
                          _currentRudderAngle.toStringAsFixed(2),
                          style: TextStyle(fontSize: 18),
                        ),
                      ),
                      Expanded(
                        child: RotatedBox(
                          quarterTurns: 3,
                          child: Slider(
                            // turn slider
                            // ignore: deprecated_member_use
                            year2023: false,
                            value: _currentRudderAngle,
                            max: 180,
                            min: 0,
                            onChanged: (double value) {
                              setState(() {
                                _currentRudderAngle = value;
                              });
                              sendBoatControl(
                                _currentThrottle.round(),
                                _currentRudderAngle.round(),
                              );
                            },
                          ),
                        ),
                      ),
                      // reset turn button
                      IconButton(
                        onPressed: () {
                          setState(() {
                            _currentRudderAngle = 90;
                          });
                        },
                        icon: Icon(Icons.replay),
                      ),
                    ],
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}
