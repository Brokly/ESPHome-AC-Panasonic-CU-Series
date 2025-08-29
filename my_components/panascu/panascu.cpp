
 #include "panascu.h"

namespace esphome
{
    namespace panascu
    {
        
        // ---------------- PanasCUClimateIr -----------------------
        void PanasCUClimateIr::setup(){
            ClimateIR::setup();
            #ifdef PANASUS_RESTORE_MODE
                this->loadDataFlash();
            #endif   
            #ifdef PANASCU_EXT_SWING
                this->swing_select->publish_state(this->swing_select->at(curData.numSwing).value()); // восстановить селектор
            #endif
            this->timerPWRF=0;
            this->oldPreset=climate::CLIMATE_PRESET_NONE;
            this->oldMode = climate::CLIMATE_MODE_OFF;
            this->mode = curData.mode;
            this->target_temperature = curData.target_temperature;
            this->fan_mode = curData.fan_mode;
            this->swing_mode = curData.swing_mode;
            this->publish_state();
            #ifdef PANASUS_RESTORE_MODE
               this->sendSwing=false;
            #else 
               this->sendFull=true;
               this->sendPreset=true; 
               this->sendSwing=true;
            #endif
        }

     #ifdef PANASUS_SHOW_ACTION
        bool PanasCUClimateIr::checkAction(){
            //auto action_=climate::CLIMATE_ACTION_OFF;
            auto action_=this->action;
            if(this->mode==climate::CLIMATE_MODE_COOL){
                if(this->target_temperature >= this->sensor_->state+DELTA_TEMP/2){
                    action_ = climate::CLIMATE_ACTION_IDLE;
                } else if(this->target_temperature <= this->sensor_->state-DELTA_TEMP/2){
                    action_ = climate::CLIMATE_ACTION_COOLING;
                }
            } else if(this->mode==climate::CLIMATE_MODE_HEAT){
                if(this->target_temperature <= this->sensor_->state-DELTA_TEMP/2){
                    action_ = climate::CLIMATE_ACTION_IDLE;
                } else if(this->target_temperature >= this->sensor_->state+DELTA_TEMP/2){
                    action_ = climate::CLIMATE_ACTION_HEATING;
                }
            } else if(this->mode==climate::CLIMATE_MODE_AUTO){
                if(this->sensor_->state >= this->target_temperature+DELTA_TEMP+DELTA_TEMP/2){
                    action_ = climate::CLIMATE_ACTION_COOLING;
                } else if(this->sensor_->state <= this->target_temperature-DELTA_TEMP/2){
                    action_ = climate::CLIMATE_ACTION_HEATING;
                } else {
                     action_ = climate::CLIMATE_ACTION_DRYING;
                }
            } else if(this->mode==climate::CLIMATE_MODE_DRY){
                if(this->target_temperature < this->sensor_->state){
                    action_ = climate::CLIMATE_ACTION_DRYING;
                } else {
                    action_ = climate::CLIMATE_ACTION_IDLE;
                }
            } else if(this->mode==climate::CLIMATE_MODE_OFF){
                action_ = climate::CLIMATE_ACTION_OFF;  
            }

            #ifdef PANASCU_USE_POWER_SENSOR
                // если сенсор питания мигает, зачит кондей выходт на режим и это простой 
                if(blink_counter>0 && action_!=climate::CLIMATE_ACTION_OFF){
                   action_ = climate::CLIMATE_ACTION_IDLE;   
                }
            #endif
            
            if(this->action!=action_){
                this->action=action_;
                return true;
            }
            return false;
        }
      #endif

