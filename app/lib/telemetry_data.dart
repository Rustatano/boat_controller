class TelemetryData {
  final int speed;
  final int temperature;
  final bool waterLeak;
  // Gyroscope values (angle of tilt on X, Y, Z)

  TelemetryData({required this.speed, required this.temperature, required this.waterLeak});

  factory TelemetryData.fromJson(Map<String, dynamic> json) {
    return TelemetryData(
      speed: (json['speed'] as num).toInt(),
      temperature: (json['temp'] as num).toInt(),
      waterLeak: json['leak'] as bool,
    );
  }

  @override
  String toString() {
    return 'Speed:\t${this.speed} m/s\nTemperature:\t${this.temperature} °C\nWater leak:\t${this.waterLeak! ? 'true' : 'false'}';
  }
}