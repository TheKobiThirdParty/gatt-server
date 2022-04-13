// Copyright 2017 Paul Nettle.
//
// This file is part of Gobbledegook.
//
// Gobbledegook is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Gobbledegook is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Gobbledegook.  If not, see <http://www.gnu.org/licenses/>.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// >>
// >>>  INSIDE THIS FILE
// >>
//
// This file contains various functions for interacting with Bluetooth Management interface, which provides adapter configuration.
//
// >>
// >>>  DISCUSSION
// >>
//
// We only cover the basics here. If there are configuration features you need that aren't supported (such as configuring BR/EDR),
// then this would be a good place for them.
//
// Note that this class relies on the `HciAdapter`, which is a very primitive implementation. Use with caution.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <string.h>

#include "Mgmt.h"
#include "Logger.h"
#include "Utils.h"

namespace ggk {

// Construct the Mgmt device
//
// Set `controllerIndex` to the zero-based index of the device as recognized by the OS. If this parameter is omitted, the index
// of the first device (0) will be used.
Mgmt::Mgmt(uint16_t controllerIndex)
: controllerIndex(controllerIndex)
{
	HciAdapter::getInstance().sync(controllerIndex);
}

// Set the adapter name and short name
//
// The inputs `name` and `shortName` may be truncated prior to setting them on the adapter. To ensure that `name` and
// `shortName` conform to length specifications prior to calling this method, see the constants `kMaxAdvertisingNameLength` and
// `kMaxAdvertisingShortNameLength`. In addition, the static methods `truncateName()` and `truncateShortName()` may be helpful.
//
// Returns true on success, otherwise false
bool Mgmt::setName(std::string name, std::string shortName)
{
	// Ensure their lengths are okay
	name = truncateName(name);
	shortName = truncateShortName(shortName);

	struct SRequest : HciAdapter::HciHeader
	{
		char name[249];
		char shortName[11];
	} __attribute__((packed));

	SRequest request;
	request.code = Mgmt::ESetLocalNameCommand;
	request.controllerId = controllerIndex;
	request.dataSize = sizeof(SRequest) - sizeof(HciAdapter::HciHeader);

	memset(request.name, 0, sizeof(request.name));
	snprintf(request.name, sizeof(request.name), "%s", name.c_str());

	memset(request.shortName, 0, sizeof(request.shortName));
	snprintf(request.shortName, sizeof(request.shortName), "%s", shortName.c_str());

	if (!HciAdapter::getInstance().sendCommand(request))
	{
		Logger::warn(SSTR << "  + Failed to set name");
		return false;
	}

	return true;
}

// Set a setting state to 'newState'
//
// Many settings are set the same way, this is just a convenience routine to handle them all
//
// Returns true on success, otherwise false
bool Mgmt::setState(uint16_t commandCode, uint16_t controllerId, uint8_t newState)
{
	struct SRequest : HciAdapter::HciHeader
	{
		uint8_t state;
	} __attribute__((packed));

	SRequest request;
	request.code = commandCode;
	request.controllerId = controllerId;
	request.dataSize = sizeof(SRequest) - sizeof(HciAdapter::HciHeader);
	request.state = newState;

	if (!HciAdapter::getInstance().sendCommand(request))
	{
		Logger::warn(SSTR << "  + Failed to set " << HciAdapter::kCommandCodeNames[commandCode] << " state to: " << static_cast<int>(newState));
		return false;
	}

	return true;
}

// Set the powered state to `newState` (true = powered on, false = powered off)
//
// Returns true on success, otherwise false
bool Mgmt::setPowered(bool newState)
{
	return setState(Mgmt::ESetPoweredCommand, controllerIndex, newState ? 1 : 0);
}

// Set the BR/EDR state to `newState` (true = enabled, false = disabled)
//
// Returns true on success, otherwise false
bool Mgmt::setBredr(bool newState)
{
	return setState(Mgmt::ESetBREDRCommand, controllerIndex, newState ? 1 : 0);
}

// Set the Secure Connection state (0 = disabled, 1 = enabled, 2 = secure connections only mode)
//
// Returns true on success, otherwise false
bool Mgmt::setSecureConnections(uint8_t newState)
{
	return setState(Mgmt::ESetSecureConnectionsCommand, controllerIndex, newState);
}

// Set the bondable state to `newState` (true = enabled, false = disabled)
//
// Returns true on success, otherwise false
bool Mgmt::setBondable(bool newState)
{
	return setState(Mgmt::ESetBondableCommand, controllerIndex, newState ? 1 : 0);
}

// Set the connectable state to `newState` (true = enabled, false = disabled)
//
// Returns true on success, otherwise false
bool Mgmt::setConnectable(bool newState)
{
	return setState(Mgmt::ESetConnectableCommand, controllerIndex, newState ? 1 : 0);
}

// Set the LE state to `newState` (true = enabled, false = disabled)
//
// Returns true on success, otherwise false
bool Mgmt::setLE(bool newState)
{
	return setState(Mgmt::ESetLowEnergyCommand, controllerIndex, newState ? 1 : 0);
}

// Set the advertising state to `newState` (0 = disabled, 1 = enabled (with consideration towards the connectable setting),
// 2 = enabled in connectable mode).
//
// Returns true on success, otherwise false
bool Mgmt::setAdvertising(uint8_t newState)
{
	return setState(Mgmt::ESetAdvertisingCommand, controllerIndex, newState);
}

// Start advertising with custom data
// Advertisement packet will contain: flags, shortName, uuid
bool Mgmt::addAdvertising()
{
	constexpr size_t ADVERTISING_MAX_DATALEN = 31;
	constexpr size_t SCAN_RSP_MAX_DATALEN = 17;

	struct SRequest : HciAdapter::HciHeader
	{
		uint8_t  instance;
		uint32_t flags;
		uint16_t duration;
		uint16_t timeout;
		uint8_t  advDataLen;
		uint8_t  scanRspLen;
		uint8_t  data[ADVERTISING_MAX_DATALEN];
		uint8_t  scanRspData[SCAN_RSP_MAX_DATALEN];
	} __attribute__((packed));

	SRequest request{};
	request.code = Mgmt::EAddAdvertisingCommand;
	request.controllerId = controllerIndex;
	request.dataSize = sizeof(SRequest) - sizeof(HciAdapter::HciHeader);

	request.instance = 0x01;
	// Connectable && Discoverable, see Bluez/lib/mgmt.h
	// setting flags results in 0x0D (Invalid Parameters)
	request.flags = 0;
	request.duration = 0;
	request.timeout = 0;

	request.advDataLen = ADVERTISING_MAX_DATALEN;
	request.scanRspLen = SCAN_RSP_MAX_DATALEN;

	// AD Data 1 <<Flags>>
	// length
	request.data[0] = 0x02;
	// type --> flags
	request.data[1] = 0x01;
	// BR/EDR not supported | General discoverable mode
	request.data[2] = 0x06;

	// AD Data 2 <<Manufacturer Specific Data>>
	// length
	request.data[3] = 0x1B;
	// type --> Manufacturer Specific data
	request.data[4] = 0xFF;
	// Company : Robert Bosch GmbH
	request.data[5] = 0xA6;
	request.data[6] = 0x02;
	// Model
	request.data[7] = 0x00;
	// PCBA_Version
	request.data[8] = 0x00;
	// Error_Code_Status
	request.data[9] = 0x00;
	// Battery
	request.data[10] = 100u;
	// Serial number
	request.data[11] = 0xB0;
	request.data[12] = 0xD0;
	request.data[13] = 0x56;
	request.data[14] = 0xF2;
	request.data[15] = 0xB5;
	request.data[16] = 0x12;
	request.data[17] = 0x00;
	request.data[18] = 0x00;
	request.data[19] = 0x00;
	request.data[20] = 0x00;
	// Reserved 21-31

	// length
	request.scanRspData[0] = 0x10;
	// Complete local name
	request.scanRspData[1] = 0x09;
	// SKYWALKER-XXXXX
	std::string local_name = "SKYWALKER-XXXXX";
	memcpy(&request.scanRspData[2], local_name.c_str(), SCAN_RSP_MAX_DATALEN-2);

	if (!HciAdapter::getInstance().sendCommand(request))
	{
		Logger::warn(SSTR << "  + Failed to start advertising with UUID");
		return false;
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------------------
// Utilitarian
// ---------------------------------------------------------------------------------------------------------------------------------

// Truncates the string `name` to the maximum allowed length for an adapter name. If `name` needs no truncation, a copy of
// `name` is returned.
std::string Mgmt::truncateName(const std::string &name)
{
	if (name.length() <= kMaxAdvertisingNameLength)
	{
		return name;
	}

	return name.substr(0, kMaxAdvertisingNameLength);
}

// Truncates the string `name` to the maximum allowed length for an adapter short-name. If `name` needs no truncation, a copy
// of `name` is returned.
std::string Mgmt::truncateShortName(const std::string &name)
{
	if (name.length() <= kMaxAdvertisingShortNameLength)
	{
		return name;
	}

	return name.substr(0, kMaxAdvertisingShortNameLength);
}

}; // namespace ggk
