import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();

  // Force landscape mode
  await SystemChrome.setPreferredOrientations([
    DeviceOrientation.landscapeLeft, // Leftside landscape mode
  ]);
  
  // Hide status bar
  await SystemChrome.setEnabledSystemUIMode(SystemUiMode.immersive);

  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Boat Controller',
      theme: ThemeData(colorScheme: .fromSeed(seedColor: Colors.deepPurple)),
      home: const MyHomePage(title: 'Home'),
    );
  }
}

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key, required this.title});

  final String title;

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  double _currentThrottle = 0; // from 0 to 100 %
  double _currentTurn = 0; // from -45 to 45 °

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Center(
        child: Row(
          mainAxisAlignment: .center,
          children: [
            Padding(
              padding: const EdgeInsets.all(10.0),
              child: Column(
                children: [
                  Text(_currentThrottle.toStringAsFixed(2)),
                  Expanded(
                    child: RotatedBox(
                      quarterTurns: 3,
                      child: Slider(
                        // Throttle slider
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
                ],
              ),
            ),
            Expanded(child: Container(color: Colors.blue)),
            Padding(
              padding: const EdgeInsets.all(10.0),
              child: Column(
                mainAxisAlignment: .center,
                children: [
                  Text(_currentTurn.toStringAsFixed(2)),
                  Slider(
                    // Turn slider
                    // ignore: deprecated_member_use
                    year2023: false,
                    value: _currentTurn,
                    min: -45,
                    max: 45,
                    onChanged: (double value) {
                      setState(() {
                        _currentTurn = value;
                      });
                    },
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }
}
