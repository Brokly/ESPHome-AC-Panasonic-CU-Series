
#pragma once

#include <cstdlib>
#include <cinttypes>
#include "esphome/components/climate_ir/climate_ir.h"
#include "esphome/components/select/select.h"
#include "esphome/components/remote_base/remote_base.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/core/log.h"

namespace esphome
{
    namespace panascu
    {
        static const char *TAG = "PanasCU";
 
        // Temperature
        constexpr float PANAAC_TEMP_MIN = 16.0; // Celsius
        constexpr float PANAAC_TEMP_MAX = 30.0; // Celsius
        constexpr float PANAAC_TEMP_STEP = 1.0;
        constexpr float PANAAC_TEMP_AUTO = 20;
        constexpr float PANAAC_TEMP_AUTOHI = PANAAC_TEMP_AUTO+2;
        constexpr float PANAAC_TEMP_AUTOLO = PANAAC_TEMP_AUTO-2;

        // Pulse parameters in usec
        constexpr uint16_t PANAAC_BIT_MARK = 1000;     
        constexpr uint16_t PANAAC_ONE_SPACE = 800;     
        constexpr uint16_t PANAAC_ZERO_SPACE = 2600;   
        constexpr uint16_t PANAAC_HEADER_MARK = 3500;  
        constexpr uint16_t PANAAC_HEADER_SPACE = 3400; 
        constexpr uint16_t PANAAC_FRAME_END = 12000; 
        constexpr uint16_t PANAAC_CARIER_FREQ = 38000;
        
        constexpr uint32_t DELAY_POWERFULL_OFF = 900000; // задержка авто отключение режима POWERFULL в mS
        constexpr uint32_t DELAY_LOOP = 250;   // период проверки режимов работы в LOOP
        constexpr uint8_t  DELTA_TEMP = 2;     // дельта температуры работы кондеея
        constexpr uint8_t  STANDART_TEMP = 22; // стандартная температура, не знаю что имеют в виду составители мануала
        constexpr uint32_t DELAY_POWER_BLINK = 1500; // время проверки мигания диода включения
        constexpr uint32_t DELAY_CHECK_MODE_PRESET = 2000; // время задержки проверки режима работы по сенсорам, после переключения режимов и пресетов
        

        // тип пакета
        // старший нимбл старшего байта __ F_ - тип пакета F - режим, температура, ON/OFF
        // младший бит младшего байта   _X __ - температура рассчет 30-значение
        //                              __ _8 - 4 бит признак нажатия кнопк ON/OFF
        //                              __ _0 - mode AUTO HI (fan-auto,temp-17) 8
        //                              __ _1 - mode AUTO (fan-auto,temp-16)    9
        //                              __ _2 - mode AUTO LO (fan-auto,temp-27) A
        //                              __ _3 - mode HEAT                       B
        //                              __ _4 - mode DRY                        C
        //                              __ _5 - mode COOL                       D
        //                              0_ __ - fan auto
        //                              D_ __ - fan 1
        //                              B_ __ - fan 2
        //                              9_ __ - fan 3

        // старший нимбл старшего байта __ С9 - тип пакета С9 - люверы текущее положение
        //                              0B __ - АВТО
        //                              6F __ - HIGEST
        //                              5F __ - HI
        //                              4F __ - MID
        //                              3F __ - LOW
        //                              2F __ - LOWEST

        // старший нимбл старшего байта __ СD - тип пакета СD - люверы переключение в положение
        //                              5E __ - HIGEST
        //                              5D __ - HI
        //                              5C __ - MID
        //                              5B __ - LOW
        //                              5A __ - LOWEST

        // старший нимбл старшего байта __ СF - тип пакета СF - люверы переключение в положение AUTO
        //                              7F __ - АВТО

        // старший нимбл старшего байта __ СA - тип пакета СA - ECO, POWERFULL
        //                              79 __ - POWERFULL
        //                              7A __ - ECO
       
        enum eMode {
            PAN_MODE_AUTOHI = 0,
            PAN_MODE_AUTO = 1,
            PAN_MODE_AUTOLO = 2,
            PAN_MODE_HEAT = 3,
            PAN_MODE_DRY = 4,
            PAN_MODE_COOL = 5,
        };

        enum eFan {
            PAN_FAN_AUTO = 0,
            PAN_FAN_LEVEL_3 = 0x09,
            PAN_FAN_LEVEL_2 = 0x0B,
            PAN_FAN_LEVEL_1 = 0X0D,
        };
 
        enum epSwingPosV:uint16_t{
            P_SWINGV_AUTO   =  0xC90B,
            P_SWINGV_LOWEST  = 0xC92F,
            P_SWINGV_LOW     = 0xC93F,
            P_SWINGV_MIDDLE  = 0xC94F,
            P_SWINGV_HIGH    = 0xC95F,
            P_SWINGV_HIGHEST = 0xC96F,
        };

        enum epSwingRunV:uint16_t {
            P_SWV_AUTO    = 0xCF7F,
            P_SWV_LOWEST  = 0xCD5A,
            P_SWV_LOW     = 0xCD5B,
            P_SWV_MIDDLE  = 0xCD5C,
            P_SWV_HIGH    = 0xCD5D,
            P_SWV_HIGHEST = 0xCD5E,
        };

        enum epAddMode:uint16_t {
            P_ECO    = 0xCA7A,
            P_PWRF   = 0xCA79,
        };
        
        enum ePower{
           PAN_POWER_UNPRESS=0,
           PAN_POWER_PRESS=1,
        };

        struct panIr {
            //=========================
            uint8_t tem:4;          //1
            eFan    fan:4;          //1
            //=========================
            eMode   mode:3;         //0
            ePower  power:1;        //0
            uint8_t header:4;       //0
            //=========================
            void setTemp(float t){tem=30-(uint8_t)t;}
            float Temp(){ return (30-tem);}
        };
        
        union convPanIr{
            panIr     data;
            uint16_t  val;
        };

        struct saveClimate{
            climate::ClimateMode mode = climate::CLIMATE_MODE_OFF;
            int8_t target_temperature=22;
            climate::ClimateFanMode fan_mode=climate::CLIMATE_FAN_AUTO;
            climate::ClimateSwingMode swing_mode=climate::CLIMATE_SWING_VERTICAL;
            uint8_t saveSwing=4;
            uint8_t numSwing=4;
        };

        class PanasCUClimateIr;

    } // namespace panaac
} // namespace esphome
