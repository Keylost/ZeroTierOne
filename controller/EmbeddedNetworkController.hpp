/*
 * ZeroTier One - Network Virtualization Everywhere
 * Copyright (C) 2011-2015  ZeroTier, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ZT_SQLITENETWORKCONTROLLER_HPP
#define ZT_SQLITENETWORKCONTROLLER_HPP

#include <stdint.h>

#include <string>
#include <map>
#include <vector>
#include <set>
#include <list>

#include "../node/Constants.hpp"

#include "../node/NetworkController.hpp"
#include "../node/Mutex.hpp"
#include "../node/Utils.hpp"
#include "../node/Address.hpp"
#include "../node/InetAddress.hpp"

#include "../osdep/OSUtils.hpp"
#include "../osdep/Thread.hpp"

#include "../ext/json/json.hpp"

#include "JSONDB.hpp"

namespace ZeroTier {

class Node;

class EmbeddedNetworkController : public NetworkController
{
public:
	EmbeddedNetworkController(Node *node,const char *dbPath);
	virtual ~EmbeddedNetworkController();

	virtual NetworkController::ResultCode doNetworkConfigRequest(
		const InetAddress &fromAddr,
		const Identity &signingId,
		const Identity &identity,
		uint64_t nwid,
		const Dictionary<ZT_NETWORKCONFIG_METADATA_DICT_CAPACITY> &metaData,
		NetworkConfig &nc);

	unsigned int handleControlPlaneHttpGET(
		const std::vector<std::string> &path,
		const std::map<std::string,std::string> &urlArgs,
		const std::map<std::string,std::string> &headers,
		const std::string &body,
		std::string &responseBody,
		std::string &responseContentType);
	unsigned int handleControlPlaneHttpPOST(
		const std::vector<std::string> &path,
		const std::map<std::string,std::string> &urlArgs,
		const std::map<std::string,std::string> &headers,
		const std::string &body,
		std::string &responseBody,
		std::string &responseContentType);
	unsigned int handleControlPlaneHttpDELETE(
		const std::vector<std::string> &path,
		const std::map<std::string,std::string> &urlArgs,
		const std::map<std::string,std::string> &headers,
		const std::string &body,
		std::string &responseBody,
		std::string &responseContentType);

private:
	static void _circuitTestCallback(ZT_Node *node,ZT_CircuitTest *test,const ZT_CircuitTestReport *report);

	// Gathers a bunch of statistics about members of a network, IP assignments, etc. that we need in various places
	// This does lock _networkMemberCache_m
	struct _NetworkMemberInfo
	{
		_NetworkMemberInfo() : authorizedMemberCount(0),activeMemberCount(0),totalMemberCount(0),mostRecentDeauthTime(0) {}
		std::set<Address> activeBridges;
		std::set<InetAddress> allocatedIps;
		unsigned long authorizedMemberCount;
		unsigned long activeMemberCount;
		unsigned long totalMemberCount;
		uint64_t mostRecentDeauthTime;
	};
	void _getNetworkMemberInfo(uint64_t now,uint64_t nwid,_NetworkMemberInfo &nmi);

	// These init objects with default and static/informational fields
	inline void _initMember(nlohmann::json &member)
	{
		if (!member.count("authorized")) member["authorized"] = false;
		if (!member.count("authHistory")) member["authHistory"] = nlohmann::json::array();
 		if (!member.count("ipAssignments")) member["ipAssignments"] = nlohmann::json::array();
		if (!member.count("recentLog")) member["recentLog"] = nlohmann::json::array();
		if (!member.count("activeBridge")) member["activeBridge"] = false;
		if (!member.count("tags")) member["tags"] = nlohmann::json::array();
		if (!member.count("capabilities")) member["capabilities"] = nlohmann::json::array();
		if (!member.count("creationTime")) member["creationTime"] = OSUtils::now();
		if (!member.count("noAutoAssignIps")) member["noAutoAssignIps"] = false;
		if (!member.count("revision")) member["revision"] = 0ULL;
		if (!member.count("enableBroadcast")) member["enableBroadcast"] = true;
		member["objtype"] = "member";
	}
	inline void _initNetwork(nlohmann::json &network)
	{
		if (!network.count("private")) network["private"] = true;
		if (!network.count("creationTime")) network["creationTime"] = OSUtils::now();
		if (!network.count("name")) network["name"] = "";
		if (!network.count("multicastLimit")) network["multicastLimit"] = (uint64_t)32;
		if (!network.count("v4AssignMode")) network["v4AssignMode"] = {{"zt",false}};
		if (!network.count("v6AssignMode")) network["v6AssignMode"] = {{"rfc4193",false},{"zt",false},{"6plane",false}};
		if (!network.count("authTokens")) network["authTokens"] = nlohmann::json::array();
		if (!network.count("capabilities")) network["capabilities"] = nlohmann::json::array();
		if (!network.count("ipAssignmentPools")) network["ipAssignmentPools"] = nlohmann::json::array();
		if (!network.count("rules")) {
			// If unspecified, rules are set to allow anything and behave like a flat L2 segment
			network["rules"] = {{
				{ "not",false },
				{ "or", false },
				{ "type","ACTION_ACCEPT" }
			}};
		}
		network["objtype"] = "network";
	}
	inline void _addNetworkNonPersistedFields(nlohmann::json &network,uint64_t now,const _NetworkMemberInfo &nmi)
	{
		network["clock"] = now;
		network["authorizedMemberCount"] = nmi.authorizedMemberCount;
		network["activeMemberCount"] = nmi.activeMemberCount;
		network["totalMemberCount"] = nmi.totalMemberCount;
	}
	inline void _addMemberNonPersistedFields(nlohmann::json &member,uint64_t now)
	{
		member["clock"] = now;
	}

	JSONDB _db;
	Mutex _db_m;

	Node *const _node;
	std::string _path;

	struct _CircuitTestEntry
	{
		ZT_CircuitTest *test;
		std::string jsonResults;
	};
	std::map< uint64_t,_CircuitTestEntry > _circuitTests;
	Mutex _circuitTests_m;

	std::map< std::pair<uint64_t,uint64_t>,uint64_t > _lastRequestTime;
	Mutex _lastRequestTime_m;
};

} // namespace ZeroTier

#endif
