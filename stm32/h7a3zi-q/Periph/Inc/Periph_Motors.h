/*
 * Periph_Motors.h
 *
 *  Created on: Feb 27, 2024
 *      Author: Charles Henri
 */

#ifndef INC_PERIPH_MOTORS_H_
#define INC_PERIPH_MOTORS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define MOTOR_AK10_9 0
#define MOTOR_AK80_64 1

typedef void (*SendCanDataFunction)(uint32_t id, uint8_t *data);

typedef struct
{
  float positionMin;
  float positionMax;

  float velocityMin;
  float velocityMax;

  float torqueMin;
  float torqueMax;

  float kpMin;
  float kpMax;

  float kdMin;
  float kdMax;
} MotorParameters;

typedef struct
{
  uint8_t id;
  float position;
  float velocity;
  float torque;
  MotorParameters parameters;
} Motor;

void PeriphMotors_Init(SendCanDataFunction sendCanFunc);
bool PeriphMotors_InitMotor(Motor *pMotor, uint8_t id, uint8_t model);
void PeriphMotors_Enable(Motor *pMotor);
void PeriphMotors_SubscribeToRx(Motor *pMotor);
void PeriphMotors_Disable(Motor *pMotor);
void PeriphMotors_SetZeroPosition(Motor *pMotor);
void PeriphMotors_Move(Motor *pMotor, float position, float velocity, float torque, float kp, float kd);
void PeriphMotors_ParseMotorState(Motor *pMotor, uint8_t *canData);


#ifdef __cplusplus
}
#endif

#endif /* INC_PERIPH_MOTORS_H_ */
