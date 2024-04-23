# SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
# SPDX-FileCopyrightText: 2024 Eike Hein <sho@eikehein.com>

"""Config flow for Hyelicht integration."""

from __future__ import annotations

import aiohttp
import asyncio
import logging
from typing import Any

import voluptuous as vol

import homeassistant.helpers.config_validation as cv
from homeassistant.config_entries import ConfigFlow, ConfigFlowResult
from homeassistant.const import CONF_HOST, CONF_PORT
from homeassistant.core import HomeAssistant
from homeassistant.exceptions import HomeAssistantError

from .const import DOMAIN

_LOGGER = logging.getLogger(__name__)

STEP_USER_DATA_SCHEMA = vol.Schema(
    {
        vol.Required(CONF_HOST, default='localhost'): cv.string,
        vol.Required(CONF_PORT, default=8082): cv.port,
    }
)


async def validate_input(hass: HomeAssistant, data: dict[str, Any]) -> dict[str, Any]:
    """Validate the user input for the connection settings."""

    url = f"http://{data[CONF_HOST]}:{data[CONF_PORT]}/v1/shelf"

    async with aiohttp.ClientSession() as session:
        try:
            async with session.get(url) as response:
                if response.status != 200:
                    _LOGGER.error(f"Failed to fetch data for {DOMAIN}, status: {response.status}")
                    raise CannotConnect
        except aiohttp.ClientError as e:
            _LOGGER.error(f"Error fetching data from {DOMAIN}: {str(e)}")
            raise CannotConnect

    return {"title": "Hyelicht"}


class ConfigFlow(ConfigFlow, domain=DOMAIN):
    """Handle config flow for Hyelicht."""

    VERSION = 0
    MINOR_VERSION = 1

    async def async_step_user(
        self, user_input: dict[str, Any] | None = None
    ) -> ConfigFlowResult:
        await self.async_set_unique_id("hyelicht")
        self._abort_if_unique_id_configured()

        errors: dict[str, str] = {}
        if user_input is not None:
            try:
                info = await validate_input(self.hass, user_input)
            except CannotConnect:
                errors["base"] = "cannot_connect"
            except Exception:  # pylint: disable=broad-except
                _LOGGER.exception("Unexpected exception")
                errors["base"] = "unknown"
            else:
                return self.async_create_entry(title=info["title"], data=user_input)

        return self.async_show_form(
            step_id="user", data_schema=STEP_USER_DATA_SCHEMA, errors=errors
        )


class CannotConnect(HomeAssistantError):
    """Error to indicate we cannot connect."""