        void PanasCUClimateIr::loop(){
            ClimateIR::loop();
            uint32_t now_= esphome::millis();
            if(now_-loopTimer>DELAY_LOOP){
                loopTimer=now_;
                
                // передавать пакеты пока не передадим все
                if(this->sendPreset || this->sendFull || this->sendSwing){
                   transmit_state();
                   return;                   
                }
                bool needPub=false;
              #ifdef PANASCU_USE_SEN_TIMER  
                if(now_ - sen_timer > DELAY_CHECK_MODE_PRESET) // таймер проверки обработки переключения режимов
              #endif
                {
                
                   #ifdef PANASCU_USE_SEN_TIMER
                      this->temp_pow_sen=false;
                   #endif
                 
                   #ifdef PANASCU_USE_POWFULL_SENSOR
                       if(powerfull_sensor!=nullptr){
                           if(powerfull_sensor->state){
                               this->temp_pow_sen=true; // раз актвен сенсор POWERFULL, то кондей включен
                               if(this->preset!=climate::CLIMATE_PRESET_BOOST && this->mode!=climate::CLIMATE_MODE_OFF){
                                   #ifdef PANASCU_USE_ECO_SENSOR
                                       if(eco_sensor!=nullptr && eco_sensor->state==false)
                                   #endif                                    
                                       {                              
                                           this->preset=climate::CLIMATE_PRESET_BOOST;
                                           needPub=true;
                                       }
                               }
                           } else {
                               if(this->preset==climate::CLIMATE_PRESET_BOOST){
                                   this->preset=climate::CLIMATE_PRESET_NONE;
                                   needPub=true;
                               }
                           }                            
                       }
                   #else
                        if(timerPWRF!=0 && now_-timerPWRF>DELAY_POWERFULL_OFF){ // контроль авто-отключения POWERFULL по таймеру
                           if(this->preset==climate::CLIMATE_PRESET_BOOST){
                               this->preset=climate::CLIMATE_PRESET_NONE;
                               needPub=true;
                               ESP_LOGV(TAG,"Powerfull auto off on timer");
                           }
                           timerPWRF=0;
                        }
                   #endif    
   
                   #ifdef PANASCU_USE_ECO_SENSOR
                       if(eco_sensor!=nullptr){
                           if(eco_sensor->state){
                               this->temp_pow_sen=true; // раз актвен сенсор ECO, то кондей включен
                               if(this->preset!=climate::CLIMATE_PRESET_ECO && this->mode!=climate::CLIMATE_MODE_OFF){
                                   this->preset=climate::CLIMATE_PRESET_ECO;
                                   timerPWRF=0;
                                   needPub=true;
                               }
                           } else {
                               if(this->preset==climate::CLIMATE_PRESET_ECO){
                                   this->preset=climate::CLIMATE_PRESET_NONE;
                                   needPub=true;
                               }
                           }                            
                       }
                   #endif
                    
                   #ifdef PANASCU_USE_POWER_SENSOR
                       if(power_sensor!=nullptr){ // сенсор есть
                            if(power_sensor->state){
                               this->temp_pow_sen=true;
                               pow_sen_timer=now_;
                               if(blink_counter>0) blink_counter--;
                           } else {
                               if(now_ - pow_sen_timer<DELAY_POWER_BLINK){ // прошло мало времени с выключения сенсора, возможно мигает
                                   this->temp_pow_sen=true;
                                   blink_counter=2+(DELAY_CHECK_MODE_PRESET/DELAY_POWER_BLINK); // флаг детекции режима мигания 
                               } else {
                                   blink_counter=0;
                               }
                           } 
                       }
                    
                       if(this->temp_pow_sen){
                           if(this->mode==climate::CLIMATE_MODE_OFF){
                               if(this->oldMode!=climate::CLIMATE_MODE_OFF){
                                  this->mode=this->oldMode;
                               } else { // так исправляем колизию режима, кондей можно включить кнопкой на нем
                                  
                                  // так кондей покажет, что встал в авто, с кнопки обычно ставится режим АВТО
                                  this->mode=climate::CLIMATE_MODE_AUTO;
                                  this->oldMode=climate::CLIMATE_MODE_OFF;
                                  this->target_temperature=PANAAC_TEMP_AUTO;
                                  
                                  // так кондей прнудительно отключится
                                  //this->oldMode=climate::CLIMATE_MODE_COOL; 
                                  
                                  this->sendFull=true;
                               }
                               needPub=true;
                           }
                       } else {
                           if(this->mode!=climate::CLIMATE_MODE_OFF){
                               this->mode=climate::CLIMATE_MODE_OFF;
                               needPub=true;
                           }
                       }
                   #endif
                }
              #ifdef PANASCU_USE_SEN_TIMER  
                else
                {
                    if(power_sensor!=nullptr  && power_sensor->state){ // сенсор есть
                        pow_sen_timer=now_;
                        this->temp_pow_sen=true;
                    }
                }
              #endif
                
                if(this->mode==climate::CLIMATE_MODE_OFF && this->preset!=climate::CLIMATE_PRESET_NONE){ // сброс пресета в отключенном состоянии
                    this->preset=climate::CLIMATE_PRESET_NONE;
                    timerPWRF=0;
                    needPub=true;
                }

                #ifdef PANASUS_SHOW_ACTION
                    if(this->sensor_!=nullptr && this->checkAction()){ 
                        needPub=true;
                    }
                #endif

                if(needPub){
                    this->publish_state();
                    //PANASUS_SET_SAVE;
                }
                
                #ifdef PANASUS_RESTORE_MODE
                    if(needRestoreMode){
                        if(now_>PANASUS_RESTORE_MODE){
                            this->sendFull=true;
                            this->sendPreset=true; 
                            this->sendSwing=true;
                            //needRestoreMode=false;
                            ESP_LOGD(TAG,"Mode restore.");
                        }
                    } else if(needSave){
                        needSave=false;
                        saveDataFlash();
                    }
                #endif
                
            }
        }
 
