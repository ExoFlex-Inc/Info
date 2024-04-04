#include <Manager_HMI.h>
#include <Manager_Motor.h>
#include <Manager_Movement.h>
#include <Periph_UartRingBuf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"

#define SECTION_LENGTH 20
#define SECTION_NBR    30
#define BUF_LENGTH     50
#define M_HMI_TIMER    50
#define M_HMI_STRING_LENGTH 20
#define M_HMI_MODE_SECTION 0
#define M_HMI_ACTION_SECTION 1
#define M_HMI_CONTENT_SECTION 2
#define M_HMI_EXERCISE_SECTION_NBR 5
#define M_HMI_CONTENT_FIRST_EXERCISE_SECTION 6


static const Motor* motorsData[MMOT_MOTOR_NBR];
static uint32_t     timerMs = 0;
char                ParsedMsg[SECTION_NBR][SECTION_LENGTH];
char                buf[BUF_LENGTH];

void ManagerHMI_ReceiveJSON();
void ManagerHMI_SendJSON();
void ManagerHMI_SetMotorDataToString();
void ManagerHMI_ParseJson(char* msg, uint8_t maxlength, uint8_t* sectionNbr);
void ManagerHMI_ExecuteJson(uint8_t sectionNbr);
void ManagerHMI_ExecuteManualIncrement(char* cmd);
void ManagerHMI_ExecuteManualHoming(char* cmd);
void ManagerHMI_ExecutePlanCmd(char* cmd, uint8_t size);
void ManagerHMI_ExecuteControlCmd(char* cmd);

void ManagerHMI_GetStrMode(uint8_t index, char* str);
void ManagerHMI_GetStrAutoState(uint8_t index, char* str);
void ManagerHMI_GetStrHomingState(uint8_t index, char* str);

void ManagerHMI_Init()
{
    cJSON_InitHooks(NULL);
    PeriphUartRingBuf_Init();

    // Get motor data (const pointer : read-only)
    for (uint8_t i = 0; i < MMOT_MOTOR_NBR; i++)
    {
        motorsData[i] = ManagerMotor_GetMotorData(i);
    }

    // Get motor data (const pointer : read-only)
    for (uint8_t i = 0; i < BUF_LENGTH; i++)
    {
        buf[i] = 0;
    }
}

void ManagerHMI_Task()
{
    ManagerHMI_ReceiveJSON();

    if (HAL_GetTick() - timerMs >= M_HMI_TIMER)
    {
        ManagerHMI_SendJSON();

        timerMs = HAL_GetTick();
    }
}

void ManagerHMI_SendJSON()
{
    cJSON* root = cJSON_CreateObject();

    autoPlanInfo_t* pPlan = ManagerMovement_GetPlanData();

    char strMode[M_HMI_STRING_LENGTH];
    char strAutoState[M_HMI_STRING_LENGTH];
    char strHomingState[M_HMI_STRING_LENGTH];

    ManagerHMI_GetStrMode(ManagerMovement_GetState(), strMode);
    ManagerHMI_GetStrHomingState(pPlan->homingState, strAutoState);
    ManagerHMI_GetStrAutoState(pPlan->autoState, strHomingState);
    uint8_t rep = pPlan->exCount;
    uint8_t set = pPlan->repsCount;
    char* strErrorCode = "NoError";

    int positions[MMOT_MOTOR_NBR];
    int torques[MMOT_MOTOR_NBR];

    for (uint8_t i = 0; i < MMOT_MOTOR_NBR; i++)
	{
		positions[i]  = motorsData[i]->position;;
		torques[i] = motorsData[i]->torque;
	}

    // Add mode, exercise, repetitions, sets, and errorcode to the JSON object
	cJSON_AddStringToObject(root, "Mode", strMode);
	cJSON_AddStringToObject(root, "AutoState", strAutoState);
	cJSON_AddStringToObject(root, "HomingState", strHomingState);
	cJSON_AddNumberToObject(root, "Repetitions", rep);
	cJSON_AddNumberToObject(root, "Sets", set);
	cJSON_AddStringToObject(root, "ErrorCode", strErrorCode);

	cJSON* positionsArray = cJSON_CreateIntArray(positions, 3);
	cJSON* torquesArray = cJSON_CreateIntArray(torques, 3);
	cJSON_AddItemToObject(root, "Positions", positionsArray);
	cJSON_AddItemToObject(root, "Torques", torquesArray);


    // Print the JSON object
    char* jsonMessage = cJSON_PrintUnformatted(root);

    // Send JSON string over UART
    PeriphUartRingBuf_Send(jsonMessage, strlen(jsonMessage));

    free(jsonMessage);
    cJSON_Delete(root);
}

