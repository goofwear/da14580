/**
 ****************************************************************************************
 *
 * @file app.c
 *
 * @brief Application entry point
 *
 * Copyright (C) RivieraWaves 2009-2013
 *
 *
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @addtogroup APP
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"             // SW configuration

#if (BLE_APP_PRESENT)
#include "app_task.h"                // Application task Definition
#include "app.h"                     // Application Definition
#include "gapm_task.h"               // GAP Manager Task API
#include "gapc_task.h"               // GAP Controller Task API
#include "co_math.h"                 // Common Maths Definition
#include "app_api.h"                // Application task Definition

#if (BLE_APP_SEC)
#include "app_sec.h"                 // Application security Definition
#endif // (BLE_APP_SEC)

#if (NVDS_SUPPORT)
#include "nvds.h"                    // NVDS Definitions
#endif //(NVDS_SUPPORT)
#if(ROLE_MASTER_YCOM)
   uint8_t last_char = 0xFF;//randy  20140415
#endif
  
/*
 * ENUMERATIONS
 ****************************************************************************************
 */
struct xapp_env_tag xapp_env;

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Application Environment Structure
struct app_env_tag app_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Application Task Descriptor
static const struct ke_task_desc TASK_DESC_APP = {NULL, &app_default_handler,
                                                  app_state, APP_STATE_MAX, APP_IDX_MAX};



bool bdaddr_compare(struct bd_addr *bd_address1,
                    struct bd_addr *bd_address2)
{
    unsigned char idx;

    for (idx=0; idx < BD_ADDR_LEN; idx++)
    {
        /// checks if the addresses are similar
        if (bd_address1->addr[idx] != bd_address2->addr[idx])
        {
           return (false);
        }
    }
    return(true);
}	

void app_proxm_enable(void)
{
        
    // Allocate the message
    struct proxm_enable_req *msg = KE_MSG_ALLOC(PROXM_ENABLE_REQ, TASK_PROXM, TASK_APP,
                                                 proxm_enable_req);

    // Fill in the parameter structure
    msg->conhdl = xapp_env.proxr_device.device.conhdl;
    msg->con_type = PRF_CON_DISCOVERY;
        
    // Send the message
     ke_msg_send(msg);

}
void app_disc_enable(void)
{       
    // Allocate the message
    struct proxm_enable_req * msg = KE_MSG_ALLOC(DISC_ENABLE_REQ, TASK_DISC, TASK_APP,
                                                proxm_enable_req);

    // Fill in the parameter structure
    msg->conhdl = xapp_env.proxr_device.device.conhdl;
    msg->con_type = PRF_CON_DISCOVERY;
        
    // Send the message
    ke_msg_send(msg);

}
void app_security_enable(void)
{
    // Allocate the message
    struct gapc_bond_cmd * req = KE_MSG_ALLOC(GAPC_BOND_CMD, KE_BUILD_ID(TASK_GAPC, xapp_env.proxr_device.device.conidx), TASK_APP,
                                             gapc_bond_cmd);
    

    req->operation = GAPC_BOND;

    req->pairing.sec_req = GAP_NO_SEC;  //GAP_SEC1_NOAUTH_PAIR_ENC;

    // OOB information
    req->pairing.oob = GAP_OOB_AUTH_DATA_NOT_PRESENT;

    // IO capabilities
    req->pairing.iocap          = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;

    // Authentication requirements
    req->pairing.auth           = GAP_AUTH_REQ_NO_MITM_BOND; //SMP_AUTH_REQ_NO_MITM_NO_BOND;

    // Encryption key size
    req->pairing.key_size       = 16;

    //Initiator key distribution
    req->pairing.ikey_dist      = 0x04; //SMP_KDIST_ENCKEY | SMP_KDIST_IDKEY | SMP_KDIST_SIGNKEY;
		req->pairing.rkey_dist      = 0x03; //SMP_KDIST_ENCKEY | SMP_KDIST_IDKEY | SMP_KDIST_SIGNKEY;

    // Send the message
     ke_msg_send(req);
}
void app_start_encryption(void)
{
    // Allocate the message
    struct gapc_encrypt_cmd * req = KE_MSG_ALLOC(GAPC_ENCRYPT_CMD, KE_BUILD_ID(TASK_GAPC, xapp_env.proxr_device.device.conidx), TASK_APP,
                                                 gapc_encrypt_cmd);


    req->operation = GAPC_ENCRYPT;
    memcpy(&req->ltk.ltk, &xapp_env.proxr_device.ltk, sizeof(struct gapc_ltk));
    
    // Send the message
    ke_msg_send(req);

}
void app_proxm_read_txp(void)
{
    struct proxm_rd_txpw_lvl_req * req = KE_MSG_ALLOC(PROXM_RD_TXPW_LVL_REQ, TASK_PROXM, TASK_APP,
                                                     proxm_rd_txpw_lvl_req);
    
    last_char = PROXM_RD_TX_POWER_LVL;
    
    req->conhdl = xapp_env.proxr_device.device.conhdl;
    ke_msg_send((void *) req);
}
void app_cancel(void)
{
    
    struct gapm_cancel_cmd *msg= KE_MSG_ALLOC(GAPM_CANCEL_CMD, TASK_GAPM, TASK_APP,
                                                  gapm_cancel_cmd);

    msg->operation = GAPM_SCAN_ACTIVE;

    ke_msg_send(msg);
}
/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
																									