        // конфигурация визуализации
        climate::ClimateTraits PanasCUClimateIr::traits() {
            
            auto traits = climate::ClimateTraits();
            traits.set_supports_current_temperature(this->sensor_ != nullptr);
            #ifdef PANASUS_SHOW_ACTION
                traits.set_supports_action(this->sensor_ != nullptr);
            #else
                traits.set_supports_action(false);
            #endif
            
            traits.set_visual_min_temperature(PANAAC_TEMP_MIN);
            traits.set_visual_max_temperature(PANAAC_TEMP_MAX);
            traits.set_visual_temperature_step(PANAAC_TEMP_STEP);
            
            traits.set_supported_modes(
                {   climate::CLIMATE_MODE_OFF, 
                    climate::CLIMATE_MODE_AUTO, 
                    climate::CLIMATE_MODE_DRY,
                    #ifdef PANASUS_HEAT_ACTIVE
                       climate::CLIMATE_MODE_HEAT,
                    #endif
                    climate::CLIMATE_MODE_COOL
                });
 
            traits.set_supported_fan_modes(
                {   climate::CLIMATE_FAN_AUTO,
                    climate::CLIMATE_FAN_LOW,       
                    climate::CLIMATE_FAN_MEDIUM,    
                    climate::CLIMATE_FAN_HIGH       
                });

            traits.set_supported_swing_modes(
                {   climate::CLIMATE_SWING_OFF, 
                    climate::CLIMATE_SWING_VERTICAL
                });
            
            traits.set_supported_presets(
                {   climate::CLIMATE_PRESET_NONE, 
                    climate::CLIMATE_PRESET_ECO,
                    climate::CLIMATE_PRESET_BOOST
                });
            
            return traits;
        }
        
        // получение по ИК пакета
        uint8_t PanasCUClimateIr::decode_data(remote_base::RemoteReceiveData data, uint16_t* buff){

            auto raw_data = data.get_raw_data();
            uint8_t size=raw_data.size();
            ESP_LOGV(TAG,"Get IR data size= %d",size);
            
            // первичная фильтрация по размеру посылки
            if((size>=132 && size<=142) || (size>=198 && size<=206)){
                //...
            } else {
                return 0;
            }
            
            uint8_t mask=0;
            uint8_t j=0xFF;
            uint8_t bu_size=1+size/16;
            uint8_t state_bytes[bu_size]={0};
            uint8_t sign[bu_size]={0};
            uint8_t index=data.get_index();
            
            while (index < size-1) {
                if(mask==0){ // байт заполнен, переходим к следующему
                    if(j!=0xFF && j>0){
                        for(uint8_t k=0; k<j && k<bu_size; k++){ // сравнение с уже полученным 
                            if(state_bytes[k]==state_bytes[j]){
                                sign[k]++; // подсчет веса текущего байта
                                j--;
                                k=0xFF; // так закончу цикл 
                            }
                        }
                    }
                    mask=1; // инициализаия переменных для чтения сл. байта
                    j++;
                    state_bytes[j]=0;
                    sign[j]=0;
                }
                if (data.expect_item(PANAAC_BIT_MARK, PANAAC_ONE_SPACE)){ // bit 1
                    state_bytes[j] |= mask;
                    mask<<=1;
                } else if (data.expect_item(PANAAC_BIT_MARK, PANAAC_ZERO_SPACE)){ // bit 0
                    mask<<=1;
                } else { // значение не соответствующее инфо битам, попробуем расшифровку снова
                    data.advance(2);
                    mask=1;
                    state_bytes[j]=0;
                }
                index=data.get_index();
            }
 
            //for(uint8_t k=0; k<j; k++){ // выбор данных по весам
            //    ESP_LOGE(TAG,"Data= %02X, sign= %d", state_bytes[k], sign[k]);
            //}
            
            uint8_t bu1=0xFF;
            int8_t si1=-1;            
            uint8_t bu2=0xFF;              
            int8_t si2=-1;      
            uint8_t che=0xF0;
            for(uint8_t i=0; i<2; i++){
                for(uint8_t k=0; k<j; k++){ // выбор данных по весам
                    if(state_bytes[k]>=che){
                        if(sign[k]>si1){
                            bu1=state_bytes[k];
                            si1=sign[k];
                        }                        
                    } else {
                        if(sign[k]>si2){
                            bu2=state_bytes[k];
                            si2=sign[k];
                        }                        
                    }
                }
                che=0xC9;
                if(si1>-1 && si2>-1){
                    i=2;
                } else {
                   si1=-1;
                   si2=-1;
                }
            } 
            if(si1>-1 && si2>-1){ // первичная проверка данных, при положительной отдать посылку в работу
                *buff = 0x100*bu1 + bu2;
                ESP_LOGD(TAG,"Get packet data= 0x%04X, count= %d", buff[0], size/66);
                return size/66;
            }
            return 0;
        }
 