void ManagerHMI_ReceiveJSON()
{
    uint32_t size = 0;
    PeriphUartRingBuf_ReadJson(buf, &size);

    if (size > 0 && size < BUF_LENGTH)
    {
        uint8_t sectionNbr = 0;
        ManagerHMI_ParseJson(buf, size, &sectionNbr);
        ManagerHMI_ExecuteJson(sectionNbr);
    }
}

void ManagerHMI_ParseJson(char* msg, uint8_t maxlength, uint8_t* sectionNbr)
{
    // Reset ParsedMsg array
    memset(ParsedMsg, 0, sizeof(ParsedMsg));

    // Check if the message starts with '{' and ends with '}'
    if (msg[0] != '{' || msg[maxlength - 1] != '}')
    {
        // Invalid message format
        return;
    }

    // Parse number of sections
    uint8_t sectionCount = 0;
    char*   ptr          = strtok(msg + 1, ";");  // Skip the '{'
    while (ptr != NULL && sectionCount < SECTION_NBR)
    {
        if (*ptr != '}')  // Ignore '}' as a section
        {
            strncpy(ParsedMsg[sectionCount], ptr, SECTION_LENGTH - 1);
            ParsedMsg[sectionCount][SECTION_LENGTH - 1] =
                '\0';  // Ensure null-terminated
            sectionCount++;
            ptr = strtok(NULL, ";");
        }
        else
        {
            break;
        }
    }

    *sectionNbr = sectionCount;

    // Check if the number of sections exceeds the maximum allowed
    if (sectionCount >= SECTION_NBR)
    {
        // Too many sections, truncate
        *sectionNbr = 0;
        return;
    }
}

void ManagerHMI_ExecuteJson(uint8_t sectionNbr)
{
    if (sectionNbr >= 3)
    {
        if (strcmp(ParsedMsg[M_HMI_MODE_SECTION], "Manual") == 0)
        {
            if (strcmp(ParsedMsg[M_HMI_ACTION_SECTION], "Increment") == 0)
            {
                ManagerHMI_ExecuteManualIncrement(ParsedMsg[M_HMI_CONTENT_SECTION]);
            }
        }
        else if (strcmp(ParsedMsg[M_HMI_MODE_SECTION], "Auto") == 0)
        {
			if (strcmp(ParsedMsg[M_HMI_ACTION_SECTION], "Plan") == 0)
			{
				ManagerHMI_ExecutePlanCmd(ParsedMsg[M_HMI_CONTENT_SECTION], sectionNbr - M_HMI_CONTENT_SECTION -1);
			}
			else if (strcmp(ParsedMsg[M_HMI_ACTION_SECTION], "Control") == 0)
			{
				ManagerHMI_ExecuteControlCmd(ParsedMsg[M_HMI_CONTENT_SECTION]);
			}
        }
    }
}

void ManagerHMI_ExecuteManualIncrement(char* cmd)
{
    if (cmd != NULL)
    {
        if (strcmp(cmd, "eversionR") == 0)
        {
            ManagerMovement_ManualCmdEversion(MMOV_RIGTH);
        }
        else if (strcmp(cmd, "eversionL") == 0)
        {
            ManagerMovement_ManualCmdEversion(MMOV_LEFT);
        }
        else if (strcmp(cmd, "dorsiflexionU") == 0)
        {
            ManagerMovement_ManualCmdDorsiflexion(MMOV_UP);
        }
        else if (strcmp(cmd, "dorsiflexionD") == 0)
        {
            ManagerMovement_ManualCmdDorsiflexion(MMOV_DOWN);
        }
        else if (strcmp(cmd, "extensionU") == 0)
        {
            ManagerMovement_ManualCmdExtension(MMOV_UP);
        }
        else if (strcmp(cmd, "extensionD") == 0)
        {
            ManagerMovement_ManualCmdExtension(MMOV_DOWN);
        }
        else if (strcmp(cmd, "goHome1") == 0)
        {
            ManagerMovement_ManualCmdHome(MMOT_MOTOR_1);
        }
        else if (strcmp(cmd, "goHome2") == 0)
        {
            ManagerMovement_ManualCmdHome(MMOT_MOTOR_2);
        }
        else if (strcmp(cmd, "goHome3") == 0)
        {
            ManagerMovement_ManualCmdHome(MMOT_MOTOR_3);
        }
        else if (strcmp(cmd, "goHome") == 0)
        {
            ManagerMovement_ManualCmdHomeAll();
        }
    }
}

