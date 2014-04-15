/**
 ****************************************************************************************
 *
 * @file periph_setup.c
 *
 * @brief Peripherals setup and initialization. 
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */
/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "rwip_config.h"             // SW configuration
#include "periph_setup.h"            // periphera configuration
#include "global_io.h"
#include "gpio.h"
#include "uart.h"                    // UART initialization

#include "app_proxr_proj.h"

#ifndef _PERIPH_SETUP_H_
#define _PERIPH_SETUP_H_

/**
 ****************************************************************************************
 * @brief Each application reserves its own GPIOs here.
 *
 * @return void
 ****************************************************************************************
 */

#if DEVELOPMENT__NO_OTP

void GPIO_reservations(void)
{

/*
* Application specific GPIOs reservation
*/    
#if (BLE_APP_PRESENT)
#if BLE_PROX_REPORTER
    RESERVE_GPIO( PUSH_BUTTON, GPIO_PORT_2, GPIO_PIN_3, PID_GPIO);
    RESERVE_GPIO( PUSH_BUTTON, GPIO_PORT_2, GPIO_PIN_6, PID_GPIO);
    //RESERVE_GPIO( PUSH_BUTTON, GPIO_PORT_2, GPIO_PIN_9, PID_GPIO);   	
    RESERVE_GPIO( GREEN_LED, GPIO_PORT_1, GPIO_PIN_1, PID_GPIO);
    RESERVE_GPIO( GREEN_LED, GPIO_PORT_1, GPIO_PIN_2, PID_GPIO);

#endif
#if BLE_BATT_SERVER
	//Setup LED GPIO for battery alert
    RESERVE_GPIO( RED_LED, GPIO_PORT_1, GPIO_PIN_0, PID_GPIO);
#endif
#endif
    
}
#endif

/**
 ****************************************************************************************
 * @brief Map port pins
 *
 * The Uart and SPI port pins and GPIO ports(for debugging) are mapped
 ****************************************************************************************
 */
void set_pad_functions(void)        // set gpio port function mode
{
    
#if BLE_PROX_REPORTER
    GPIO_ConfigurePin( GPIO_PORT_2, GPIO_PIN_3, INPUT_PULLDOWN, PID_GPIO, false ); // Push Button 
	  GPIO_ConfigurePin( GPIO_PORT_2, GPIO_PIN_6, INPUT_PULLDOWN, PID_GPIO, false ); // Push Button 
	  //GPIO_ConfigurePin( GPIO_PORT_2, GPIO_PIN_3, INPUT_PULLDOWN, PID_GPIO, false ); // Push Button 
    GPIO_ConfigurePin( GPIO_PORT_1, GPIO_PIN_1, OUTPUT, PID_GPIO, false ); //Alert LED
    GPIO_ConfigurePin( GPIO_PORT_1, GPIO_PIN_2, OUTPUT, PID_GPIO, false ); //Alert LED

#endif
#if BLE_BATT_SERVER    
    GPIO_ConfigurePin( GPIO_BAT_LED_PORT, GPIO_BAT_LED_PIN, OUTPUT, PID_GPIO, false ); //Battery alert LED
#endif
}


/**
 ****************************************************************************************
 * @brief Enable pad's and peripheral clocks assuming that peripherals' power domain is down. The Uart and SPi clocks are set.
 *
 * @return void
 ****************************************************************************************
 */
void periph_init(void)  // set i2c, spi, uart, uart2 serial clks
{
	// Power up peripherals' power domain
    SetBits16(PMU_CTRL_REG, PERIPH_SLEEP, 0);
    while (!(GetWord16(SYS_STAT_REG) & PER_IS_UP)) ; 
    
    SetBits16(CLK_16M_REG,XTAL16_BIAS_SH_DISABLE, 1);
	  SetBits16(P07_MODE_REG, PID, 0x10);//randy
	
	//rom patch
	patch_func();
	
	//Init pads
	set_pad_functions();


#if (BLE_APP_PRESENT)
    
#if BLE_PROX_REPORTER
    app_proxr_port_reinit(GPIO_PORT_1, GPIO_PIN_1);
    app_proxr_port_reinit(GPIO_PORT_1, GPIO_PIN_2);
	app_button_enable();
#elif BLE_FINDME_LOCATOR
    app_button_enable();
#endif //BLE_PROX_REPORTER
#if BLE_BATTERY_SERVER
    app_batt_port_reinit();
#endif //BLE_BATTERY_SERVER

#endif //BLE_APP_PRESENT

    // Enable the pads
	SetBits16(SYS_CTRL_REG, PAD_LATCH_EN, 1);
}

#endif //_PERIPH_SETUP_H_