        // отправка пакета по ИК
        void PanasCUClimateIr::transmit_data(uint16_t* buff, uint8_t count, uint8_t size){

            if(size>1){
               ESP_LOGD(TAG,"Send long packet, size=%d data= [0x%04X, 0x%04X], count= %d", size, buff[0], buff[1], count);
            } else {
               ESP_LOGD(TAG,"Send packet data= 0x%04X, count= %d", buff[0], count);
            }
            
            auto transmit = this->transmitter_->transmit();
            auto *raw = transmit.get_data();
            #ifndef PANASUS_IR_RETRANSLATE
               raw->set_carrier_frequency(PANAAC_CARIER_FREQ);
            #endif
            
            uint8_t* data;
            for(uint8_t f=0;f<size;f++){
                data=(uint8_t*)(&(buff[f]));
                raw->mark(PANAAC_HEADER_MARK);
                raw->space(PANAAC_HEADER_SPACE);
                for(uint8_t k=0;k<count;k++){
                    for(uint8_t j=0;j<2;j++){
                        for(uint8_t i=0;i<2;i++){
                            uint8_t mask=1;
                            while (mask){ 
                                raw->mark(PANAAC_BIT_MARK);
                                raw->space(((data[j] & mask)!=0) ? PANAAC_ONE_SPACE : PANAAC_ZERO_SPACE);
                                mask<<=1;
                            }
                        }
                    }
                    raw->mark(PANAAC_HEADER_MARK);
                    raw->space(PANAAC_HEADER_SPACE);
                }
                if(f<size-1){
                   raw->mark(PANAAC_BIT_MARK);
                   raw->space(PANAAC_FRAME_END);
                }          
            }
            raw->mark(PANAAC_BIT_MARK);

            // transmit
            transmit.perform();
            #ifdef PANASUS_RESTORE_MODE
                needRestoreMode=false; // если отправили пакет, то восстановление режима работы более не нужно
            #endif
            PANASUS_SET_SAVE;
            #ifdef PANASCU_USE_SEN_TIMER  
                this->sen_timer=esphome::millis(); // задерживаем проверку переключения режимов
                //this->pow_sen_timer=this->sen_timer;
            #endif

        }

