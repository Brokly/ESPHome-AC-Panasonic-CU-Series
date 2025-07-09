
#pragma once

#include <cinttypes>

//#define PANASCU_EXT_SWING          // активация кода селектора расширеных настроек люверсов
//#define PANASCU_USE_POWER_SENSOR   // использовать сенсор состояния вкл-выкл
//#define PANASCU_USE_POWFULL_SENSOR // использовать сенсор режима POWERFULL
//#define PANASCU_USE_ECO_SENSOR     // использовать сенсор режима ECO
//#define PANASUS_IR_RETRANSLATE     // ретрансляция принятого пакета для варианта, когда ИК канал отключен от кондиционера
//#define PANASUS_HEAT_ACTIVE        // активация режима нагрева
//#define PANASUS_RESTORE_MODE      // активация сохранения режима работы 
//#define PANASUS_SHOW_ACTION        // активация режима показа текущего состояния работы

#include "definitions.h"

namespace esphome
{
    namespace panascu
    {

      #ifdef PANASCU_EXT_SWING  
        class PanasCUSwingSelect : public esphome::select::Select, public esphome::Parented<PanasCUClimateIr> {
          protected:
	        void control(const std::string &value) override{
                this->state_callback_.call(value, index_of(value).value());
            }
          friend class PanasCUClimateIr;   
        };
      #endif
        class PanasCUClimateIr : public climate_ir::ClimateIR {
          public:
            PanasCUClimateIr() : climate_ir::ClimateIR(
                                  PANAAC_TEMP_MIN, PANAAC_TEMP_MAX, 1.0f, true, true,
                                  {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH},
                                  {climate::CLIMATE_SWING_OFF,climate::CLIMATE_SWING_VERTICAL},
                                  {climate::CLIMATE_PRESET_NONE, climate::CLIMATE_PRESET_ECO, climate::CLIMATE_PRESET_BOOST}){}

      #ifdef PANASCU_EXT_SWING  
            PanasCUSwingSelect *swing_select = nullptr;// селектор выбора дополнительных режимов работы люверсов
            void set_swing_select(PanasCUSwingSelect *sel);  // подключение селектора
      #endif

      #ifdef PANASCU_USE_POWER_SENSOR
            binary_sensor::BinarySensor *power_sensor{nullptr};
            void set_power_sensor(binary_sensor::BinarySensor *sens){ power_sensor=sens;}
            uint32_t pow_sen_timer=0; // таймер для обработки мигания светодиода POWER
            uint8_t blink_counter=0; // признак мигания
            #ifndef PANASCU_USE_SEN_TIMER
               #define PANASCU_USE_SEN_TIMER
            #endif
      #endif
      #ifdef PANASCU_USE_POWFULL_SENSOR
            binary_sensor::BinarySensor *powerfull_sensor{nullptr};
            void set_powerfull_sensor(binary_sensor::BinarySensor *sens){ powerfull_sensor=sens;}
            #ifndef PANASCU_USE_SEN_TIMER
               #define PANASCU_USE_SEN_TIMER
            #endif
      #endif
      #ifdef PANASCU_USE_ECO_SENSOR
            binary_sensor::BinarySensor *eco_sensor{nullptr};
            void set_eco_sensor(binary_sensor::BinarySensor *sens){ eco_sensor=sens;}
            #ifndef PANASCU_USE_SEN_TIMER
               #define PANASCU_USE_SEN_TIMER
            #endif
      #endif
      #ifdef PANASCU_USE_SEN_TIMER
            uint32_t sen_timer=0; // таймер сенсоров
            bool temp_pow_sen=false; // сенсор включения с очисткой от мигания
      #endif            
            void control(const climate::ClimateCall &call) override;
            void loop() override;
            void setup() override;
            void dump_config() override;

          protected:
            void transmit_state() override;
            bool on_receive(remote_base::RemoteReceiveData data) override;
            climate::ClimateTraits traits() override;
            void transmit_data(uint16_t* buff, uint8_t count, uint8_t size=1);
            uint8_t decode_data(remote_base::RemoteReceiveData data, uint16_t* buff);
       #ifdef PANASUS_SHOW_ACTION
            bool checkAction();
       #endif     
            bool sendFull;
            bool sendPreset; 
            bool sendSwing;
            uint32_t timerPWRF; // таймер режиима powerfull
            uint32_t loopTimer; // таймер проверки режимов
            climate::ClimatePreset oldPreset;
            climate::ClimateMode oldMode;
            saveClimate curData; // данные для сохранения настроек
       #ifdef PANASUS_RESTORE_MODE
            saveClimate oldData; // для контроля сохранения данных
            // объект хранения данныых
            ESPPreferenceObject storage = global_preferences->make_preference<saveClimate>(this->get_object_id_hash());
            bool saveDataFlash();
            bool loadDataFlash();
            bool needSave=false;
            bool needRestoreMode=true;
            #define PANASUS_SET_SAVE needSave=true;
       #else 
            #define PANASUS_SET_SAVE ;
       #endif

 
      #ifdef PANASCU_EXT_SWING  
          friend class PanasCUSwingSelect;
      #endif
 
        };

    } // namespace panascu
} // namespace esphome
