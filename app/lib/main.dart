import 'package:boat_controller/control_page.dart';
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

  runApp(const BoatControllerApp());
}

class BoatControllerApp extends StatelessWidget {
  const BoatControllerApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Boat Controller',
      theme: ThemeData(
        colorScheme: .fromSeed(
          seedColor: const Color.fromARGB(255, 65, 109, 223),
        ),
      ),
      home: const ControlPage(title: 'Home'),
    );
  }
}
