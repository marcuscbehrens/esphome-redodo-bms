import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import DEVICE_CLASS_PROBLEM

from . import redodo_bms_ns, RedodoBMS

CONF_REDODO_BMS_ID = "redodo_bms_id"
CONF_PROBLEM = "problem"

MAX_CELLS = 16

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_REDODO_BMS_ID): cv.use_id(RedodoBMS),
    cv.Optional(CONF_PROBLEM): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PROBLEM,
    ),
    **{f"cell_{i+1}_balancing": cv.Optional(f"cell_{i+1}_balancing")(
        binary_sensor.binary_sensor_schema()
    ) for i in range(MAX_CELLS)},
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