        // построение пакета на основе текущего состояния и отправка ИК посылки
        void PanasCUClimateIr::transmit_state(){
            uint16_t buff[3];
            if(sendFull){
                sendFull=false;
                convPanIr acIrData;
                acIrData.data.header=0xF;
               
                // температура
                acIrData.data.setTemp(this->target_temperature);
            
                // признак вкл-выкл
                //#ifdef PANASCU_USE_POWER_SENSOR
                //    if(power_sensor!=nullptr){
                //        if(power_sensor->state == (this->mode==climate::CLIMATE_MODE_OFF)){
                //            acIrData.data.power=PAN_POWER_PRESS;
                //        }
                //    }
                //#else
                    if(this->mode!=this->oldMode && (this->mode==climate::CLIMATE_MODE_OFF || this->oldMode==climate::CLIMATE_MODE_OFF)){
                        acIrData.data.power=PAN_POWER_PRESS;
                    }
                //#endif
                this->oldMode=this->mode;
                // текущий режим
                if(this->mode==climate::CLIMATE_MODE_HEAT){
                    acIrData.data.mode=PAN_MODE_HEAT;
                } else if(this->mode==climate::CLIMATE_MODE_DRY || this->mode==climate::CLIMATE_MODE_OFF){
                    acIrData.data.mode=PAN_MODE_DRY;
                } else if(this->mode==climate::CLIMATE_MODE_COOL){
                    acIrData.data.mode=PAN_MODE_COOL;
                } else if(this->mode==climate::CLIMATE_MODE_AUTO){
                    float temp=acIrData.data.Temp();
                    if(temp>(PANAAC_TEMP_AUTOHI+PANAAC_TEMP_AUTO)/2){ //21
                        acIrData.data.mode=PAN_MODE_AUTOHI;            
                        acIrData.data.setTemp(27); // какая то особенность кондея
                        //this->target_temperature=PANAAC_TEMP_AUTOHI;  //22
                    } else if(temp<(PANAAC_TEMP_AUTOLO+PANAAC_TEMP_AUTO)/2){ //19
                        acIrData.data.mode=PAN_MODE_AUTOLO;                   
                        acIrData.data.setTemp(16); // какая то особенность кондея
                        //this->target_temperature=PANAAC_TEMP_AUTOLO; //18
                    } else {  //20
                        acIrData.data.mode=PAN_MODE_AUTO;
                        acIrData.data.setTemp(17);
                        //this->target_temperature=PANAAC_TEMP_AUTO; //20
                    }
                }
            
                // fan
                if(this->fan_mode==climate::CLIMATE_FAN_AUTO){
                    acIrData.data.fan=PAN_FAN_AUTO;
                } else if(this->fan_mode==climate::CLIMATE_FAN_HIGH){
                    acIrData.data.fan=PAN_FAN_LEVEL_3;
                } else if(this->fan_mode==climate::CLIMATE_FAN_MEDIUM){
                    acIrData.data.fan=PAN_FAN_LEVEL_2;
                } else if(this->fan_mode==climate::CLIMATE_FAN_LOW){
                    acIrData.data.fan=PAN_FAN_LEVEL_1;
                }

                // отправляем базовую ИК посылку
                buff[0]=acIrData.val;
                ESP_LOGD(TAG,"Power= %d, Mode = %d, Fan = %02X, Temperature = %d",acIrData.data.power, acIrData.data.mode, acIrData.data.fan, 30-acIrData.data.tem);

                // люверсы - ТЕКУЩЕЕ ПОЛОЖЕНИЕ
                if(this->swing_mode==climate::CLIMATE_SWING_VERTICAL){
                    buff[1]=P_SWINGV_AUTO;
                } else {
                    buff[1]=P_SWINGV_LOW;
      #ifdef PANASCU_EXT_SWING  
                    if(this->swing_select!=nullptr){
                        epSwingPosV pos[6]={P_SWINGV_AUTO, P_SWINGV_LOWEST, P_SWINGV_LOW, P_SWINGV_MIDDLE, P_SWINGV_HIGH, P_SWINGV_HIGHEST};
                        buff[1]=pos[curData.numSwing];   
                    }
      #endif
                }
                transmit_data(buff, 2, 2); 
            } else if(sendPreset){ // пресеты
                sendPreset=false;
                buff[0]=0;
                if(this->preset==climate::CLIMATE_PRESET_NONE){
                    if(oldPreset==climate::CLIMATE_PRESET_ECO){
                        buff[0]=P_ECO;                        
                        oldPreset=climate::CLIMATE_PRESET_NONE;
                    } else if(oldPreset==climate::CLIMATE_PRESET_BOOST){
                        buff[0]=P_PWRF;                        
                        oldPreset=climate::CLIMATE_PRESET_NONE;
                    }
                    timerPWRF=0;
                } else if(this->preset==climate::CLIMATE_PRESET_ECO){
                    if(oldPreset!=climate::CLIMATE_PRESET_ECO){
                        oldPreset=climate::CLIMATE_PRESET_ECO;
                        buff[0]=P_ECO;                        
                        timerPWRF=0;                        
                    }
                } else if(this->preset==climate::CLIMATE_PRESET_BOOST){
                    if(oldPreset!=climate::CLIMATE_PRESET_BOOST){
                        oldPreset=climate::CLIMATE_PRESET_BOOST;
                        buff[0]=P_PWRF;
                        timerPWRF=esphome::millis();                        
                    }
                }
                if(buff[0]!=0){
                   transmit_data(buff, 3);
                }
                
            } else if(sendSwing){ // люверсы - ПЕРЕКЛЮЧЕНИЕ
                sendSwing=false;
                if(this->swing_mode==climate::CLIMATE_SWING_VERTICAL){
                    buff[0]=P_SWV_AUTO;
                } else {
                    buff[0]=P_SWV_LOW;
      #ifdef PANASCU_EXT_SWING  
                    if(swing_select!=nullptr){
                        epSwingRunV pos[6]={P_SWV_AUTO, P_SWV_LOWEST, P_SWV_LOW, P_SWV_MIDDLE, P_SWV_HIGH, P_SWV_HIGHEST};
                        buff[0]=pos[curData.numSwing];   
                    }
      #endif
                }
                transmit_data(buff, 3); 
            }
        }

