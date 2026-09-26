import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import DEVICE_CLASS_PROBLEM

from . import redodo_bms_ns, RedodoBMS

CONF_REDODO_BMS_ID = "redodo_bms_id"
CONF_PROBLEM = "problem"

MAX_CELLS = 16

_BALANCING_SCHEMA = binary_sensor.binary_sensor_schema()

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_REDODO_BMS_ID): cv.use_id(RedodoBMS),
    cv.Optional(CONF_PROBLEM): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
    ),
    cv.Optional("cell_1_balancing"): _BALANCING_SCHEMA,
    cv.Optional("cell_2_balancing"): _BALANCING_SCHEMA,
    cv.Optional("cell_3_balancing"): _BALANCING_SCHEMA,
    cv.Optional("cell_4_balancing"): _BALANCING_SCHEMA,
    cv.Optional("cell_5_balancing"): _BALANCING_SCHEMA,
    cv.Optional("cell_6_balancing"): _BALANCING_SCHEMA,
    cv.Optional("cell_7_balancing"): _BALANCING_SCHEMA,
    cv.Optional("cell_8_balancing"): _BALANCING_SCHEMA,
    cv.Optional("cell_9_balancing"): _BALANCING_SCHEMA,
    cv.Optional("cell_10_balancing"): _BALANCING_SCHEMA,
    cv.Optional("cell_11_balancing"): _BALANCING_SCHEMA,
    cv.Optional("cell_12_balancing"): _BALANCING_SCHEMA,
    cv.Optional("cell_13_balancing"): _BALANCING_SCHEMA,
    cv.Optional("cell_14_balancing"): _BALANCING_SCHEMA,
    cv.Optional("cell_15_balancing"): _BALANCING_SCHEMA,
    cv.Optional("cell_16_balancing"): _BALANCING_SCHEMA,
})

async def to_code(config):
    hub = await cg.get_variable(config[CONF_REDODO_BMS_ID])

    if CONF_PROBLEM in config:
        bs = await binary_sensor.new_binary_sensor(config[CONF_PROBLEM])
        cg.add(hub.set_problem_binary_sensor(bs))

    for i in range(MAX_CELLS):
        key = f"cell_{i+1}_balancing"
        if key in config:
            bs = await binary_sensor.new_binary_sensor(config[key])
            cg.add(hub.set_balancing_binary_sensor(i, bs))