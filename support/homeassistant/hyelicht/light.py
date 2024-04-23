# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
# SPDX-FileCopyrightText: 2024 Eike Hein <sho@eikehein.com>

"""Platform for light integration."""

from __future__ import annotations

import aiohttp
import asyncio
import logging

import homeassistant.helpers.config_validation as cv
from homeassistant.helpers.device_registry import DeviceInfo
from homeassistant.components.light import (ATTR_BRIGHTNESS,
                                            ATTR_RGB_COLOR, ColorMode,
                                            LightEntityFeature,
                                            LightEntity)
from homeassistant.const import CONF_HOST, CONF_PORT

from .const import DOMAIN

_LOGGER = logging.getLogger(__name__)

async def async_setup_entry(hass, entry, async_add_entities):
    config = entry.data

    _LOGGER.info("Adding Hyelicht.")
    async_add_entities([HyelichtLight(config)], True)

async def async_unload_entry(hass, entry) -> bool:
    _LOGGER.info("Removing Hyelicht.")
    return True

class HyelichtLight(LightEntity):
    """Representation of a Hyelicht shelf."""

    _attr_has_entity_name = True
    _attr_name = None
    _attr_icon = "mdi:bookshelf"

    _attr_unique_id = "hyelicht"

    _attr_supported_color_modes = {ColorMode.RGB}
    _attr_color_mode = ColorMode.RGB

    def __init__(self, config) -> None:
        """Initialize a Hyelicht shelf."""
        self._host = config.get(CONF_HOST)
        self._port = config.get(CONF_PORT)
        self._state = {
            'enabled': False,
            'brightness': 0.0,
            'averageColor': '#ffffff'
        }

    @property
    def device_info(self):
        """Return the device info for this Hyelicht shelf."""
        return DeviceInfo(
            identifiers={(DOMAIN, f"{self._host}:{self._port}")},
            name="Hyelicht",
            manufacturer="Hyerim and Eike",
            model=DOMAIN,
            sw_version="0.1"
        )

    @property
    def brightness(self):
        """Return the brightness of this Hyelicht shelf."""
        return max(1, int(self._state['brightness'] * 255))

    @property
    def is_on(self) -> bool | None:
        """Return the average color of this Hyelicht shelf."""
        return self._state['enabled']

    @property
    def rgb_color(self):
        """ Return the average color of this Hyelicht shelf."""
        return hex_to_rgb(self._state['averageColor'])

    async def async_turn_on(self, **kwargs):
        """Instruct this Hyelicht shelf to turn on."""

        async with aiohttp.ClientSession() as session:
            try:
                if ATTR_RGB_COLOR in kwargs:
                    url = f"http://{self._host}:{self._port}/v1/shelf/averageColor"
                    payload = {'averageColor': rgb_to_hex(kwargs[ATTR_RGB_COLOR])}

                    async with session.put(url, json=payload) as response:
                        if response.status == 200:
                            self._state['enabled'] = True

                            response_json = await response.json()
                            self._state['averageColor'] = response_json['averageColor']
                        else:
                            _LOGGER.error(f"Failed to turn on Hyelicht shelf, status: {response.status}")

                if ATTR_BRIGHTNESS in kwargs:
                    url = f"http://{self._host}:{self._port}/v1/shelf/brightness"
                    brightness = kwargs.get(ATTR_BRIGHTNESS) / 255.0
                    payload = {'brightness': brightness}

                    async with session.put(url, json=payload) as response:
                        if response.status == 200:
                            if (brightness > 0.0):
                                self._state['enabled'] = True

                            response_json = await response.json()
                            self._state['brightness'] = response_json['brightness']
                        else:
                            _LOGGER.error(f"Failed to turn on Hyelicht shelf, status: {response.status}")

                if not ATTR_RGB_COLOR in kwargs and not ATTR_BRIGHTNESS in kwargs:
                    url = f"http://{self._host}:{self._port}/v1/shelf/enabled"
                    payload = {'enabled': True}

                    async with session.put(url, json=payload) as response:
                        if response.status == 200:
                            response_json = await response.json()
                            self._state['enabled'] = response_json['enabled']
                        else:
                            _LOGGER.error(f"Failed to turn off Hyelicht shelf, status: {response.status}")
            except aiohttp.ClientError as e:
                _LOGGER.error(f"HTTP request to Hyelicht shelf failed: {str(e)}")

    async def async_turn_off(self, **kwargs):
        """Instruct this Hyelicht shelf to turn off."""

        url = f"http://{self._host}:{self._port}/v1/shelf/enabled"
        payload = {'enabled': False}

        async with aiohttp.ClientSession() as session:
            try:
                async with session.put(url, json=payload) as response:
                    if response.status == 200:
                        response_json = await response.json()
                        self._state['enabled'] = response_json['enabled']
                    else:
                        _LOGGER.error(f"Failed to turn off Hyelicht shelf, status: {response.status}")
            except aiohttp.ClientError as e:
                _LOGGER.error(f"HTTP request to Hyelicht shelf failed: {str(e)}")

    async def async_update(self):
        """Fetch new state data for this Hyelicht shelf."""

        url = f"http://{self._host}:{self._port}/v1/shelf"

        async with aiohttp.ClientSession() as session:
            try:
                async with session.get(url) as response:
                    if response.status == 200:
                        self._state = await response.json()
                    else:
                        self._state = None
                        _LOGGER.error(f"Failed to fetch data for {self._name}, status: {response.status}")
            except aiohttp.ClientError as e:
                self._state = None
                _LOGGER.error(f"Error fetching data from {self._name}: {str(e)}")

def rgb_to_hex(rgb_color):
    r, g, b = (rgb_color[0], rgb_color[1], rgb_color[2])
    return f'#{r:02x}{g:02x}{b:02x}'

def hex_to_rgb(hex_color):
    hex_color = hex_color.lstrip('#')
    return tuple(int(hex_color[i:i+2], 16) for i in (0, 2, 4))