unsigned char app_device_recorded(struct bd_addr *padv_addr)
{
    int i;

    for (i=0; i < MAX_SCAN_DEVICES; i++)
    {
        if (xapp_env.devices[i].free == false)
        if (bdaddr_compare(&xapp_env.devices[i].adv_addr, padv_addr))
                break;
    }

    return i;
            
}																									

/**
 ****************************************************************************************
 * @brief Application task initialization.
 *
 * @return void
 ****************************************************************************************
 */

void app_init(void)
{
    #if (NVDS_SUPPORT)
    uint8_t length = NVDS_LEN_SECURITY_ENABLE;
    #endif // NVDS_SUPPORT

    // Reset the environment
    memset(&app_env, 0, sizeof(app_env));

    // Initialize next_prf_init value for first service to add in the database
    app_env.next_prf_init = APP_PRF_LIST_START + 1;

    #if (NVDS_SUPPORT)
    // Get the security enable from the storage
    if (nvds_get(NVDS_TAG_SECURITY_ENABLE, &length, (uint8_t *)&app_env.sec_en) != NVDS_OK)
    #endif // NVDS_SUPPORT
    {
        // Set true by default (several profiles requires security)
        app_env.sec_en = true;
    }

	  app_init_func();    
		
    // Create APP task
    ke_task_create(TASK_APP, &TASK_DESC_APP);

    // Initialize Task state
    ke_state_set(TASK_APP, APP_DISABLED);

    #if (BLE_APP_SEC)
    app_sec_init();
    #endif // (BLE_APP_SEC)
}

/**
 ****************************************************************************************
 * @brief Profiles's Database initialization sequence.
 *
 * @return void
 ****************************************************************************************
 */

bool app_db_init(void)
{
    
    // Indicate if more services need to be added in the database
    bool end_db_create = false;
    
    end_db_create = app_db_init_func();
        
    return end_db_create;
}

/**
 ****************************************************************************************
 * @brief Send BLE disconnect command
 *
 * @return void
 ****************************************************************************************
 */

void app_disconnect(void)
{
    struct gapc_disconnect_cmd *cmd = KE_MSG_ALLOC(GAPC_DISCONNECT_CMD,
                                              KE_BUILD_ID(TASK_GAPC, app_env.conidx), TASK_APP,
                                              gapc_disconnect_cmd);

    cmd->operation = GAPC_DISCONNECT;
    cmd->reason = CO_ERROR_REMOTE_USER_TERM_CON;

    // Send the message
    ke_msg_send(cmd);
}

/**
 ****************************************************************************************
 * @brief Sends a connection confirmation message
 *
 * @return void
 ****************************************************************************************
 */

void app_connect_confirm(uint8_t auth)
{
    // confirm connection
    struct gapc_connection_cfm *cfm = KE_MSG_ALLOC(GAPC_CONNECTION_CFM,
            KE_BUILD_ID(TASK_GAPC, app_env.conidx), TASK_APP,
            gapc_connection_cfm);

    cfm->auth = auth;
    cfm->authorize = GAP_AUTHZ_NOT_SET;

    // Send the message
    ke_msg_send(cfm);
		GPIO_ConfigurePin( GPIO_PORT_1, GPIO_PIN_1, OUTPUT, PID_GPIO, true);//Alert LED  
}


/**
 ****************************************************************************************
 * Advertising Functions
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Start Advertising. Setup Advertsise and Scan Response Message
 *
 * @return void
 ****************************************************************************************
 */

void app_adv_start(void)
{
   
    // Allocate a message for GAP
    struct gapm_start_advertise_cmd *cmd = KE_MSG_ALLOC(GAPM_START_ADVERTISE_CMD,
                                                TASK_GAPM, TASK_APP,
                                                gapm_start_advertise_cmd);
  

    app_adv_func(cmd);
    
    // Send the message
    ke_msg_send(cmd);

    // We are now connectable
    ke_state_set(TASK_APP, APP_CONNECTABLE);
}

/**
 ****************************************************************************************
 * @brief Stop Advertising
 *
 * @return void
 ****************************************************************************************
 */

void app_adv_stop(void)
{
    // Disable Advertising
    struct gapm_cancel_cmd *cmd = KE_MSG_ALLOC(GAPM_CANCEL_CMD,
                                           TASK_GAPM, TASK_APP,
                                           gapm_cancel_cmd);

    cmd->operation = GAPM_CANCEL;

    // Send the message
    ke_msg_send(cmd);
}

/**
 ****************************************************************************************
 * @brief Send a connection param update request message
 *
 * @return void
 ****************************************************************************************
 */

void app_param_update_start(void)
{
    app_param_update_func();
}


/**
 ****************************************************************************************
 * @brief Start a kernel timer
 *
 * @return void
 ****************************************************************************************
 */

void app_timer_set(ke_msg_id_t const timer_id, ke_task_id_t const task_id, uint16_t delay)
{
    // Delay shall not be more than maximum allowed
    if(delay > KE_TIMER_DELAY_MAX)
    {
        delay = KE_TIMER_DELAY_MAX;

    }
    // Delay should not be zero
    else if(delay == 0)
    {
        delay = 1;
    }
    
    ke_timer_set(timer_id, task_id, delay);
}

#endif //(BLE_APP_PRESENT)

/// @} APP
