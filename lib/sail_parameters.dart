class SailParameters {
  DateTime? time;
  double? speed;
  double? heatSensor1;
  double? heatSensor2;
  // Gyroscope values (angle of tilt on X, Y, Z)

  // Constructor
  SailParameters(this.time);

  @override
  String toString() {
    return 'Time:\t${this.time}\nSpeed:\t${this.speed} m/s\nHeat Sensor 1:\t${this.heatSensor1} °C\nHeat Sensor 2:\t${this.heatSensor2} °C';
  }
}
