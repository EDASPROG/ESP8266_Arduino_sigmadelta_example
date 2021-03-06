/*
/******************************************************************************
 * MODULEName : set_sigma MODULE

EACH PIN CAN CONNET TO A SIGMA-DELTA , ALL PINS SHEARS THE SAME SIGMA-DELTA SOURCE.

THE TARGET DUTY AND FREQUENCY CAN BE MODIFIED VIA THE REG ADDR GPIO_SIGMA_DELTA

THE TARGET FREQUENCY IS DEFINED AS:

FREQ = 80,000,000/prescale * target /256  HZ,     0<target<128
FREQ = 80,000,000/prescale * (256-target) /256  HZ,     128<target<256
target: duty ,0-255
prescale: clk_div,0-255
so the target and prescale will both affect the freq.


YOU CAN DO THE TEST LIKE THIS:
1. INIT :  sigma_delta_setup(PERIPHS_IO_MUX_MTDI_U,12,FUNC_GPIO12);
2. USE 312K:   set_sigma_duty_312KHz(2);
OR 2. SET VAL:  set_sigma_target(uint8 target) AND  set_sigma_prescale

3. DEINIT AND DISABLE: sigma_delta_close(uint32 GPIO_NUM), eg.sigma_delta_close(2)

*******************************************************************************/

#include "wiring_private.h"
#include "pins_arduino.h"

#include "osapi.h"
#include "c_types.h"
#include "ets_sys.h"
#include "eagle_soc.h"
#include "gpio.h"


#define GPIO_SIGMA_DELTA  ESP8266_REG(0x368) //GPIO_SIGMA_DELTA

//#define GPIO_PIN_ADDR(i)                    (GPIO_PIN0_ADDRESS + i*4)
//#define GPIO_SIGMA_DELTA                     0x68  //defined in gpio register.xls

#define GPIO_SIGMA_DELTA_SETTING_MASK       (0x00000001ff)

#define GPIO_SIGMA_DELTA_ENABLE             1
#define GPIO_SIGMA_DELTA_DISABLE            (~GPIO_SIGMA_DELTA_ENABLE)
#define GPIO_SIGMA_DELTA_MSB                16
#define GPIO_SIGMA_DELTA_LSB                16
#define GPIO_SIGMA_DELTA_MASK               (0x00000001<<GPIO_SIGMA_DELTA_LSB)
#define GPIO_SIGMA_DELTA_GET(x)             (((x) & GPIO_SIGMA_DELTA_MASK) >> GPIO_SIGMA_DELTA_LSB)
#define GPIO_SIGMA_DELTA_SET(x)             (((x) << GPIO_SIGMA_DELTA_LSB) & GPIO_SIGMA_DELTA_MASK)


#define GPIO_SIGMA_DELTA_TARGET_MSB         7
#define GPIO_SIGMA_DELTA_TARGET_LSB         0
#define GPIO_SIGMA_DELTA_TARGET_MASK        (0x000000FF<<GPIO_SIGMA_DELTA_TARGET_LSB)
#define GPIO_SIGMA_DELTA_TARGET_GET(x)      (((x) & GPIO_SIGMA_DELTA_TARGET_MASK) >> GPIO_SIGMA_DELTA_TARGET_LSB)
#define GPIO_SIGMA_DELTA_TARGET_SET(x)      (((x) << GPIO_SIGMA_DELTA_TARGET_LSB) & GPIO_SIGMA_DELTA_TARGET_MASK)


#define GPIO_SIGMA_DELTA_PRESCALE_MSB       15
#define GPIO_SIGMA_DELTA_PRESCALE_LSB       8
#define GPIO_SIGMA_DELTA_PRESCALE_MASK      (0x000000FF<<GPIO_SIGMA_DELTA_PRESCALE_LSB)
#define GPIO_SIGMA_DELTA_PRESCALE_GET(x)    (((x) & GPIO_SIGMA_DELTA_PRESCALE_MASK) >> GPIO_SIGMA_DELTA_PRESCALE_LSB)
#define GPIO_SIGMA_DELTA_PRESCALE_SET(x)    (((x) << GPIO_SIGMA_DELTA_PRESCALE_LSB) & GPIO_SIGMA_DELTA_PRESCALE_MASK)


