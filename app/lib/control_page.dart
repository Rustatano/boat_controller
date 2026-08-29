import 'package:boat_controller/sail_parameters.dart';
import 'package:flutter/material.dart';
import 'package:flutter_mjpeg/flutter_mjpeg.dart';

class ControlPage extends StatefulWidget {
  const ControlPage({super.key, required this.title});

  final String title;

  @override
  State<ControlPage> createState() => _ControlPageState();
}

class _ControlPageState extends State<ControlPage> {
  double _currentThrottle = 0; // from 0 to 100 %
  double _currentTurn = 0; // grom -45 to 45 °

  SailParameters sailParameters = SailParameters(DateTime.now());

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
                            onChanged: (double value) {
                              setState(() {
                                _currentThrottle = value;
                              });
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
                    Text(
                      sailParameters.toString(),
                      style: TextStyle(color: Colors.black),
                    ),
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
                          _currentTurn.toStringAsFixed(2),
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
                            value: _currentTurn,
                            max: 45,
                            min: -45,
                            onChanged: (double value) {
                              setState(() {
                                _currentTurn = value;
                              });
                            },
                            onChangeEnd: (value) {
                              // snap to zero when tap ended
                              setState(() {
                                _currentTurn = 0;
                              });
                            },
                          ),
                        ),
                      ),
                      // reset turn button
                      IconButton(
                        onPressed: () {
                          setState(() {
                            _currentTurn = 0;
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
