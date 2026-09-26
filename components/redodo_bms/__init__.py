import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import ble_client
from esphome.const import CONF_ID

CONF_BLE_CLIENT_ID = "ble_client_id"

redodo_bms_ns = cg.esphome_ns.namespace("redodo_bms")
RedodoBMS = redodo_bms_ns.class_(
    "RedodoBMS",
    cg.Component,
    ble_client.BLEClientNode
)

CONFIG_SCHEMA = cv.ensure_list(cv.Schema({
    cv.GenerateID(): cv.declare_id(RedodoBMS),
    cv.Required(CONF_BLE_CLIENT_ID): cv.use_id(ble_client.BLEClient),
}).extend(cv.polling_component_schema("60s")))

async def to_code(config):
    for conf in config:
        var = cg.new_Pvariable(conf[CONF_ID])
        await cg.register_component(var, conf)
        await ble_client.register_ble_node(var, conf)

