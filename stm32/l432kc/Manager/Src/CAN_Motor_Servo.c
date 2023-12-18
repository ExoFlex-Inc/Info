#include "CAN_Motor_Servo.h"

extern float p_in_1;
extern float p_in_2;
extern float p_in_3;

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan)
{
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData);
}

void buffer_append_int16(uint8_t* buffer, int16_t number, int16_t* index)
{
    buffer[(*index)++] = number >> 8;
    buffer[(*index)++] = number;
}

void buffer_append_int32(uint8_t* buffer, int32_t number, int32_t* index)
{
    buffer[(*index)++] = number >> 24;
    buffer[(*index)++] = number >> 16;
    buffer[(*index)++] = number >> 8;
    buffer[(*index)++] = number;
}

void comm_can_transmit_eid(uint32_t id, const uint8_t* data, uint8_t len)
{
    uint8_t i = 0;

    if (len > 8)
    {
        len = 8;
    }

    TxHeader.DLC   = len;  // data length
    TxHeader.IDE   = CAN_ID_EXT;
    TxHeader.RTR   = CAN_RTR_DATA;
    TxHeader.StdId = 0;
    TxHeader.ExtId = id;  // ID
    for (i = 0; i < len; i++)
    {
        TxData[i] = data[i];
    }

    HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);
}

void motor_receive(float* motor_pos, float* motor_spd, float* motor_cur,
                   uint8_t* motor_temp, uint8_t* motor_error)
{
    int16_t pos_int = RxData[0] << 8 | RxData[1];
    int16_t spd_int = RxData[2] << 8 | RxData[3];
    int16_t cur_int = RxData[4] << 8 | RxData[5];

    *motor_pos   = (float) (pos_int * 0.1f);  // motor pos
    *motor_spd   = (float) (spd_int * 0.1f);  // motor spd
    *motor_cur   = (float) (cur_int * 0.1f);  // motor cur
    *motor_temp  = RxData[6];  // motor temp
    *motor_error = RxData[7];  // motor error
}

void comm_can_set_origin(uint8_t controller_id)
{
    // 0: Temporary origin (power failure elimination),
    // 1: Permanent zero point (automatic parameter saving),
    // 2: Restoring the default zero point (automatic parameter saving)
    // For now, only mode 0 works and position can't be saved in drive for power
    // off

    // For now, enforce set_origin_mode to be 0
    uint8_t set_origin_mode = 0;

    int8_t send_index = 1;
    comm_can_transmit_eid(
        controller_id | ((uint32_t) CAN_PACKET_SET_ORIGIN_HERE << 8),
        &set_origin_mode, send_index);

    // Reset positions based on controller_id
    if (controller_id == 1)
    {
        p_in_1 = 0.0;
    }
    else if (controller_id == 2)
    {
        p_in_2 = 0.0;
    }
    else if (controller_id == 3)
    {
        p_in_3 = 0.0;
    }
}

void comm_can_set_pos(uint8_t controller_id, float pos)
{
    int32_t send_index = 0;
    uint8_t buffer[4];
    buffer_append_int32(buffer, (int32_t) (pos * 1000000.0), &send_index);
    comm_can_transmit_eid(controller_id | ((uint32_t) CAN_PACKET_SET_POS << 8),
                          buffer, send_index);
}

void comm_can_set_pos_spd(uint8_t controller_id, float pos, int16_t spd,
                          int16_t RPA)
{
    // pos: -3600-3600,
    // spd:                        -32768-32767,
    // acc: 0-200

    int32_t send_index  = 0;
    int16_t send_index1 = 4;
    int16_t send_index2 = 6;
    uint8_t buffer[8];
    buffer_append_int32(buffer, (int32_t) (pos * 10000.0), &send_index);

    buffer_append_int16(buffer, spd, &send_index1);

    buffer_append_int16(buffer, RPA, &send_index2);

    comm_can_transmit_eid(controller_id | ((uint32_t) CAN_PACKET_POS_SPD << 8),
                          buffer, send_index2);
}
