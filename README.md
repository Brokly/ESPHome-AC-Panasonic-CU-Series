# ESPHome-AC-Panasonic-CU-Series

EspHome component for Air Сonditioners:<br>
CS-A73KE / CU-A73KE<br>
CS-A93KE / CU-A93KE<br>
CS-A123KE / CU-A123KE<br>
CS-C75KE / CU-C75KE<br>
CS-C95KE / CU-C95KE<br>
CS-C125KE / CU-C125KE<br>
and others...

![image](https://github.com/user-attachments/assets/65d0cf1c-f14a-4d0c-a6ea-52152536e391)

Все кондиционеры с таким пультом, скорее всего, будут работать с этим компонентом.

Настройки компонента
```yaml
climate:
  - platform: panascu
    name: ${upper_devicename} Climate
    receiver_id: ir_receiver        # (required) приемник ИК сигнала
    heat_mode: true                 # (optional) поддержка режима обогрева
    swing_add:                      # (optional) использовать дополнительные опции люверс
       name: ${upper_devicename} Swing
    sensor: temperature_id          # (optional) дополнительный датчик температуры
    power_sens_id: pow_id           # (optional) сенсор режима вкл-выкл, крайне желательно 
    power_full_sens_id: powfull_id  # (optional) сенсор режима POWERFULL, в отсутствии - пытается эмулировать программно, может не совпадать период активности режима
    eco_sens_id: eco_id             # (optional) сенсор режима эко, в отсутствии эмулируется программно
    ir_retranslate: True            # (optional) ретранслировать ик сигнала, используется при верехвате подключения ИК датчика кондиционера
    show_action: True               # (optional) показывать аналитику текущего режима работы
    restore_timeouf: 30s            # (optional) таймаут восстановление режима кондиционера после отключения питания, при отсутстви настройки режим не сохраняется
```

Settings:
```yaml
climate:
  - platform: panascu
    name: ${upper_devicename} Climate
    receiver_id: ir_receiver        # (required) IR receiver
    heat_mode: true                 # (optional) air conditioner with heating mode
    swing_add:                      # (optional) use all air direction modes
       name: ${upper_devicename} Swing
    sensor: temperature_id          # (optional) an additional temperature sensor is needed to display the current temperature and operating mode
    power_sens_id: pow_id           # (optional) ON/OFF mode sensor, connect to the air conditioner POWER (red) LED
    power_full_sens_id: powfull_id  # (optional) POWERFULL mode sensor, connect to the air conditioner POWERFULL (orange) LED
    eco_sens_id: eco_id             # (optional) ECO mode sensor, connect to the air conditioner ECO (green) LED
    ir_retranslate: True            # (optional) rebroadcast the IR signal if you have disabled the air conditioner's IR senso
    show_action: True               # (optional) show analytics of the current operating mode
    restore_timeouf: 30s            # (optional) timeout restores the air conditioner mode after power failure, and the mode is not saved if this not setting
```

Я использую одно из китайских реле для управления освещением, оно имеет свой блок питания, который гальванически связан с сетью 220 вольт. Поэтому мне потребовалось сделать адаптер с гальванческой развязкой. Для уменьшения количества проводов подключения, я свел все сенсоры на резистивный делитель и просто вычисляю режим работы измеряя напряжение на этом делителе. Так же я отключил родной ИК приемник кондиционера и использовал дополнительный, родной датчик был рассчтан на питание 5 вольт, а питание процессора модуля 3.3 вольта. Сигнал на кондиионер я передаю так же через оптрон. Разъемы которые требуются для изготовления переходнка: 'JST 1.25mm 9pin' .  На схемах не показан датчик температуры, я использую Dallas DS18b20, его подключение описано в документации по EspHome. Место установки - рядом с испарителем (радиатором) внутреннего блока, можно установить в то же место где установлен родной датчик кондиционера.
![image](https://github.com/user-attachments/assets/da8ca49d-e770-42fb-8426-d66f115e25e0)


Схема адаптера с гальванической развязкой:
![image](https://github.com/user-attachments/assets/aec859e5-9cef-42b1-9c90-03a1928af1fa)

Схема адаптера с питанием от кодиционера. Используется родной ИК датчик кондиционера с ретрансляцией ИК сигнала кондиционеру. С подключением сенсоров состояния:
![image](https://github.com/user-attachments/assets/942b1db8-65ed-4287-ad65-21b62f304886)

Схема адаптера с питанием от кодиционера. Используется родной ИК датчик кондиционера, без ретрансляции ИК сигнала. Без подключения сенсоров состояния:
![image](https://github.com/user-attachments/assets/292abfd5-3b1b-4022-9127-8364ba547ddd)

Разъем, куда подключается адаптер выполненный по схемам выше:
![image](https://github.com/user-attachments/assets/840fabf4-18ec-437f-99ce-1ba3f0032913)

Разборка внутреннего блока очень проста и хорошо описана в руководствах из папки 'docs', данного репозитария.