        // сюда прииходят RAW данные ИК посылки
        bool PanasCUClimateIr::on_receive(remote_base::RemoteReceiveData data) {
            auto raw_data = data.get_raw_data();
            uint8_t len=raw_data.size();
            
            ESP_LOGV(TAG, "Received %d bits", len);
            if(len<120){
                return false;
            }

            uint16_t buff;
            len=decode_data(data, &buff); // расшировываем посылку
            if (len==0) {
                ESP_LOGD(TAG, "Decode IR data failed");
                return false;
            }

            // разбираем данные
            bool ret=true;
            bool ch=false; // флаг изменениий 
            #ifdef PANASUS_IR_RETRANSLATE
                transmit_data(&buff, len);
            #endif
            uint8_t lu=0xFF; // новое положение люверс
            if(len==2){
                if((buff & 0xF000) == 0xF000){ // получили основной пакет пакет
                    convPanIr acIrData;
                    acIrData.val=buff;
                    // температура
                    float newTemp=acIrData.data.Temp();
                    if(this->target_temperature != newTemp){
                       this->target_temperature = newTemp;
                       ch=true;                       
                    }
                    // mode
                    auto newMode = this->mode;
                    switch (acIrData.data.mode)
                    {
                        case PAN_MODE_AUTOHI:
                            newMode=climate::CLIMATE_MODE_AUTO;
                            this->target_temperature=PANAAC_TEMP_AUTOHI;
                            break;
                        case PAN_MODE_AUTO:
                            newMode=climate::CLIMATE_MODE_AUTO;
                            this->target_temperature=PANAAC_TEMP_AUTO;
                            break;
                        case PAN_MODE_AUTOLO:
                            newMode=climate::CLIMATE_MODE_AUTO;
                            this->target_temperature=PANAAC_TEMP_AUTOLO;
                            break;
                        case PAN_MODE_HEAT:
                            newMode=climate::CLIMATE_MODE_HEAT;
                            break;
                        case PAN_MODE_DRY:
                            newMode=climate::CLIMATE_MODE_DRY;
                            break;
                        case PAN_MODE_COOL:
                            newMode=climate::CLIMATE_MODE_COOL;
                            break;
                    }
                    if(acIrData.data.power == PAN_POWER_UNPRESS){ // кнопка вкл-выкл не нажата
                        if(this->mode != climate::CLIMATE_MODE_OFF){ //кондей включен
                            if(this->mode != newMode){
                                this->mode = newMode;
                                ch=true;
                            }
                        } 
                    } else { // кнопка вкл-выкл нажата
                        if(this->mode != climate::CLIMATE_MODE_OFF){ //кондей включен
                            this->mode = climate::CLIMATE_MODE_OFF;
                        } else { // кондей выключен
                            this->mode = newMode;
                        }
                        ch=true;
                    }
                    // fan
                    auto newFan = this->fan_mode;
                    switch (acIrData.data.fan)
                    {
                        case PAN_FAN_AUTO:
                            newFan=climate::CLIMATE_FAN_AUTO;
                            break;
                        case PAN_FAN_LEVEL_3:
                            newFan=climate::CLIMATE_FAN_HIGH;
                            break;
                        case PAN_FAN_LEVEL_2:
                            newFan=climate::CLIMATE_FAN_MEDIUM;
                            break;
                        case PAN_FAN_LEVEL_1:
                            newFan=climate::CLIMATE_FAN_LOW;
                            break;
                    }
                    if(this->fan_mode != newFan){
                        this->fan_mode = newFan;
                        ch=true;
                    }
                } else if(buff <= P_SWINGV_HIGHEST){ // получили текущее положение люверc
                    if(buff==P_SWINGV_AUTO){
                        lu=0;
                        if(this->swing_mode!=climate::CLIMATE_SWING_VERTICAL){
                           this->swing_mode = climate::CLIMATE_SWING_VERTICAL; 
                           ch=true;
                        }                           
                    } else {
                        if(this->swing_mode==climate::CLIMATE_SWING_VERTICAL){
                           this->swing_mode = climate::CLIMATE_SWING_OFF; 
                           ch=true;
                        }
        #ifdef PANASCU_EXT_SWING  
                        switch (buff)
                        {
                            case P_SWINGV_LOWEST:
                                lu=1;
                                break;
                            case P_SWINGV_LOW:
                                lu=2;
                                break;
                            case P_SWINGV_MIDDLE:
                                lu=3;
                                break;
                            case P_SWINGV_HIGH:
                                lu=4;
                                break;
                            case P_SWINGV_HIGHEST:
                                lu=5;
                                break;
                        }
        #endif
                    }
                } else {
                    ret=false;
                }         
            } else if(len==3){
                if(buff==P_ECO){ // получили команду о изменении пресета
                    if(this->preset!=climate::CLIMATE_PRESET_ECO){
                        this->preset=climate::CLIMATE_PRESET_ECO;
                        ch=true;                            
                    } else {
                        this->preset=climate::CLIMATE_PRESET_NONE;
                        ch=true;                            
                    }
                    ret=true;
                } else if(buff==P_PWRF){
                    if(this->preset!=climate::CLIMATE_PRESET_BOOST){
                        this->preset=climate::CLIMATE_PRESET_BOOST;
                        timerPWRF=esphome::millis();  //таймер отключения режима BOOST                      
                        ch=true;                            
                    } else {
                        this->preset=climate::CLIMATE_PRESET_NONE;
                        ch=true;                            
                    }
                    ret=true;
                } else if(buff<P_SWV_LOWEST){
                    //...
                } else if(buff<=P_SWV_HIGHEST){
                    ret=true;
        #ifdef PANASCU_EXT_SWING  
                    // режимы люверов не совместимые с хомассистант
                    if(buff==P_SWV_LOWEST){
                        lu=1;
                    } else if(buff==P_SWV_LOW){
                        lu=2;
                    } else if(buff==P_SWV_MIDDLE){
                        lu=3;
                    } else if(buff==P_SWV_HIGH){
                        lu=4;
                    } else if(buff==P_SWV_HIGHEST){
                        lu=5;
                    } else {   
                        ret=false;
                    }
        #endif
                    if(this->swing_mode == climate::CLIMATE_SWING_VERTICAL && ret){
                       this->swing_mode = climate::CLIMATE_SWING_OFF;
                       ch=true;                            
                    } 
                } else if(buff==P_SWV_AUTO){
                    lu=0;
                    if(this->swing_mode != climate::CLIMATE_SWING_VERTICAL){
                       this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
                       ch=true;
                    } 
                    ret=true;
                } else { 
                    ret=false;
                }
            } else {
                ret=false;
            }
            if(ret==false){
               ESP_LOGD(TAG, "Unknown packet len = %d, data = %04X", len, buff);
               return false;
            }
      #ifdef PANASCU_EXT_SWING  
            if(lu!=0xFF){
                this->swing_select->publish_state(this->swing_select->at(lu).value());  
                this->sendSwing=false;
            }
      #endif
            if(ch){
               this->publish_state();
               #ifndef PANASUS_IR_RETRANSLATE
                  PANASUS_SET_SAVE;
               #endif
            }
            return true;
        }

