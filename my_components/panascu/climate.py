
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate_ir, select, binary_sensor

from esphome.const import (
    CONF_ID,
    CONF_OPTIONS,
    CONF_ICON,
    CONF_SENSOR,
#    CONF_NUMBER,
#    CONF_CURRENT,
#    CONF_VALUE,
#    CONF_INITIAL_VALUE,
#    CONF_MAX_VALUE,
#    CONF_MIN_VALUE,
#    CONF_STEP,
#    CONF_UNIT_OF_MEASUREMENT,
#    CONF_ACCURACY_DECIMALS,
#    CONF_RESTORE_VALUE,
#    CONF_DEBUG,
)

CODEOWNERS = ["@Brokly"]
DEPENDENCIES = ["climate_ir"]
AUTO_LOAD = ['climate_ir','select','binary_sensor']

CONF_SWING = "swing_add"
ICON_SWING = "mdi:tailwind"

CONF_POWER_SENSOR   = "power_sens_id"
CONF_POWFULL_SENSOR = "power_full_sens_id"
CONF_ECO_SENSOR     = "eco_sens_id"
CONF_IR_RETRANSLATE = "ir_retranslate"
CONF_HEAT_MODE      = "heat_mode"
CONF_SHOW_ACTION    = "show_action"
CONF_RESTORE        = "restore_timeouf"

panascu_ns = cg.esphome_ns.namespace('panascu')
PanasCUClimateIr = panascu_ns.class_('PanasCUClimateIr', climate_ir.ClimateIR)
PanasCUSwingSelect = panascu_ns.class_('PanasCUSwingSelect', select.Select)

OPTIONS_SWING = [
    "Auto",
    "Lowest",
    "Low",
    "Middle",
    "High",
    "Highest",
]

def validate_actions(config):
    if CONF_SHOW_ACTION in config and config[CONF_SHOW_ACTION] and CONF_SENSOR not in config:
       raise cv.Invalid("For a show actions need use termo sensor. Add 'sensor:' field.")
    return config

CONFIG_SCHEMA = cv.All(
  climate_ir.climate_ir_with_receiver_schema(PanasCUClimateIr).extend({
    cv.GenerateID(): cv.declare_id(PanasCUClimateIr),
    cv.Optional(CONF_IR_RETRANSLATE, default=False): cv.boolean,
    cv.Optional(CONF_HEAT_MODE, default=False): cv.boolean,
    cv.Optional(CONF_SHOW_ACTION, default=False): cv.boolean,
    #cv.Optional(CONF_RESTORE, default=False): cv.boolean,
    cv.Optional(CONF_RESTORE): cv.positive_time_period_milliseconds,
    cv.Optional(CONF_POWER_SENSOR): cv.use_id(binary_sensor.BinarySensor),
    cv.Optional(CONF_POWFULL_SENSOR): cv.use_id(binary_sensor.BinarySensor),
    cv.Optional(CONF_ECO_SENSOR): cv.use_id(binary_sensor.BinarySensor),
    # положение люверов
    cv.Optional(CONF_SWING): select.select_schema(PanasCUSwingSelect).extend(
        {
            cv.GenerateID(): cv.declare_id(PanasCUSwingSelect),
            cv.Optional(CONF_ICON, default=ICON_SWING): cv.icon,
            cv.Optional(CONF_OPTIONS,default=OPTIONS_SWING):cv.All(
                cv.ensure_list(cv.string_strict), 
                cv.Length(min=1), 
            ),
        }
    ),
  }),
  validate_actions,
)


async def to_code(config):

    var = await climate_ir.new_climate_ir(config)

    if CONF_SWING in config:
        cg.add_define("PANASCU_EXT_SWING")
        sel = await select.new_select(config[CONF_SWING],options=config[CONF_SWING][CONF_OPTIONS])
        cg.add(var.set_swing_select(sel))

    if CONF_POWER_SENSOR in config:
        if sensor_id := config.get(CONF_POWER_SENSOR):
            cg.add_define("PANASCU_USE_POWER_SENSOR")
            sens = await cg.get_variable(sensor_id)
            cg.add(var.set_power_sensor(sens))
 
    if CONF_POWFULL_SENSOR in config:
        if sensor_id := config.get(CONF_POWFULL_SENSOR):
            cg.add_define("PANASCU_USE_POWFULL_SENSOR")
            sens = await cg.get_variable(sensor_id)
            cg.add(var.set_powerfull_sensor(sens))

    if CONF_ECO_SENSOR in config:
        if sensor_id := config.get(CONF_ECO_SENSOR):
            cg.add_define("PANASCU_USE_ECO_SENSOR")
            sens = await cg.get_variable(sensor_id)
            cg.add(var.set_eco_sensor(sens))

    if CONF_IR_RETRANSLATE in config:
        if config[CONF_IR_RETRANSLATE]:
           cg.add_define("PANASUS_IR_RETRANSLATE")

    if CONF_HEAT_MODE in config:
        if config[CONF_HEAT_MODE]:
           cg.add_define("PANASUS_HEAT_ACTIVE")

    if CONF_SHOW_ACTION in config:
        if config[CONF_SHOW_ACTION]:
           cg.add_define("PANASUS_SHOW_ACTION")

    if CONF_RESTORE in config:
        if config[CONF_RESTORE]:
           cg.add_define("PANASUS_RESTORE_MODE",config[CONF_RESTORE])


