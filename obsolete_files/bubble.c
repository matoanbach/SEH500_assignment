#include <stdio.h>
#include "fsl_debug_console.h"
#include "math.h"
#include "fsl_fxos.h"
#include "pin_mux.h"
#include "peripherals.h"
#include "board.h"
#include "clock_config.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define I2C_RELEASE_BUS_COUNT 100U
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
void BOARD_I2C_ReleaseBus(void);
static void Board_UpdatePwm(uint16_t x, uint16_t y);
/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile int16_t xAngle = 0;
volatile int16_t yAngle = 0;
volatile int16_t zAngle = 0;
volatile int16_t yawAngle = 0; // Added for yaw value
volatile int16_t pitchAngle = 0; // Pitch value
volatile int16_t rollAngle = 0;  // Roll value

/* FXOS device address */
const uint8_t g_accel_address[] = {0x1CU, 0x1DU, 0x1EU, 0x1FU};

/*******************************************************************************
 * Code
 ******************************************************************************/
// Function prototype
extern int16_t atan_asm(int16_t yData, int16_t xData);


static void i2c_release_bus_delay(void)
{
    uint32_t i = 0;
    for (i = 0; i < I2C_RELEASE_BUS_COUNT; i++)
    {
        __NOP();
    }
}

void BOARD_I2C_ReleaseBus(void)
{
    uint8_t i = 0;

    BOARD_Init_I2C_GPIO_pins();

    /* Drive SDA low first to simulate a start */
    GPIO_PinWrite(BOARD_ACCEL_I2C_SDA_GPIO, BOARD_ACCEL_I2C_SDA_PIN, 0U);
    i2c_release_bus_delay();

    /* Send 9 pulses on SCL and keep SDA high */
    for (i = 0; i < 9; i++)
    {
        GPIO_PinWrite(BOARD_ACCEL_I2C_SCL_GPIO, BOARD_ACCEL_I2C_SCL_PIN, 0U);
        i2c_release_bus_delay();

        GPIO_PinWrite(BOARD_ACCEL_I2C_SDA_GPIO, BOARD_ACCEL_I2C_SDA_PIN, 1U);
        i2c_release_bus_delay();

        GPIO_PinWrite(BOARD_ACCEL_I2C_SCL_GPIO, BOARD_ACCEL_I2C_SCL_PIN, 1U);
        i2c_release_bus_delay();
        i2c_release_bus_delay();
    }

    /* Send stop */
    GPIO_PinWrite(BOARD_ACCEL_I2C_SCL_GPIO, BOARD_ACCEL_I2C_SCL_PIN, 0U);
    i2c_release_bus_delay();

    GPIO_PinWrite(BOARD_ACCEL_I2C_SDA_GPIO, BOARD_ACCEL_I2C_SDA_PIN, 0U);
    i2c_release_bus_delay();

    GPIO_PinWrite(BOARD_ACCEL_I2C_SCL_GPIO, BOARD_ACCEL_I2C_SCL_PIN, 1U);
    i2c_release_bus_delay();

    GPIO_PinWrite(BOARD_ACCEL_I2C_SDA_GPIO, BOARD_ACCEL_I2C_SDA_PIN, 1U);
    i2c_release_bus_delay();
}
/* Update the duty cycle of an active pwm signal */
static void Board_UpdatePwm(uint16_t x, uint16_t y)
{
    /* Start PWM mode with updated duty cycle */
    FTM_UpdatePwmDutycycle(BOARD_TIMER_PERIPHERAL, kFTM_Chnl_5, kFTM_EdgeAlignedPwm, x);
    FTM_UpdatePwmDutycycle(BOARD_TIMER_PERIPHERAL, kFTM_Chnl_1, kFTM_EdgeAlignedPwm, y);
    /* Software trigger to update registers */
    FTM_SetSoftwareTrigger(BOARD_TIMER_PERIPHERAL, true);
}