        // управлеие из esphome
        void PanasCUClimateIr::control(const climate::ClimateCall &call) {
            this->sendFull=false;
            float coreTemp=0; // для корректировки тепературы в режиме авто
            float newTemp=0;
            if(call.get_target_temperature().has_value()){
                newTemp=*call.get_target_temperature();
                if(this->target_temperature != newTemp ){
                    this->sendFull=true;
                }
                if(this->mode==climate::CLIMATE_MODE_AUTO){
                    if(newTemp>PANAAC_TEMP_AUTOHI){
                        coreTemp=PANAAC_TEMP_AUTOHI;
                    } else if(newTemp<PANAAC_TEMP_AUTOLO){
                        coreTemp=PANAAC_TEMP_AUTOLO; 
                    }
                }
            }
            if (call.get_mode().has_value()) {
                auto mode = *call.get_mode();
                if(this->mode != mode){
                    this->sendFull=true;
                    if(mode==climate::CLIMATE_MODE_AUTO){
                        if(newTemp==0){
                            newTemp=this->target_temperature;
                        }
                        if(newTemp>PANAAC_TEMP_AUTOHI){
                            coreTemp=PANAAC_TEMP_AUTOHI;
                        } else if(newTemp<PANAAC_TEMP_AUTOLO){
                            coreTemp=PANAAC_TEMP_AUTOLO; 
                        }
                    }
                }
            }
            if (call.get_swing_mode().has_value()){
                auto newSwing = *call.get_swing_mode(); 
                if(this->swing_mode != newSwing){
                    //this->sendFull=true;
                    this->sendSwing=true;
      #ifdef PANASCU_EXT_SWING  
                    uint8_t pos=curData.saveSwing;
                    if(newSwing == climate::CLIMATE_SWING_VERTICAL){
                        pos=0;                        
                    } 
                    ESP_LOGV("","Set swing %d, %s", pos, this->swing_select->at(pos).value().c_str());
                    this->swing_select->publish_state(this->swing_select->at(pos).value());  
      #endif
                }
            }
            if (call.get_fan_mode().has_value()){
                if(fan_mode != *call.get_fan_mode()){
                    this->sendFull=true;
                }
            }
            
            this->sendPreset=false;
            if (call.get_preset().has_value()){
                if(this->preset != *call.get_preset()){
                    this->sendPreset=true;
                }
            }
            if(this->sendPreset || this->sendFull || this->sendSwing){
                climate_ir::ClimateIR::control(call);
                if(coreTemp!=0){
                    this->target_temperature=coreTemp;
                    this->publish_state();
                    //PANASUS_SET_SAVE;
                }
            }
        }
        
