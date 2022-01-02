# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
# SPDX-FileCopyrightText: 2021-2022 Eike Hein <sho@eikehein.com>

import json
import logging
import requests

from functions.colors import convert_rgb_xy, convert_xy

def hex_to_rgb(value):
    value = value.lstrip('#')
    lv = len(value)
    tup = tuple(int(value[i:i + lv // 3], 16) for i in range(0, lv, lv // 3))
    return list(tup)

def rgb_to_hex(rgb):
    return '#%02x%02x%02x' % tuple(rgb)

def discover(bridge_config, new_lights):
  pass

def set_light(address, light, data):
    logging.debug("hyelicht: <set_light> invoked!")
    
    headers = {"Content-Type": "application/json"}

    for key, value in data.items():
        if key == "bri":
            data = { "brightness": str(round(float(data["bri"] / 255), 2)) }
            url = "http://" + address["ip"] + "/v1/shelf/brightness"
            requests.put(url, data=json.dumps(data), headers=headers, timeout=3)
        elif key == "on":
            data = { "enabled": data["on"] }
            url = "http://" + address["ip"] + "/v1/shelf/enabled"
            requests.put(url, data=json.dumps(data), headers=headers, timeout=3)
        elif key == "xy":
            rgb = convert_xy(value[0], value[1], light["state"]["bri"])
            data = { "averageColor": rgb_to_hex(rgb) }
            url = "http://" + address["ip"] + "/v1/shelf/averageColor"
            requests.put(url, data=json.dumps(data), headers=headers, timeout=3)

def get_light_state(address, light):
    logging.debug("hyelicht: <get_light_state> invoked!")
    
    state = {}
    
    url = "http://" + address["ip"] + "/v1/shelf"
    data = json.loads(requests.get(url, timeout=3).text)
    
    state['on'] = data["enabled"]
    state['bri'] = round(float(data["brightness"]) * 254)

    rgb = hex_to_rgb(data["averageColor"])
    state["xy"] = convert_rgb_xy(rgb[0], rgb[1], rgb[2])
    state["colormode"] = "xy"

    return state