/******************************************************************************
 * FunctionName : sigma_delta_setup
 * Description  : Init Pin Config for Sigma_delta , change pin source to sigma-delta
 * Parameters   : uint32 GPIO_MUX, GPIO MUX REG ,DEFINED IN EAGLE_SOC.H, e.g.: PERIPHS_IO_MUX_MTCK_U
        uint32 GPIO_NUM, GPIO NUM ACCORDING TO THE MUX NUM , e.g.: 13 for MTCK
        uint32 GPIO_FUNC, GPIO PIN FUNC , DEFINED IN EAGLE_SOC.H , e.g.: FUNC_GPIO13
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
sigma_delta_setup(uint32 GPIO_NUM)
{
    //============================================================================
    //STEP 1: SIGMA-DELTA CONFIG;REG SETUP
    GPIO_SIGMA_DELTA=
                            ( GPIO_SIGMA_DELTA &(~GPIO_SIGMA_DELTA_SETTING_MASK))|
                              GPIO_SIGMA_DELTA_SET(GPIO_SIGMA_DELTA_ENABLE)|
                              GPIO_SIGMA_DELTA_TARGET_SET(0x00)|
                              GPIO_SIGMA_DELTA_PRESCALE_SET(0x00) ;

    //============================================================================

    //STEP 2: PIN FUNC CONFIG :SET PIN TO GPIO MODE, ENABLE OUTPUT AND SET SOURCE AS SIGMA
    GPF(GPIO_NUM) = GPFFS(GPFFS_GPIO(GPIO_NUM));//Set mode to GPIO
    GPC(GPIO_NUM) = (1 << GPCS)| (0 << GPCD) | (0 << GPCI) | (0 << GPCWE); //SOURCE(SIGMA) | DRIVER(NORMAL) | INT_TYPE(UNCHANGED) | WAKEUP_ENABLE(DISABLED)
    GPES = (1 << GPIO_NUM); //Enable

    
    //============================================================================

}




/******************************************************************************
 * FunctionName : sigma_delta_close
 * Description  : DEinit Pin ,from Sigma_delta mode to GPIO input mode
 * Parameters   : uint32 GPIO_NUM, GPIO NUM ACCORDING TO THE MUX NUM , e.g.: 13 for MTCK
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
    sigma_delta_close(uint32 GPIO_NUM)
{

    //============================================================================
    //STEP 1: SIGMA-DELTA DEINIT
    GPIO_SIGMA_DELTA=
                            ( GPIO_SIGMA_DELTA &(~GPIO_SIGMA_DELTA_SETTING_MASK))|
                              GPIO_SIGMA_DELTA_SET(GPIO_SIGMA_DELTA_DISABLE)|
                              GPIO_SIGMA_DELTA_TARGET_SET(0x00)|
                              GPIO_SIGMA_DELTA_PRESCALE_SET(0x00) ;
    //ets_printf("test reg gpio sigma : 0x%08x \n",GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(GPIO_SIGMA_DELTA_NUM))));
    //============================================================================

    //STEP 2: GPIO OUTPUT DISABLE
    GPEC = (1 << GPIO_NUM); //Disable
    //============================================================================

    //STEP 3: CONNECT GPIO TO PIN PAD

    GPC(GPIO_NUM) = GPC(GPIO_NUM)| (0 << GPCS); //SOURCE(GPIO) | DRIVER(NORMAL) | INT_TYPE(UNCHANGED) | WAKEUP_ENABLE(DISABLED)

    //============================================================================

}



/******************************************************************************
 * FunctionName : set_sigma_target
 * Description  : SET TARGET DUTY FOR SIGMA-DELTA 
 * Parameters   : uint8 target, DUTY NUM , 1BYTE , DUTY RANGE : 0-255
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
    set_sigma_target(uint8 target)
{
    //set sigma signal duty target
        GPIO_SIGMA_DELTA=
                                                        (GPIO_SIGMA_DELTA &(~GPIO_SIGMA_DELTA_TARGET_MASK))|
                                                          GPIO_SIGMA_DELTA_TARGET_SET(target);
}

/******************************************************************************
 * FunctionName : set_sigma_prescale
 * Description  : SET SIGMA-DELTA SIGNAL CLK PRESCALE(CLE_DIV)
 * Parameters   : uint8 prescale, CLK_DIV , 0-255
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
    set_sigma_prescale(uint8 prescale)
{
    //set sigma signal clk prescale(clk div) 
        GPIO_SIGMA_DELTA=
                                                        (GPIO_SIGMA_DELTA &(~GPIO_SIGMA_DELTA_PRESCALE_MASK))|
                                                          GPIO_SIGMA_DELTA_PRESCALE_SET(prescale);

}



/******************************************************************************
 * FunctionName : set_sigma_duty_312KHz
 * Description  : 312K CONFIG EXAMPLE
 * Parameters   : uint8 duty, TARGET DUTY FOR 312K,  0-255
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
    set_sigma_duty_312KHz(uint8 duty)
{
    
    uint8 target = 0,prescale=0;
    target = (duty>128)?(256-duty):duty;
    prescale = (target==0)?0:(target-1);

    //freq = 80000 (khz) /256 /duty_target * (prescale+1)
    set_sigma_target(duty);//SET DUTY TARGET
    set_sigma_prescale(prescale);//SET CLK DIV
    
}


/******************************************************************************
 * FunctionName : set_sigma_duty_156KHz
 * Description  : 156K CONFIG EXAMPLE
 * Parameters   : uint8 duty, TARGET DUTY FOR 156K,  0-255
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
    set_sigma_duty_156KHz(uint8 duty)
{

    uint8 target = 0,prescale=0;
    target = (duty>128)?(256-duty):duty;
    prescale = (target==0)?0:(2*target-1);

    set_sigma_target(duty);//SET DUTY TARGET
    set_sigma_prescale(prescale);//SET CLK DIV

}