      #ifdef PANASUS_RESTORE_MODE

        // сохранение данных во флеш
        bool PanasCUClimateIr::saveDataFlash(){ 
            curData.mode=this->mode;
            curData.target_temperature=this->target_temperature;
            curData.fan_mode=this->fan_mode.value();
            curData.swing_mode=this->swing_mode;
            if(memcmp(&this->curData,&this->oldData,sizeof(this->curData))!=0){ // данные были изменены
                if (this->storage.save(&this->curData) && global_preferences->sync()){ // данные успешно сохранены
                    memcpy(&this->oldData,&this->curData,sizeof(this->curData)); // копируем копию данных в буфер сравнения
                    ESP_LOGV(TAG, "Data store to flash."); 
                } else {
                    ESP_LOGE(TAG, "Data store to flash - ERROR !");
                    return false;
                }
            }
            return true;
        }

        // получение данных восстановления из флеша
        bool PanasCUClimateIr::loadDataFlash(){
            bool res=storage.load(&this->oldData);
            if(res){ // результат
                memcpy(&this->curData,&this->oldData,sizeof(this->curData));
                ESP_LOGD(TAG, "Data load from flash.");
                return true;
            }
            memcpy(&this->oldData,&this->curData,sizeof(this->curData));
            ESP_LOGE(TAG, "Stored data load - ERROR or NOT INIT!"); 
            return false;
        }
      
      #endif        

        void PanasCUClimateIr::dump_config(){
            ESP_LOGCONFIG(TAG, "Panasonic AC CU series:");
            //this->dump_config;   
            this->dump_traits_(TAG); 
            LOG_SENSOR("","Temperature sensor", this->sensor_);
            #ifdef PANASUS_IR_RETRANSLATE
                ESP_LOGCONFIG(TAG, "IR signal retnslate: ON, modulation: OFF");
            #endif            
            #ifdef PANASCU_EXT_SWING  
                ESP_LOGCONFIG(TAG, "Extended select swngs: ACTVE ");
                LOG_SELECT("", "Adittional swings selector", this->swing_select); 
            #endif
            #ifdef PANASCU_USE_POWER_SENSOR
                LOG_BINARY_SENSOR("", "On POWER sensor", this->power_sensor);
            #endif
            #ifdef PANASCU_USE_POWFULL_SENSOR
                LOG_BINARY_SENSOR("", "POWERFULL sensor", this->powerfull_sensor);
            #endif
            #ifdef PANASCU_USE_ECO_SENSOR
                LOG_BINARY_SENSOR("", "ECO sensor", this->eco_sensor);
            #endif
            #ifdef PANASUS_RESTORE_MODE
                ESP_LOGCONFIG(TAG, "Mode restore after power lost: ACTVE ");
            #endif
            //#ifdef PANASUS_SHOW_ACTION
            //    ESP_LOGCONFIG(TAG, "Show sintetic action: ACTVE ");
            //#endif
        }

        
        // селектор дополнительных режимов люверс
      #ifdef PANASCU_EXT_SWING  
        void PanasCUClimateIr::set_swing_select(PanasCUSwingSelect *sel){  // подключение селектора
                this->swing_select = sel;
                //this->swing_select->publish_state(this->swing_select->at(curData.numSwing).value());
                sel->add_on_state_callback([this](const std::string payload, size_t num) {
                    if(num>5) num=5;
                    if(curData.numSwing==num){
                        return;
                    }
                    curData.numSwing=num;
                    if(num==0){
                        if(this->swing_mode != climate::CLIMATE_SWING_VERTICAL){
                            this->swing_mode = climate::CLIMATE_SWING_VERTICAL; 
                            this->publish_state(); 
                            //PANASUS_SET_SAVE;
                        }
                    } else {
                        curData.saveSwing=num;
                        if(this->swing_mode == climate::CLIMATE_SWING_VERTICAL){
                            this->swing_mode = climate::CLIMATE_SWING_OFF;
                            this->publish_state();                            
                            //PANASUS_SET_SAVE;
                        }
                    }
                    this->sendSwing=true;
                });
            }
      #endif
    } // namespace panascu
} // namespace esphome
