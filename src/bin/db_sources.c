/* zrpc: RPC database+server for Zentific
 * Copyright (C) 2007, 2008, 2009, 2010, 2011 Zentific LLC and AUTHORS
 *
 * All rights reserved.
 * Use is subject to license terms.
 *
 * Please visit http://zentific.com for news and updates
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */
#include "zentific.h"
#include <dlfcn.h>
#include <Ecore.h>

#include "db_sources.h"

static Eina_Bool db_sources_cb(Eina_Module *m, zentific_config *conf){
	const char *filename;
	size_t fl, ml;

	filename = eina_module_file_get(m);
	filename = strrchr(filename, '/') + 1;
	fl = strlen(conf->DB_MODULE);
	ml = strlen(filename);
	if ((ml != fl + 3) ||(strcmp(filename + (ml - 3), ".so") || strncmp(filename, conf->DB_MODULE, fl)))
		return EINA_FALSE;
	return EINA_TRUE;
}

static void
db_source_init_cb(DB *data, Eina_Bool success){
	if (success){
		extern DB *db;
		db = data;
		INF("DB Source successfully initialized.");
	}
	ecore_main_loop_quit();
}

Eina_Bool db_init(Zrpcdb_Connect_Cb cb, Ecore_Event_Handler_Cb error_cb, Ecore_Event_Handler_Cb result_cb, zentific_config *conf){
	DB *db;
	Eina_Array *a;
	Eina_Module *m;


	a = eina_module_list_get(NULL, conf->DB_MODULES_PATH, EINA_FALSE, (Eina_Module_Cb)db_sources_cb, conf);
	if (!a) {
		ERR("DB module '%s.so' does not exist in '%s'", conf->DB_MODULE, conf->DB_MODULES_PATH);
		//then the path doesnt exist
		//fail early.
		return EINA_FALSE;
	}

	/* use default cb if none passed (used on startup of zrpc) */
	if (!cb) cb = db_source_init_cb;
	/* attempt to load the module */
	m = eina_array_pop(a);
	eina_array_free(a);
	INF("Loading module %s", eina_module_file_get(m));
	if (!eina_module_load(m)) {
		ERR("Could not load module '%s.so' from %s - Reason: '%s.'", conf->DB_MODULE, conf->DB_MODULES_PATH, dlerror());
		eina_module_free(m);
		return EINA_FALSE;
	}

	/* if we succesfully loaded the library, then grab the functions */
	INF("Creating database object");

	if (!(db = calloc(1, sizeof(DB)))){
		ERR("Cannot allocate memory for new database object.");
		eina_module_free(m);
		return EINA_FALSE;
	}

	db->library = m;

	/* generate all of the necessary references */
	INF("Building database object from db module.");
	INF("Initializing basic db functions.");



#define LOAD(X) \
	do \
	if (!(db->X = eina_module_symbol_get(m, S(X)))){ \
		ERR("Could not load function '%s' from DB module.", S(X)); \
		db_uninit(db); \
		return EINA_FALSE; \
	} \
	while (0)

	/* this looks way more boring, but is a lot easier to maintain.
	 * simply add a LOAD line with the db function name and you're done!
	 * to regenerate a set of macros from a header file, simply run this:
	 */
	//cat include/zrpcdb_X.h |sed 's/.* \([a-zA-Z]*\)(.*/LOAD(\1);/'|sed 's/.* \*\([a-zA-Z]*\)(.*/LOAD(\1);/'|grep LOAD

	/* API Function Loading */
	INF("Initializing DB private functions...");
	LOAD(init);
	LOAD(uninit);
	/*API session Functions*/
	INF("Initializing DB session functions...");
	LOAD(getLoggedInUsers);
	LOAD(getLoggedInUsernames);
	LOAD(getSessions);
	LOAD(getSession);
	LOAD(setSession);
	LOAD(setLastLogin);
	LOAD(deauthSession);
	LOAD(removeSession);
	LOAD(getUserBySession);
	LOAD(getUserNameBySession);


	/*API Platform Namespace Functions*/
	INF("Initializing DB platform functions...");
	LOAD(isControlModuleForPlatform);
	LOAD(isControlModuleForPlatformByMid);
	LOAD(isControlModuleForPlatformById);
	LOAD(isControlModuleForPlatformByName);
	LOAD(isControlModule);
	LOAD(isControlModuleByName);
	LOAD(isPlatform);
	LOAD(isPlatformByName);
	LOAD(enableControlModule);
	LOAD(disableControlModule);
	LOAD(enablePlatform);
	LOAD(disablePlatform);
	LOAD(getPlatformId);
	LOAD(getPlatforms);
	LOAD(getPlatformsFull);
	LOAD(getPlatform);
	LOAD(getPlatformByName);
	LOAD(addPlatform);
	LOAD(modifyPlatform);
	LOAD(getPlatformConfig);
	LOAD(getControlModuleConfig);
	LOAD(getControlModuleId);
	LOAD(getControlModuleName);
	LOAD(getControlModuleNameByNetwork);
	LOAD(getControlModuleNameByStorage);
	LOAD(getControlModules);
	LOAD(getStorageControlModulesFull);
	LOAD(getNetworkControlModulesFull);
	LOAD(getPlatformControlModulesFull);
	LOAD(getPlatformControlModules);
	LOAD(getStorageControlModules);
	LOAD(getNetworkControlModules);
	LOAD(getControlModulesByPlatform);
	LOAD(getControlModulesFull);
	LOAD(getControlModule);
	LOAD(getControlModuleByName);
	LOAD(getPlatformIdByVm);
	LOAD(getPlatformFeatures);

	/*API Scheduler Namespace Functions*/
	INF("Initializing DB scheduler functions...");
	LOAD(isJob);
	LOAD(getJob);
	LOAD(getRecentJobs);
	LOAD(getRecentJobsByUID);
	LOAD(getJobUID);
	LOAD(getJobsByIds);
	LOAD(getJobs);
	LOAD(getJobsByUID);
	LOAD(getJobsByUUID);
	LOAD(getJobsStatus);
	LOAD(getJobStatus);
	LOAD(addJob);
	LOAD(getSchedulerAddressByModuleName);
	LOAD(getSchedulerAddressByJob);
	LOAD(getSchedulerIdByModuleName);
	LOAD(getSchedulerIdByJob);
	LOAD(getSchedulerIdByPlatform);
	LOAD(fetchJobs);
	LOAD(updateJob);
	LOAD(getModuleConfigByNode);
	LOAD(getModuleConfigByVm);
	LOAD(getModuleConfig);
	LOAD(getBlobConfig);
	LOAD(registerPlatform);
	LOAD(registerControlModule);
	LOAD(registerScheduler);
	LOAD(getModuleID);
	LOAD(getPlatformID);
	LOAD(isModule);
	LOAD(isPlatformNameByScheduler);
	LOAD(clearControlModules);

	/*API USER Functions*/
	INF("Initializing DB user functions...");
	LOAD(getUser);
	LOAD(isUserUID);
	LOAD(setUserType);
	LOAD(getUserType);
	LOAD(getUsername);
	LOAD(getSessionsByUID);
	LOAD(isVMowner);
	LOAD(isNODEowner);
	LOAD(setUserAttribute);
	LOAD(isActive);
	LOAD(removeUser);
	LOAD(setPassword);
	LOAD(getPrivilege);
	LOAD(grantPrivilege);
	LOAD(revokePrivilege);
	LOAD(modifyUser);
	LOAD(isUserName);
	LOAD(getUserUID);
	LOAD(getActiveUserUID);
	LOAD(getUserTypeByName);
	LOAD(getUserByName);
	LOAD(getSessionsByName);
	LOAD(isVMownerByName);
	LOAD(isNODEownerByName);
	LOAD(setUserAttributeByName);
	LOAD(isActiveByName);
	LOAD(removeUserByName);
	LOAD(setPasswordByName);
	LOAD(getPrivilegeByName);
	LOAD(grantPrivilegeByName);
	LOAD(revokePrivilegeByName);
	LOAD(modifyUserByName);
	LOAD(getLanguages);
	LOAD(getLanguageIds);
	LOAD(getLanguageCode);
	LOAD(getLanguageId);
	LOAD(addUser);
	LOAD(getAllUsers);
	LOAD(getAllUsersFull);
	LOAD(getAllUsersFullSorted);
	LOAD(getAllUsersSorted);
	LOAD(getUserCount);


	/*API USER_GROUP Functions*/
	INF("Initializing DB user group functions...");
	LOAD(isUserGroup);
	LOAD(modifyUserGroup);
	LOAD(removeUserGroup);
	LOAD(getUserGroup);
	LOAD(getUserGroupName);
	LOAD(getUserGroupMembers);
	LOAD(getUserGroupMembersFull);
	LOAD(addToUserGroup);
	LOAD(removeFromUserGroup);
	LOAD(renameUserGroup);
	LOAD(isUserGroupMember);
	LOAD(isUserGroupByName);
	LOAD(modifyUserGroupByName);
	LOAD(removeUserGroupByName);
	LOAD(getUserGroupByName);
	LOAD(getUserGroupId);
	LOAD(getUserGroupMembersByName);
	LOAD(renameUserGroupByName);
	LOAD(addToUserGroupByName);
	LOAD(removeFromUserGroupByName);
	LOAD(isUserGroupMemberByName);
	LOAD(addToUserGroupByNameAndUsername);
	LOAD(removeFromUserGroupByNameAndUsername);
	LOAD(isUserGroupMemberByNameAndUsername);
	LOAD(addUserGroup);
	LOAD(getAllUserGroups);
	LOAD(getAllUserGroupNames);
	LOAD(getAllUserGroupsFull);
	LOAD(getUserGroupsByUID);
	LOAD(getUserGroupsByName);
	LOAD(getUserGroupNamesByUID);
	LOAD(getUserGroupNamesByName);
	LOAD(getAllUserGroupsSorted);
	LOAD(getAllUserGroupsFullSorted);
	LOAD(getUserGroupCount);

	/*API auth Functions*/
	INF("Initializing DB auth functions...");
	LOAD(getAuth);
	LOAD(getAuthByName);

	/* API NETWORK Functions */
	LOAD(getAllNetworksFull);
	LOAD(getAllNetworks);
	LOAD(getAllNetworksFullSorted);
	LOAD(getAllNetworksSorted);
	LOAD(getAllNetworksByType);
	LOAD(getNetwork);
	LOAD(getNetworkVlans);
	LOAD(removeNetwork);
	LOAD(getNodesByNetworkVlan);
	LOAD(getNodesByNetwork);
	LOAD(getNetworksByNode);
	LOAD(defineNetwork);
	LOAD(modifyNetwork);
	LOAD(isNetwork);
	LOAD(isNetworkId);
	LOAD(addNetIf);
	LOAD(attachNetIf);
	LOAD(detachNetIf);
	LOAD(modifyNetIf);
	LOAD(removeNetIf);
	LOAD(detachAllNetIfsByNetwork);
	LOAD(getNetworkControlModule);
	LOAD(setNetworkControlModule);
	LOAD(detachNetworkFromNode);
	LOAD(attachNetworkToNode);
	LOAD(getVmsByNetwork);
	LOAD(getNetworksByVm);
	LOAD(isNetworkInUse);
	LOAD(getNetworkByNetIf);
	LOAD(getNetworkCount);


	/* API Storage Functions */
	LOAD(getAllStorage);
	LOAD(getAllStorageFull);
	LOAD(getAllStorageFullSorted);
	LOAD(getAllStorageSorted);
	LOAD(getAllStorageByType);
	LOAD(getStorageHost);
	LOAD(getStorage);
	LOAD(removeStorage);
	LOAD(getNodesByStorage);
	LOAD(getStorageByNode);
	LOAD(defineStorage);
	LOAD(modifyStorage);
	LOAD(isStorageId);
	LOAD(isStorage);
	LOAD(addDisk);
	LOAD(enableDisk);
	LOAD(disableDisk);
	LOAD(attachDisk);
	LOAD(detachDisk);
	LOAD(modifyDisk);
	LOAD(removeDisk);
	LOAD(detachAllDisksByStorage);
	LOAD(getStorageControlModule);
	LOAD(setStorageControlModule);
	LOAD(detachStorageFromNode);
	LOAD(attachStorageToNode);
	LOAD(getVmsByStorage);
	LOAD(getStorageByVm);
	LOAD(isStorageInUse);
	LOAD(getStorageByDisk);
	LOAD(getStorageIdByName);
	LOAD(getAllStorageTypes);
	LOAD(getStorageCount);

	/*API NETIF Functions*/
	INF("Initializing DB net functions...");
	LOAD(getVMvifs);
	LOAD(getVMvifByMac);
	LOAD(updateNetIfStats);
	LOAD(getNumVMdisks);
	LOAD(getVmDisk);
	LOAD(getNumVMvifs);
	LOAD(createNetIf);
	LOAD(isNetIf);
	LOAD(isNetIfByMAC);
	LOAD(isNetIfInUse);
	LOAD(updateNetIfConfig);
	LOAD(addNetIf);
	LOAD(modifyNetIf);
	LOAD(removeNetIf);
	LOAD(attachNetIf);
	LOAD(detachNetIf);
	LOAD(detachAllNetIfsByNetwork);
	LOAD(pruneNetIfStats);
	LOAD(getNetworkControlModule);
	LOAD(setNetworkControlModule);
	LOAD(getVMvifsByNetwork);


	/*API NODE Functions*/
	INF("Initializing DB node functions...");
	LOAD(getNodePlatform);
	LOAD(getNodeAddress);
	LOAD(getNodeByName);
	LOAD(getNodeAddress);
	LOAD(getNODEstat);
	LOAD(getNODEattribute);
	LOAD(getLatestNodeStat);
	LOAD(getNodeStats);
	LOAD(updateNodeStats);
	LOAD(updateNodeConfig);
	LOAD(getNodeControlModule);
	LOAD(setNodeConfiguredHost);
	LOAD(getNodeConfiguredHost);
	LOAD(createNode);
	LOAD(setNodeStatus);
	LOAD(setNodeControlModule);
	LOAD(setNodeControlModuleByName);
	LOAD(removeNode);
	LOAD(isNodeEnabled);
	LOAD(isNodeEnabledAndConfigured);
	LOAD(getAllUnconfiguredNodes);
	LOAD(getAllUnconfiguredNodesByUID);
	LOAD(getNodeConfiguredState);
	LOAD(isNode);
	LOAD(getNode);
	LOAD(getAllNodesFull);
	LOAD(getAllNodes);
	LOAD(getAllNodesByUID);
	LOAD(getAllNodesSorted);
	LOAD(getAllNodesByUIDSorted);
	LOAD(getNodeByHostname);
	LOAD(pruneNodeStats);
	LOAD(getNodeLogs);
	LOAD(getNodeDefaultVNCpass);
	LOAD(getNodeHVM);
	LOAD(getNodeName);
	LOAD(getNodeNameAndUuid);

	/*API DISK Functions*/
	INF("Initializing DB disk functions...");
	LOAD(isDisk);
	//LOAD(isDiskInUse);
	LOAD(createDisk);
	LOAD(updateDiskConfig);
	LOAD(getVMdisks);
	LOAD(getVMdisksByStorage);
	LOAD(updateDiskStats);
	LOAD(getDiskByStorage);
	LOAD(getDisksByStorage);
	LOAD(getVmByDisk);
	LOAD(pruneDiskStats);

	/*API VM Functions*/
	INF("Initializing DB vm functions...");
	LOAD(isVM);
	LOAD(isVmIgnored);
	LOAD(needsReboot);
	LOAD(setVmNeedsReboot);
	LOAD(getVMparentNode);
	LOAD(getVMparentNodeAndAddress);
	LOAD(updateVMparentNode);
	LOAD(createVM);
	LOAD(createVmTemplate);
	LOAD(getLatestRevision);
	LOAD(saveVMrevision);
	LOAD(getVMsSorted);
	LOAD(getVMs);
	LOAD(getVMsByUIDSorted);
	LOAD(getVMsByUID);
	LOAD(getAllVmsByNode);
	LOAD(getAllVmsByNodeByUID);
	LOAD(updateVmStats);
	LOAD(ignoreVm);
	LOAD(removeVm);
	LOAD(getVM);
	LOAD(updateVmConfig);
	LOAD(getAllUsersByVm);
	LOAD(getVMstat);
	LOAD(getVMattribute);
	LOAD(getTopBandwidthConsumers);
	LOAD(getTopCpuConsumers);
	LOAD(saveVmNotes);
	LOAD(getVmNotes);
	LOAD(getVMparentAddress);
	LOAD(getConsoleLockVmsBySession);
	LOAD(getConsoleType);
	LOAD(getConsoleLockSession);
	LOAD(hasConsoleLock);
	LOAD(acquireConsoleLock);
	LOAD(releaseConsoleLock);
	LOAD(saveGraphicalConsolePort);
	LOAD(getGraphicalConsolePort);
	LOAD(getConsoleNodeHost);
	LOAD(getConsoleClientHost);
	LOAD(getGraphicalConsoleTunnelPort);
	LOAD(saveTextConsolePort);
	LOAD(getTextConsolePort);
	LOAD(pruneVmStats);
	LOAD(getVmLogs);
	LOAD(hasGuestTools);
	LOAD(ztoolsUpdate);
	LOAD(setVmState);
	LOAD(updateVmTimestamp);
	LOAD(getVmByName);

	LOAD(getAllTemplates);
	LOAD(getAllTemplatesFull);
	LOAD(getTemplate);
	LOAD(getTemplateByName);
	LOAD(getTemplateConfig);
	LOAD(getTemplateConfigByName);
	LOAD(addTemplate);
	LOAD(modifyTemplate);

	/*API Zentific Functions*/
	INF("Initializing DB zentific functions...");
	LOAD(cleanDB);
	LOAD(getOperatingSystems);
	LOAD(getConfigValue);
	LOAD(getConfigs);
	LOAD(getConfig);
	LOAD(setConfig);
	LOAD(setBlobConfig);
	LOAD(getConfigsFull);
	LOAD(isFrontendPluginEnabled);
	LOAD(getAllPlugins);
	LOAD(getFrontendPlugins);
	LOAD(modifyPlugin);

	/* otherwise, it is good so initialize it */
	INF("Initializing db");

	/* report any initialization errors */
	db->initialized = db->init(db, cb, error_cb, result_cb, conf);
	if ( !db->initialized ) {
		ERR("Error initializing module %s", eina_module_file_get(m));
		db_uninit(db);
		return EINA_FALSE;
	}

	return EINA_TRUE;
}



int db_uninit(DB *db) {

	if (!db)
		return 1;

	if (db->initialized) db->uninit(db);
	eina_module_free(db->library);

	free(db);

	/* success */
	return 0;
}

