class TelemetryData {
  double? speed;
  double? temperature;
  bool? waterLeak;
  // Gyroscope values (angle of tilt on X, Y, Z)

  // Constructor
  TelemetryData();

  @override
  String toString() {
    return 'Speed:\t${this.speed} m/s\nTemperature:\t${this.temperature} °C\nWater leak:\t${this.waterLeak! ? 'true' : 'false'}';
  }
}