int main(void)
{

    fxos_handle_t fxosHandle = {0};
    fxos_data_t sensorData   = {0};
    fxos_config_t config     = {0};
    uint8_t sensorRange      = 0;
    uint8_t dataScale        = 0;
    int16_t xData            = 0;
    int16_t yData            = 0;
    int16_t zData            = 0;
    int16_t magXData         = 0;
    int16_t magYData         = 0;
    int16_t magZData         = 0;
    uint8_t i                = 0;
    uint8_t array_addr_size  = 0;
    status_t result          = kStatus_Fail;

    /* Board pin, clock, debug console init */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_I2C_ReleaseBus();
    BOARD_I2C_ConfigurePins();
    BOARD_InitDebugConsole();
    BOARD_InitPeripherals();

    /* Configure the I2C function */
    config.I2C_SendFunc    = BOARD_Accel_I2C_Send;
    config.I2C_ReceiveFunc = BOARD_Accel_I2C_Receive;

    array_addr_size = sizeof(g_accel_address) / sizeof(g_accel_address[0]);
    for (i = 0; i < array_addr_size; i++)
    {
        config.slaveAddress = g_accel_address[i];
        /* Initialize accelerometer sensor */
        result = FXOS_Init(&fxosHandle, &config);
        if (result == kStatus_Success)
        {
            break;
        }
    }

    if (result != kStatus_Success)
    {
        PRINTF("\r\nSensor device initialize failed!\r\n");
        return -1;
    }

    /* Get sensor range */
    if (FXOS_ReadReg(&fxosHandle, XYZ_DATA_CFG_REG, &sensorRange, 1) != kStatus_Success)
    {
        return -1;
    }
    if (sensorRange == 0x00)
    {
        dataScale = 2U;
    }
    else if (sensorRange == 0x01)
    {
        dataScale = 4U;
    }
    else if (sensorRange == 0x10)
    {
        dataScale = 8U;
    }
    else
    {
    }
    /* Start timer */
    FTM_StartTimer(BOARD_TIMER_PERIPHERAL, kFTM_SystemClock);

    /* Print a note to terminal */
    PRINTF("\r\nWelcome to the BUBBLE example\r\n");
    PRINTF("\r\nYou will see the change of angle data and LED brightness when change the angles of board\r\n");

    /* Main loop. Get sensor data and update duty cycle */
    while (1)
    {
        /* Get new accelerometer data. */
        if (FXOS_ReadSensorData(&fxosHandle, &sensorData) != kStatus_Success)
        {
            return -1;
        }

        /* Get the X and Y data from the sensor data structure in 14 bit left format data*/
        xData = (int16_t)((uint16_t)((uint16_t)sensorData.accelXMSB << 8) | (uint16_t)sensorData.accelXLSB) / 4U;
        yData = (int16_t)((uint16_t)((uint16_t)sensorData.accelYMSB << 8) | (uint16_t)sensorData.accelYLSB) / 4U;
        zData = (int16_t)((uint16_t)((uint16_t)sensorData.accelZMSB << 8) | (uint16_t)sensorData.accelZLSB) / 4U;


        /* Get the magnetometer data */
        magXData = (int16_t)((uint16_t)((uint16_t)sensorData.magXMSB << 8) | (uint16_t)sensorData.magXLSB);
        magYData = (int16_t)((uint16_t)((uint16_t)sensorData.magYMSB << 8) | (uint16_t)sensorData.magYLSB);
        magZData = (int16_t)((uint16_t)((uint16_t)sensorData.magZMSB << 8) | (uint16_t)sensorData.magZLSB);

        /* Convert raw data to angle (normalize to 0-90 degrees). No negative angles. */
        xAngle = (int16_t)floor((double)xData * (double)dataScale * 90 / 8192);
        if (xAngle < 0)
        {
            xAngle *= -1;
        }
        yAngle = (int16_t)floor((double)yData * (double)dataScale * 90 / 8192);
        if (yAngle < 0)
        {
            yAngle *= -1;
        }
        zAngle = (int16_t)floor((double)zData * (double)dataScale * 90 / 8192);
        if (zAngle < 0)
        {
            zAngle *= -1;
        }


        /* Using atan_asm to calculate pitch and roll angles from accelerometer data */
        pitchAngle = (int16_t)(atan_asm(yData, (int16_t)sqrt((double)xData * xData + (double)zData * zData)));
        rollAngle = (int16_t)(atan_asm(xData, (int16_t)sqrt((double)yData * yData + (double)zData * zData)));

        /* Calculate yaw angle from magnetometer data */
        yawAngle = (int16_t)(atan_asm((double)magYData, (double)magXData)) * 10;
//        yawAngle = (int16_t)(atan_asm((int16_t)sqrt((double)magXData * magXData + (double)magYData * magYData), magZData));
//        yawAngle = (int16_t)(atan_asm((int16_t)sqrt((double)xData * xData + (double)yData * yData), zData));
        yawAngle %= 360;

        /* Using builtin atan2 to calculate pitch and roll angles from accelerometer data */
//        pitchAngle = (int16_t)(atan2((double)yData, sqrt((double)xData * xData + (double)zData * zData)) * (180.0 / M_PI));
//        rollAngle = (int16_t)(atan2((double)xData, sqrt((double)yData * yData + (double)zData * zData)) * (180.0 / M_PI));
//
//        /* Calculate yaw angle from magnetometer data */
//        yawAngle = (int16_t)(atan2((double)magYData, (double)magXData) * (180.0 / M_PI));
//        if (yawAngle < 0)
//        {
//            yawAngle += 360;
//        }

        /* Update the duty cycle of PWM */
        Board_UpdatePwm(xAngle, yAngle);

        /* Print out the angle data. */
        PRINTF("%2d, %2d, %3d\r\n", pitchAngle, rollAngle, yawAngle);

    }
}