void ManagerHMI_ExecuteManualHoming(char* cmd)
{
    if (cmd != NULL)
    {
        if (strcmp(cmd, "goHome1") == 0)
        {
            ManagerMovement_ManualCmdHome(MMOT_MOTOR_1);
        }
        else if (strcmp(cmd, "goHome2") == 0)
        {
            ManagerMovement_ManualCmdHome(MMOT_MOTOR_2);
        }
        else if (strcmp(cmd, "goHome3") == 0)
        {
            ManagerMovement_ManualCmdHome(MMOT_MOTOR_3);
        }
        else if (strcmp(cmd, "goHome") == 0)
        {
            ManagerMovement_ManualCmdHomeAll();
        }
    }
}

void ManagerHMI_ExecutePlanCmd(char* cmd, uint8_t size)
{
	if (true/*ManagerMovement_ResetExercise()*/)
	{
		//Verif if prefix of plan is present
		if (size > M_HMI_CONTENT_FIRST_EXERCISE_SECTION)
		{
			//Get max torque and pos (skip for now)
			cmd += M_HMI_CONTENT_FIRST_EXERCISE_SECTION;
			size -= M_HMI_CONTENT_FIRST_EXERCISE_SECTION;

			//Verif if exercise plan is ok
			if (size % M_HMI_EXERCISE_SECTION_NBR == 0)
			{
				uint8_t exNbr = size / M_HMI_EXERCISE_SECTION_NBR;

				//Get exercise data
				for (uint8_t i = 0; i < exNbr; i++)
				{
					char *strExercise = cmd++;
					uint8_t rep = atoi(cmd++);
					float rest = atof(cmd++);
					float pos = atof(cmd++);
					float torque = atof(cmd++);
					float time = atof(cmd++);

					uint8_t exercise;

					if (strcmp(strExercise, "Dorsiflexion") == 0)
					{
						exercise = MMOV_DORSIFLEXION;
					}
					else if (strcmp(strExercise, "Eversion") == 0)
					{
						exercise = MMOV_EVERSION;
					}
					else if (strcmp(strExercise, "Extension") == 0)
					{
						exercise = MMOV_EXTENSION;
					}

					ManagerMovement_AddExercise(i, exercise ,rep, time, rest);
					ManagerMovement_SetFinalPos(i, pos);
				}
			}
		}
	}
}

void ManagerHMI_ExecuteControlCmd(char* cmd)
{
	if (strcmp(ParsedMsg[2], "Start") == 0)
	{
		ManagerMovement_StartExercise();
	}
	else if (strcmp(ParsedMsg[2], "Next") == 0)
	{
		ManagerMovement_NextExercise();
	}
	else if (strcmp(ParsedMsg[2], "Pause") == 0)
	{
		//ManagerMovement_PauseExercise();
	}
	else if (strcmp(ParsedMsg[2], "Stop") == 0)
	{
		ManagerMovement_StopExercise();
	}
}

 void ManagerHMI_GetStrMode(uint8_t index, char* str)
 {
	 switch (index)
	 {
		 case MMOV_STATE_WAITING_SECURITY:
			 str = "WaitingSecurity";
			 break;
		 case MMOV_STATE_HOMING:
			 str = "Homing";
			 break;
		 case MMOV_STATE_MANUAL:
			 str = "Manual";
			 break;
		 case MMOV_STATE_AUTOMATIC:
			 str = "Automatic";
			 break;
		 case MMOV_STATE_ERROR:
			 str = "Error";
			 break;
		 default:
			 str = "";
			 break;
	 }
 }

 void ManagerHMI_GetStrAutoState(uint8_t index, char* str)
 {
	 switch (index)
	 {
		 case MMOV_AUTO_STATE_WAITING4PLAN:
			 str = "WaitingForPlan";
			 break;
		 case MMOV_AUTO_STATE_READY:
			 str = "Ready";
			 break;
		 case MMOV_AUTO_STATE_2GOAL:
			 str = "ToGoal";
			 break;
		 case MMOV_AUTO_STATE_STRETCHING:
			 str = "Stretching";
			 break;
		 case MMOV_AUTO_STATE_2FIRST_POS:
			 str = "ToFirstPos";
			 break;
		 case MMOV_AUTO_STATE_STOP:
			 str = "Pause";
			 break;
		 default:
			 str = "";
			 break;
	 }
 }

 void ManagerHMI_GetStrHomingState(uint8_t index, char* str)
 {
	 switch (index)
	 {
		 case MMOV_VERIF_PERSON_IN:
			 str = "VerifIfUser";
			 break;
		 case MMOV_HOMING_EXTENSION:
			 str = "Extension";
			 break;
		 case MMOV_HOMING_EVERSION:
			 str = "Eversion";
			 break;
		 case MMOV_HOMING_DORSIFLEXION:
			 str = "Dorsiflexion";
			 break;
		 case MMOV_REST_POS:
			 str = "Rest";
			 break;
		 default:
			 str = "";
			 break;
	 }
 }




