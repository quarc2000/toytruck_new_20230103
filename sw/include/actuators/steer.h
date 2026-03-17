// hearderfile for Steering control

#ifndef STEER_H
#define STEER_H

#include <actuators/motor.h>

class Steer
{
public:
  Steer(); // declare default constructor with controlpin input
  void Begin();
  void direction(int direction);
  void nudgeNeutralTrim(int delta);

private:
  Motor motor;
  motorType_t motorType;
  steerType_t steerType;
  bool begun = false;
  boolean servoReverse = false;
  int steer_servo_min = 0;
  int steer_servo_max = 0;
  int steer_servo_adjust = 0;
  int steer_neutral_trim = 0;

public:
  void Right();
  void Left();
  void Straight();
  void Stop();  //Depreciated, Stop is not relevant with Steer, use Straight indr´tead, or direction(0);
};

#endif
