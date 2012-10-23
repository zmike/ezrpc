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

#ifndef ZENTIFIC_TYPES_H
#define ZENTIFIC_TYPES_H

#include "zentific.h"
#include <Eina.h>
#include <Ecore.h>
#include "Common.h"
typedef struct _DB DB;

typedef enum {
	USER_TYPE_SUPER_USER,
	USER_TYPE_MAINTENANCE,
	USER_TYPE_ADMIN,
	USER_TYPE_READ_ONLY_ADMIN,
	USER_TYPE_NONE,
	USER_TYPE_NODE
} Z_User_Type;

typedef struct {
	Eina_List *cbs;
	Eina_Hash *params;
	void *parent; /* parent object */
	void *retval; /* pointer to return */
	void *data; /* adding data pointers */
	void *data2; /* like a boss */
	Ecore_Cb free_func; /* for retval */
	Ecore_Cb cb;
	Ecore_Cb job;
	Eina_Error error; /* error code if necessary */
	unsigned int count; /* counter */
	Z_User_Type user_type; /* user type */
	Eina_Bool state : 1; /* state for switches/num rows */
	Eina_Bool ret : 1; /* value to return with */
	Eina_Bool ret_set : 1; /* if ret was set */
	Eina_Bool dead : 1; /* client is dead */
	Eina_Bool checked : 1; /* if call has been checked */
	Eina_Bool auth : 1; /* if call is authorized to continue */
	Eina_Bool sso : 1; /* single sign on enabled */
} Zentific_Ctx;
typedef void (*Z_Ctx_Cb)();

typedef enum {
	ZPLATFORM_GRAPHICALCONSOLE      = ( 1 << 1 ),
	ZPLATFORM_TEXTCONSOLE	   = ( 1 << 2 ),
	ZPLATFORM_MIGRATION	     = ( 1 << 3 ),
	ZPLATFORM_SNAPSHOTS	     = ( 1 << 4 ),
	ZPLATFORM_ONLINEMODIFY	  = ( 1 << 5 )
} zplatform_feature_flags;


typedef struct _zentific_config {
	const char *DB_MODULES_PATH;

	//Operation Tweaks
	int PORT;
	Eina_Log_Level LOG_LEVEL;
	int SESSION_TIMEOUT;
	int RETRY_TIMEOUT;
	int MAX_LOGIN_FAILURES;

	Eina_Bool SELF_HOST;
	Eina_Bool DAEMONIZE;
	Eina_Bool IS_VM;
	Eina_Bool USE_SSL;
	Eina_Bool XML_DEBUG;
	Eina_Bool GDB_BT;

	const char *UTILS_PATH;
	const char *VAR_PATH;
	const char *SSH_PATH;
	const char *MODULES_PATH;
	const char *DB_MODULE;
	const char *CREDSTORE_PUBKEY_PATH;

	const char *LISTEN;
	const char *MY_UUID;
	const char *SSL_PATH;
	const char *SSL_CERT;
	const char *KEY_PATH;
	const char *WEB_ROOT;
	const char *PID_FILE;
	const char *LOG_FILE;
	const char *CONF_FILE;
	Zshare_Log_Type LOG_TYPE;
	const char *LOG_PREFIX;
	const char *VM_CONF_PATH;
} zentific_config;


typedef struct _DB_config DB_config;
typedef void (*Zrpcdb_Connect_Cb)(DB *db, Eina_Bool success);


/* status of the execution of a job */
typedef enum {
	ZENSCHED_JOB_STATUS_INSERTED = 0,
	ZENSCHED_JOB_STATUS_FETCHED,
	ZENSCHED_JOB_STATUS_ENQUEUED,
	ZENSCHED_JOB_STATUS_EXECUTING,
	ZENSCHED_JOB_STATUS_COMPLETED,
	ZENSCHED_JOB_STATUS_CANCELLED,
	ZENSCHED_JOB_STATUS_ERROR,
	ZENSCHED_JOB_STATUS_LAUNCHING,
	ZENSCHED_JOB_STATUS_RESCHEDULED,
	ZENSCHED_JOB_STATUS_DEPENDFAIL
} Zensched_Job_Status;

/* default priorities for each job module */
typedef enum {
	ZENSCHED_JOB_PRIORITY_NONE = -1,
	ZENSCHED_JOB_PRIORITY_LOW = 1,
	ZENSCHED_JOB_PRIORITY_DEFAULT = 3,
	ZENSCHED_JOB_PRIORITY_HIGH = 5,
	ZENSCHED_JOB_PRIORITY_IMMEDIATE = 10
} Zensched_Job_Priority;

/* relative execution speed estimations */
typedef enum {
	ZENSCHED_JOB_SPEED_NONE = -1,
	ZENSCHED_JOB_SPEED_FAST = 1,
	ZENSCHED_JOB_SPEED_DEFAULT = 5,
	ZENSCHED_JOB_SPEED_SLOW = 10
} Zensched_Job_Speed;

/*
 * Describes a job action to perform.
 *
 * The priority can be provided in the database or left NULL. If it is NULL, then it is assumed to
 * be ZENSCHED_JOB_PRIORITY_DEFAULT. The only exceptions to this are the "cancel" job, which is always a
 * priority ZENSCHED_JOB_PRIORITY_IMMEDIATE, and a "domain_update" job, which is also a JOBPRIORITY_IMMEDIATE.
 * These two are executed right away because they affect other jobs that may have been ahead of them
 * in the execution queue.
 */
typedef struct {
	/* describes the job as it is listed in the database */
	int id;			 /* a unique identifier for this job (only needs to be unique to the source) */
	int runtype;		    /* 0 = normal job, 1 = scheduled one-time job, 2 = scheduled, repeating job */
	Zensched_Job_Priority priority; /* one of the JOBPRIORITY defines from above */
	int blocking;		   /* whether it will prevent other jobs from running */
	char *vm;		       /* UUID of target vm with dashes */
	char *node;		     /* UUID of target node with dashes */
	char *host;		     /* IP address with periods */
	char *extra;		    /* other job-related data */

	/* describes which job module will handle the execution */
	char *module_name;
	char *module_function;

	/* describes the job as it is executing */
	int pid;			/* PID of the job while it is executing */
	int time_started;	       /* UNIX timestamp */
	int time_finished;	      /* UNIX timestamp */
	Zensched_Job_Status status;     /* Job Status */
	int return_value;	       /* value returned from the execution */

	void *module;		   /* job module structure to use in executing the job */

	/** WARNING: These two are deprecated and will be removed in future versions. Write new
	 * jobmodules off of the "addon" jobmodule pattern (see jobmodules/addon/README.txt)
	 * FIXME access these via zensched callback ptr handed to module
	 */
	Eina_Hash *input_hash;
	char *input_string;	     /* input string for the job if needed */
	char *output_string;	    /* output string for job at completion */
	Zensched_Job_Speed speed;       /* options are ZENSCHED_JOB_SPEED_SLOW, ZENSCHED_JOB_SPEED_DEFAULT, or ZENSCHED_JOB_SPEED_FAST */

	char *cron;		     /* the job's cron string if it is a recurring job */
	time_t cron_nexttime;	   /* if the job is a recurring job, the next time it will run in utc ( gmt ) time */
	int cron_count;		 /* if the job is a recurring job, how many more times this job will run (-1 is forever) */
	int cron_faulttolerance;	/* 0 - not at all fault tolerant, do not reschedule if failed
					 * 1 - lazy tolerance,
					 * 2 - strict fault tolerance
					 */

	Eina_List *dependencies;	   /* for listing jobs which must complete before this job can execute */

} Zensched_Job;

/**
 * Enumerates zensched module types
 */
typedef enum {
	ZENSCHED_MODULE_TYPE_ADDON	      = ( 1 << 1 ),
	ZENSCHED_MODULE_TYPE_INTERNAL	   = ( 1 << 2 ),
	ZENSCHED_MODULE_TYPE_PLATFORMCTRL       = ( 1 << 3 ),
	ZENSCHED_MODULE_TYPE_STORAGECTRL	= ( 1 << 5 ),
	ZENSCHED_MODULE_TYPE_NETWORKCTRL	= ( 1 << 8 )
} Zensched_Module_Type;

/*
 * struct returned by a module initialization function
 */
typedef struct {
	/* type of the module. ZENSCHED_MODULE_TYPE_INTERNAL, ZENSCHED_MODULE_TYPE_ADDON */
	Zensched_Module_Type type;

	/* major version of the module */
	double version;

	/* zensched API version the module targets */
	double zensched_api;

	char *platform;
	char *platformver;
	char *platformbuild;
	char *platformvendor;
	char *platformfull;
	char *platformdescription;
	int platformfeatures;

	/* name of the module */
	char *name;
	char *description;
	char *parentmodule;

	/* module handle. points to whatever the module defines */
	void *handle;

} Zensched_Module_Info;

struct _DB {
	Eina_Module *library;
	Eina_Bool initialized : 1;
	DB_config *config;
	int pool_size;

	void *e;

	/*DB Abstraction Functions
	 * regenerate with:
	 */
#if 0
for x in db_sources/mysql/zrpcdb_*.c; do [[ "$(basename $x)" = "zrpcdb_common.c" ]] && continue; echo -e "\n\n/* $(basename $x) */\n"; grep "^Eina_Bool" $x |sed 's/\(.*\) \([a-zA-Z]*\)(\(.*\)/\1 (*\2)(\3/;s/\(.*\) \*\([a-zA-Z]*\)(\(.*\)/\1 *(*\2)(\3/;s/) \?{/)/;s/ __UNUSED__//g;s/$/;/' ;done
#endif

	Eina_Bool (*init)(DB *db, Zrpcdb_Connect_Cb cb, Ecore_Event_Handler_Cb error_cb, Ecore_Event_Handler_Cb result_cb, zentific_config *conf);
	Eina_Bool (*uninit)(DB *db);

	/* zrpcdb_auth.c */

	Eina_Bool (*getAuthByName)(DB *db, Zentific_Ctx *ctx, const char *name);
	Eina_Bool (*getAuth)(DB *db, Zentific_Ctx *ctx, int uid);


	/* zrpcdb_console.c */

	Eina_Bool (*getConsoleLockVmsBySession)(DB *db, Zentific_Ctx *ctx, const char *session_id);
	Eina_Bool (*getConsoleType)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *session_id);
	Eina_Bool (*getConsoleLockSession)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *type);
	Eina_Bool (*hasConsoleLock)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *type);
	Eina_Bool (*acquireConsoleLock)(DB *db, Zentific_Ctx *ctx, const char *uuid, SESSION *session, const char *type, const char *nodehost);
	Eina_Bool (*releaseConsoleLock)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *sessionid, const char *type);
	Eina_Bool (*saveGraphicalConsolePort)(DB *db, Zentific_Ctx *ctx, const char *uuid, int tunnelport, int externalport);
	Eina_Bool (*getGraphicalConsolePort)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getConsoleNodeHost)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *type);
	Eina_Bool (*getConsoleClientHost)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *type);
	Eina_Bool (*getGraphicalConsoleTunnelPort)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*saveTextConsolePort)(DB *db, Zentific_Ctx *ctx, const char *uuid,  int externalport);
	Eina_Bool (*getTextConsolePort)(DB *db, Zentific_Ctx *ctx, const char *uuid);


	/* zrpcdb_disk.c */

	Eina_Bool (*isDisk)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *ext_dev);
	Eina_Bool (*createDisk)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *path, int storid);
	Eina_Bool (*getNumVMdisks)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getDisksByStorage)(DB *db, Zentific_Ctx *ctx, int storid);
	Eina_Bool (*getVMdisksByStorage)(DB *db, Zentific_Ctx *ctx, const char *uuid, int storid);
	Eina_Bool (*getVMdisks)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getVmDisk)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *path);
	Eina_Bool (*updateDiskConfig)(DB *db, Zentific_Ctx *ctx, const char * uuid, int rev, DISK *disk);
	Eina_Bool (*updateDiskStats)(DB *db, Zentific_Ctx *ctx, const char *uuid, DISK *disk);
	Eina_Bool (*pruneDiskStats)(DB *db, Zentific_Ctx *ctx, const char * uuid, const char *extdev);
	Eina_Bool (*isDiskInUse)(DB *db, Zentific_Ctx *ctx, int storid, const char *path );
	Eina_Bool (*getDiskByStorage)(DB *db, Zentific_Ctx *ctx, int storid, const char *path);
	Eina_Bool (*getVmByDisk)(DB *db, Zentific_Ctx *ctx, int storid, const char *path);

	/* zrpcdb_group.c */

	Eina_Bool (*isUserGroup)(DB *db, Zentific_Ctx *ctx, int gid);
	Eina_Bool (*modifyUserGroup)(DB *db, Zentific_Ctx *ctx, int gid, USER_GROUP *ug);
	Eina_Bool (*removeUserGroup)(DB *db, Zentific_Ctx *ctx, int gid);
	Eina_Bool (*getUserGroup)(DB *db, Zentific_Ctx *ctx, int gid);
	Eina_Bool (*getUserGroupName)(DB *db, Zentific_Ctx *ctx, int gid);
	Eina_Bool (*getUserGroupMembers)(DB *db, Zentific_Ctx *ctx, int gid);
	Eina_Bool (*getUserGroupMembersFull)(DB *db, Zentific_Ctx *ctx, int gid);
	Eina_Bool (*addToUserGroup)(DB *db, Zentific_Ctx *ctx, int uid, int gid);
	Eina_Bool (*removeFromUserGroup)(DB *db, Zentific_Ctx *ctx, int uid, int gid);
	Eina_Bool (*renameUserGroup)(DB *db, Zentific_Ctx *ctx, int gid, const char *new_name);
	Eina_Bool (*isUserGroupMember)(DB *db, Zentific_Ctx *ctx, int uid, int gid);
	Eina_Bool (*isUserGroupByName)(DB *db, Zentific_Ctx *ctx, const char *name);
	Eina_Bool (*modifyUserGroupByName)(DB *db, Zentific_Ctx *ctx, const char *groupname, USER_GROUP *ug);
	Eina_Bool (*removeUserGroupByName)(DB *db, Zentific_Ctx *ctx, const char *groupname);
	Eina_Bool (*getUserGroupByName)(DB *db, Zentific_Ctx *ctx, const char *groupname);
	Eina_Bool (*getUserGroupId)(DB *db, Zentific_Ctx *ctx, const char *groupname);
	Eina_Bool (*getUserGroupMembersByName)(DB *db, Zentific_Ctx *ctx, const char *groupname);
	Eina_Bool (*renameUserGroupByName)(DB *db, Zentific_Ctx *ctx, const char *groupname, const char *new_name);
	Eina_Bool (*addToUserGroupByName)(DB *db, Zentific_Ctx *ctx, int uid, const char *groupname);
	Eina_Bool (*removeFromUserGroupByName)(DB *db, Zentific_Ctx *ctx, int uid, const char *groupname);
	Eina_Bool (*isUserGroupMemberByName)(DB *db, Zentific_Ctx *ctx, int uid, const char *groupname);
	Eina_Bool (*addToUserGroupByNameAndUsername)(DB *db, Zentific_Ctx *ctx, const char *username, const char *groupname);
	Eina_Bool (*removeFromUserGroupByNameAndUsername)(DB *db, Zentific_Ctx *ctx, const char *username, const char *groupname);
	Eina_Bool (*isUserGroupMemberByNameAndUsername)(DB *db, Zentific_Ctx *ctx, const char *username, const char *groupname);
	Eina_Bool (*addUserGroup)(DB *db, Zentific_Ctx *ctx, USER_GROUP *ug);
	Eina_Bool (*getAllUserGroups)(DB *db, Zentific_Ctx *ctx, int offset, int recordcount);
	Eina_Bool (*getAllUserGroupNames)(DB *db, Zentific_Ctx *ctx, int offset, int recordcount);
	Eina_Bool (*getAllUserGroupsFull)(DB *db, Zentific_Ctx *ctx, int offset, int recordcount);
	Eina_Bool (*getUserGroupsByUID)(DB *db, Zentific_Ctx *ctx, int uid);
	Eina_Bool (*getUserGroupsByName)(DB *db, Zentific_Ctx *ctx, const char *username);
	Eina_Bool (*getUserGroupNamesByUID)(DB *db, Zentific_Ctx *ctx, int uid);
	Eina_Bool (*getUserGroupNamesByName)(DB *db, Zentific_Ctx *ctx, const char *username);
	Eina_Bool (*getAllUserGroupsSorted)(DB *db, Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
	Eina_Bool (*getAllUserGroupsFullSorted)(DB *db, Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
	Eina_Bool (*getUserGroupCount)(DB *db, Zentific_Ctx *ctx);


	/* zrpcdb_network.c */

	Eina_Bool (*getAllNetworksFull)(DB *db, Zentific_Ctx *ctx, int offset, int recordcount);
	Eina_Bool (*getAllNetworksFullSorted)(DB *db, Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
	Eina_Bool (*getAllNetworksSorted)(DB *db, Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
	Eina_Bool (*getAllNetworks)(DB *db, Zentific_Ctx *ctx, int offset, int recordcount);
	Eina_Bool (*getAllNetworksByType)(DB *db, Zentific_Ctx *ctx, const char *type, int offset, int recordcount);
	Eina_Bool (*getNetwork)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*getNetworkVlans)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*removeNetwork)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*getNodesByNetworkVlan)(DB *db, Zentific_Ctx *ctx, int vlan);
	Eina_Bool (*getNodesByNetwork)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*getNetworksByNode)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*defineNetwork)(DB *db, Zentific_Ctx *ctx, NETWORK *net);
	Eina_Bool (*modifyNetwork)(DB *db, Zentific_Ctx *ctx, int id, NETWORK *net);
	Eina_Bool (*isNetwork)(DB *db, Zentific_Ctx *ctx, NETWORK *net);
	Eina_Bool (*isNetworkId)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*addNetIf)(DB *db, Zentific_Ctx *ctx, const char *uuid, NETIF *vif);
	Eina_Bool (*attachNetIf)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *mac, int nwid);
	Eina_Bool (*detachNetIf)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *mac, int nwid);
	Eina_Bool (*modifyNetIf)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *mac, NETIF *vif);
	Eina_Bool (*removeNetIf)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *mac);
	Eina_Bool (*detachAllNetIfsByNetwork)(DB *db, Zentific_Ctx *ctx, int nwid);
	Eina_Bool (*getNetworkControlModule)(DB *db, Zentific_Ctx *ctx, int nwid);
	Eina_Bool (*setNetworkControlModule)(DB *db, Zentific_Ctx *ctx, int nwid, int mid);
	Eina_Bool (*detachNetworkFromNode)(DB *db, Zentific_Ctx *ctx, const char *uuid, int nwid);
	Eina_Bool (*attachNetworkToNode)(DB *db, Zentific_Ctx *ctx, const char *uuid, int nwid);
	Eina_Bool (*getVmsByNetwork)(DB *db, Zentific_Ctx *ctx, int nwid);
	Eina_Bool (*getNetworksByVm)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*isNetworkInUse)(DB *db, Zentific_Ctx *ctx, int nwid);
	Eina_Bool (*getNetworkByNetIf)(DB * db, Zentific_Ctx *ctx, const char * uuid, const char *mac);
	Eina_Bool (*getNetworkIdByName)( DB *db, Zentific_Ctx *ctx, const char *name);
	Eina_Bool (*getNetworkCount)(DB *db, Zentific_Ctx *ctx);


	/* zrpcdb_node.c */

	Eina_Bool (*getNODEstat)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *stat);
	Eina_Bool (*getNODEattribute)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *attribute);
	Eina_Bool (*getLatestNodeStat)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getNodeAddress)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getNodeStats)(DB *db, Zentific_Ctx *ctx, const char *uuid, int start_epoch, int end_epoch, int max_events, int interval);
	Eina_Bool (*updateNodeStats)(DB *db, Zentific_Ctx *ctx, const char *uuid, NODE_STATS *stats);
	Eina_Bool (*updateNodeConfig)(DB *db, Zentific_Ctx *ctx, NODE *node);
	Eina_Bool (*getNodeControlModule)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*setNodeConfiguredHost)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *host);
	Eina_Bool (*getNodeConfiguredHost)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*createNode)(DB *db, Zentific_Ctx *ctx, NODE *node);
	Eina_Bool (*setNodeStatus)(DB *db, Zentific_Ctx *ctx, const char *uuid, Eina_Bool enabled);
	Eina_Bool (*setNodeControlModuleByName)(DB *db, Zentific_Ctx *ctx, const char *nodeuuid, const char *controlmodule);
	Eina_Bool (*setNodeControlModule)(DB *db, Zentific_Ctx *ctx, const char *nodeuuid, int controlmodule);
	Eina_Bool (*removeNode)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*isNodeEnabled)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*isNodeEnabledAndConfigured)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getAllUnconfiguredNodes)(DB *db, Zentific_Ctx *ctx, int offset, int recordcount);
	Eina_Bool (*getAllUnconfiguredNodesByUID)(DB *db, Zentific_Ctx *ctx, int uid, int offset, int recordcount);
	Eina_Bool (*getNodeConfiguredState)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*isNode)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getNodePlatform)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getNode)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getAllNodesFullRangeSorted)(DB *db, Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
	Eina_Bool (*getAllNodesFull)(DB *db, Zentific_Ctx *ctx);
	Eina_Bool (*getNodeNameAndUuid)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getNodeName)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getNodeByName)(DB *db, Zentific_Ctx *ctx, const char *name);
	Eina_Bool (*getAllNodes)(DB *db, Zentific_Ctx *ctx, int which, int offset, int recordcount);
	Eina_Bool (*getAllNodesByUID)(DB *db, Zentific_Ctx *ctx, int which, int uid, int offset, int recordcount);
	Eina_Bool (*getAllNodesSorted)(DB *db, Zentific_Ctx *ctx, int which, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
	Eina_Bool (*getAllNodesByUIDSorted)(DB *db, Zentific_Ctx *ctx, int which, const char *sortfield, Eina_Bool desc, int uid, int offset, int recordcount);
	Eina_Bool (*getNodeByHostname)(DB *db, Zentific_Ctx *ctx, const char *hostname);
	Eina_Bool (*pruneNodeStats)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getNodeLogs)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getNodeDefaultVNCpass)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getNodeHVM)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getNodeCount)(DB *db, Zentific_Ctx *ctx);
	Eina_Bool (*getNodeCountByUID)(DB *db, Zentific_Ctx *ctx, int uid);

	/* zrpcdb_platform.c */

	Eina_Bool (*isControlModuleForPlatform)(DB *db, Zentific_Ctx *ctx, const char *modname, int platformid);
	Eina_Bool (*isControlModuleForPlatformByMid)(DB *db, Zentific_Ctx *ctx, int mid, int platformid);
	Eina_Bool (*isControlModuleForPlatformById)(DB *db, Zentific_Ctx *ctx, int mid, const char *platformname);
	Eina_Bool (*isControlModuleForPlatformByName)(DB *db, Zentific_Ctx *ctx, const char *modname, const char *platformname);
	Eina_Bool (*isControlModule)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*isControlModuleByName)(DB *db, Zentific_Ctx *ctx, const char *name);
	Eina_Bool (*isPlatform)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*isPlatformByName)(DB *db, Zentific_Ctx *ctx, const char *platname);
	Eina_Bool (*enableControlModule)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*disableControlModule)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*enablePlatform)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*disablePlatform)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*getPlatformId)(DB *db, Zentific_Ctx *ctx, const char *name);
	Eina_Bool (*getPlatforms)(DB *db, Zentific_Ctx *ctx);
	Eina_Bool (*getPlatformsFull)(DB *db, Zentific_Ctx *ctx);
	Eina_Bool (*getPlatform)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*getPlatformByName)(DB *db, Zentific_Ctx *ctx, const char *name);
	Eina_Bool (*addPlatform)(DB *db, Zentific_Ctx *ctx, PLATFORM *p);
	Eina_Bool (*modifyPlatform)(DB *db, Zentific_Ctx *ctx, int id, PLATFORM *p);
	Eina_Bool (*setPlatformConfig)(DB *db, Zentific_Ctx *ctx, int id, Eina_List /*KEYVALUE*/ *config);
	Eina_Bool (*getPlatformConfig)(DB *db, Zentific_Ctx *ctx, int platformid, const char *key);
	Eina_Bool (*getControlModuleConfig)(DB *db, Zentific_Ctx *ctx, int id, const char *key);
	Eina_Bool (*getControlModuleId)(DB *db, Zentific_Ctx *ctx, const char *name);
	Eina_Bool (*getControlModuleName)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*getControlModuleNameByNetwork)(DB *db, Zentific_Ctx *ctx, int nwid);
	Eina_Bool (*getControlModuleNameByStorage)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*getControlModules)(DB *db, Zentific_Ctx *ctx);
	Eina_Bool (*getStorageControlModulesFull)(DB *db, Zentific_Ctx *ctx);
	Eina_Bool (*getNetworkControlModulesFull)(DB *db, Zentific_Ctx *ctx);
	Eina_Bool (*getPlatformControlModulesFull)(DB *db, Zentific_Ctx *ctx);
	Eina_Bool (*getPlatformControlModules)(DB *db, Zentific_Ctx *ctx);
	Eina_Bool (*getStorageControlModules)(DB *db, Zentific_Ctx *ctx);
	Eina_Bool (*getNetworkControlModules)(DB *db, Zentific_Ctx *ctx);
	Eina_Bool (*getControlModulesByPlatform)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*getControlModulesFull)(DB *db, Zentific_Ctx *ctx);
	Eina_Bool (*getControlModule)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*getControlModuleByName)(DB *db, Zentific_Ctx *ctx, const char *name);
	Eina_Bool (*setControlModuleConfig)(DB *db, Zentific_Ctx *ctx, int id, Eina_List /*KEYVALUE*/ *config);
	Eina_Bool (*getPlatformIdByVm)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getPlatformFeatures)(DB *db, Zentific_Ctx *ctx, int platform);


	/* zrpcdb_scheduler.c */

	Eina_Bool (*isJob)(DB *db, Zentific_Ctx *ctx, int jobid);
	Eina_Bool (*getJob)(DB *db, Zentific_Ctx *ctx, int jobid, int caller_uid);
	Eina_Bool (*getRecentJobs)(DB *db, Zentific_Ctx *ctx, int age);
	Eina_Bool (*getRecentJobsByUID)(DB *db, Zentific_Ctx *ctx, int uid, int age);
	Eina_Bool (*getJobUID)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*getJobsByIds)(DB *db, Zentific_Ctx *ctx, Eina_List *jids, int caller_uid);
	Eina_Bool (*getJobs)(DB *db, Zentific_Ctx *ctx);
	Eina_Bool (*getJobsByUID)(DB *db, Zentific_Ctx *ctx, int uid);
	Eina_Bool (*getJobsByUUID)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getJobsStatus)(DB *db, Zentific_Ctx *ctx, Eina_List *jids, int caller_uid);
	Eina_Bool (*getJobStatus)(DB *db, Zentific_Ctx *ctx, int jobid, int caller_uid);
	Eina_Bool (*addJob)(DB *db, Zentific_Ctx *ctx, JOB *job);
	Eina_Bool (*getSchedulerAddressByModuleName)(DB *db, Zentific_Ctx *ctx, const char *modname);
	Eina_Bool (*getSchedulerAddressByJob)(DB *db, Zentific_Ctx *ctx, int jobid);
	Eina_Bool (*getSchedulerIdByModuleName)(DB *db, Zentific_Ctx *ctx, const char *modname);
	Eina_Bool (*getSchedulerIdByJob)(DB *db, Zentific_Ctx *ctx, int jobid);
	Eina_Bool (*getSchedulerIdByPlatform)(DB *db, Zentific_Ctx *ctx, const char *modname);
	Eina_Bool (*fetchJobs)(DB *db, Zentific_Ctx *ctx, Eina_Bool fetchAll);
	Eina_Bool (*updateJob)(DB *db, Zentific_Ctx *ctx, Zensched_Job *job, int schedulerId);
	Eina_Bool (*getModuleConfigByNode)(DB *db, Zentific_Ctx *ctx, int mid, const char *uuid);
	Eina_Bool (*getModuleConfigByVm)(DB *db, Zentific_Ctx *ctx, int mid, const char *uuid);
	Eina_Bool (*getModuleConfig)(DB *db, Zentific_Ctx *ctx, const char *module_name);
	Eina_Bool (*getBlobConfig)(DB *db, Zentific_Ctx *ctx, int platform, int module, const char *key);
	Eina_Bool (*registerPlatform)(DB *db, Zentific_Ctx *ctx, Zensched_Module_Info *modinfo, int schedulerId);
	Eina_Bool (*registerControlModule)(DB *db, Zentific_Ctx *ctx, Zensched_Module_Info *modinfo, int platid, int schedulerId);
	Eina_Bool (*registerScheduler)(DB *db, Zentific_Ctx *ctx, const char *hostname, const char *address, int port);
	Eina_Bool (*getModuleID)(DB *db, Zentific_Ctx *ctx, const char *module, int schedulerId);
	Eina_Bool (*getPlatformID)(DB *db, Zentific_Ctx *ctx, const char *platform);
	Eina_Bool (*isModule)(DB *db, Zentific_Ctx *ctx, const char *modname, int schedulerId);
	Eina_Bool (*isPlatformNameByScheduler)(DB *db, Zentific_Ctx *ctx, const char *platname, int schedulerId);
	Eina_Bool (*clearControlModules)(DB *db, Zentific_Ctx *ctx, int schedulerId);


	/* zrpcdb_session.c */

	Eina_Bool (*getLoggedInUsers)(DB *db, Zentific_Ctx *ctx);
	Eina_Bool (*getLoggedInUsernames)(DB *db, Zentific_Ctx *ctx);
	Eina_Bool (*getSessions)(DB *db, Zentific_Ctx *ctx);
	Eina_Bool (*getSession)(DB *db, Zentific_Ctx *ctx, const char *sessionid);
	Eina_Bool (*setSession)(DB *db, Zentific_Ctx *ctx, SESSION *session);
	Eina_Bool (*setLastLogin)(DB *db, Zentific_Ctx *ctx, int uid);
	Eina_Bool (*deauthSession)(DB *db, Zentific_Ctx *ctx, const char *session_id);
	Eina_Bool (*removeSession)(DB *db, Zentific_Ctx *ctx, const char *session_id);
	Eina_Bool (*getUserBySession)(DB *db, Zentific_Ctx *ctx, const char *session_id);
	Eina_Bool (*getUserNameBySession)(DB *db, Zentific_Ctx *ctx, const char *session_id);

	/* zrpcdb_storage.c */

	Eina_Bool (*getAllStorage)(DB *db, Zentific_Ctx *ctx, int offset, int recordcount);
	Eina_Bool (*getAllStorageFull)(DB *db, Zentific_Ctx *ctx, int offset, int recordcount);
	Eina_Bool (*getAllStorageFullSorted)(DB *db, Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
	Eina_Bool (*getAllStorageSorted)(DB *db, Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
	Eina_Bool (*getAllStorageByType)(DB *db, Zentific_Ctx *ctx, const char *type, int offset, int recordcount);
	Eina_Bool (*getStorageHost)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*getStorage)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*removeStorage)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*getNodesByStorage)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*getStorageByNode)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*defineStorage)(DB *db, Zentific_Ctx *ctx, STORAGE *stor);
	Eina_Bool (*modifyStorage)(DB *db, Zentific_Ctx *ctx, int id, STORAGE *storage);
	Eina_Bool (*isStorageId)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*isStorage)(DB *db, Zentific_Ctx *ctx, STORAGE *stor);
	Eina_Bool (*addDisk)(DB *db, Zentific_Ctx *ctx, int storid, DISK *disk);
	Eina_Bool (*enableDisk)(DB *db, Zentific_Ctx *ctx, int storid, const char *path);
	Eina_Bool (*disableDisk)(DB *db, Zentific_Ctx *ctx, int storid, const char *path);
	Eina_Bool (*attachDisk)(DB *db, Zentific_Ctx *ctx, int storid, const char *path);
	Eina_Bool (*detachDisk)(DB *db, Zentific_Ctx *ctx, int storid, const char *path);
	Eina_Bool (*modifyDisk)(DB *db, Zentific_Ctx *ctx, int storid, const char *path, DISK *disk);
	Eina_Bool (*removeDisk)(DB *db, Zentific_Ctx *ctx, int storid, const char *path);
	Eina_Bool (*detachAllDisksByStorage)(DB *db, Zentific_Ctx *ctx, int storid);
	Eina_Bool (*getStorageControlModule)(DB *db, Zentific_Ctx *ctx, int storid);
	Eina_Bool (*setStorageControlModule)(DB *db, Zentific_Ctx *ctx, int storid, int mid);
	Eina_Bool (*detachStorageFromNode)(DB *db, Zentific_Ctx *ctx, const char *uuid, int storid);
	Eina_Bool (*attachStorageToNode)(DB *db, Zentific_Ctx *ctx, const char *uuid, int storid);
	Eina_Bool (*getVmsByStorage)(DB *db, Zentific_Ctx *ctx, int storid);
	Eina_Bool (*getStorageByVm)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*isStorageInUse)(DB *db, Zentific_Ctx *ctx, int storid);
	Eina_Bool (*getStorageByDisk)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *path);
	Eina_Bool (*getStorageIdByName)(DB *db, Zentific_Ctx *ctx, const char *name);
	Eina_Bool (*getAllStorageTypes)(DB *db, Zentific_Ctx *ctx);
	Eina_Bool (*isDiskByStorage)(DB *db, Zentific_Ctx *ctx, int storid, const char *path);
	Eina_Bool (*getStorageCount)(DB *db, Zentific_Ctx *ctx);


	/* zrpcdb_user.c */

	Eina_Bool (*getUser)(DB *db, Zentific_Ctx *ctx, int uid);
	Eina_Bool (*isUserUID)(DB *db, Zentific_Ctx *ctx, int uid);
	Eina_Bool (*setUserType)(DB *db, Zentific_Ctx *ctx, int uid, int type);
	Eina_Bool (*getUserType)(DB *db, Zentific_Ctx *ctx, int uid);
	Eina_Bool (*getUsername)(DB *db, Zentific_Ctx *ctx, int uid);
	Eina_Bool (*getSessionsByUID)(DB *db, Zentific_Ctx *ctx, int uid);
	Eina_Bool (*isVMowner)(DB *db, Zentific_Ctx *ctx, const char *uuid, int uid);
	Eina_Bool (*isNODEowner)(DB *db, Zentific_Ctx *ctx, const char *uuid, int uid);
	Eina_Bool (*setUserAttribute)(DB *db, Zentific_Ctx *ctx, int uid, const char *attribute, const char *value);
	Eina_Bool (*isActive)(DB *db, Zentific_Ctx *ctx, int uid);
	Eina_Bool (*removeUser)(DB *db, Zentific_Ctx *ctx, int uid);
	Eina_Bool (*setPassword)(DB *db, Zentific_Ctx *ctx, int uid, const char *hash, const char *salt);
	Eina_Bool (*getPrivilege)(DB *db, Zentific_Ctx *ctx, const char *uuid, int uid, int gid, int node_vm);
	Eina_Bool (*grantPrivilege)(DB *db, Zentific_Ctx *ctx, const char *uuid, int uid, int gid, int level, int node_vm);
	Eina_Bool (*revokePrivilege)(DB *db, Zentific_Ctx *ctx, const char *uuid, int uid, int gid, int node_vm);
	Eina_Bool (*modifyUser)(DB *db, Zentific_Ctx *ctx, int uid, USER *u);
	Eina_Bool (*isUserName)(DB *db, Zentific_Ctx *ctx, const char *name);
	Eina_Bool (*getUserUID)(DB *db, Zentific_Ctx *ctx, const char *name);
	Eina_Bool (*getActiveUserUID)(DB *db, Zentific_Ctx *ctx, const char *name);
	Eina_Bool (*getUserTypeByName)(DB *db, Zentific_Ctx *ctx, const char *name);
	Eina_Bool (*getUserByName)(DB *db, Zentific_Ctx *ctx, const char *name);
	Eina_Bool (*getSessionsByName)(DB *db, Zentific_Ctx *ctx, const char *name);
	Eina_Bool (*isVMownerByName)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *name);
	Eina_Bool (*isNODEownerByName)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *name);
	Eina_Bool (*setUserAttributeByName)(DB *db, Zentific_Ctx *ctx, const char *name, const char *attribute, const char *value);
	Eina_Bool (*isActiveByName)(DB *db, Zentific_Ctx *ctx, const char *name);
	Eina_Bool (*removeUserByName)(DB *db, Zentific_Ctx *ctx, const char *name);
	Eina_Bool (*setPasswordByName)(DB *db, Zentific_Ctx *ctx, const char *name, const char *hash, const char *salt);
	Eina_Bool (*getPrivilegeByName)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *name, int node_vm);
	Eina_Bool (*grantPrivilegeByName)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *name, int level, int node_vm);
	Eina_Bool (*revokePrivilegeByName)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *name, int node_vm);
	Eina_Bool (*modifyUserByName)(DB *db, Zentific_Ctx *ctx, const char *name, USER *u);
	Eina_Bool (*getLanguages)(DB *db, Zentific_Ctx *ctx);
	Eina_Bool (*getLanguageIds)(DB *db, Zentific_Ctx *ctx);
	Eina_Bool (*getLanguageCode)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*getLanguageId)(DB *db, Zentific_Ctx *ctx, const char *lang);
	Eina_Bool (*addUser)(DB *db, Zentific_Ctx *ctx, USER *user);
	Eina_Bool (*getAllUsers)(DB *db, Zentific_Ctx *ctx, int offset, int recordcount);
	Eina_Bool (*getAllUsersFull)(DB *db, Zentific_Ctx *ctx, int offset, int recordcount);
	Eina_Bool (*getAllUsersSorted)(DB *db, Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
	Eina_Bool (*getAllUsersFullSorted)(DB *db, Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
	Eina_Bool (*getUserCount)(DB *db, Zentific_Ctx *ctx);


	/* zrpcdb_vif.c */

	Eina_Bool (*isNetIfByMAC)(DB *db, Zentific_Ctx *ctx, const char *mac);
	Eina_Bool (*isNetIf)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *mac);
	Eina_Bool (*createNetIf)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *mac);
	Eina_Bool (*getNumVMvifs)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*updateNetIfStats)(DB *db, Zentific_Ctx *ctx, const char *uuid, NETIF *vif);
	Eina_Bool (*getVMvifByMac)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *mac);
	Eina_Bool (*getVMvifsByNetwork)(DB *db, Zentific_Ctx *ctx, int nwid);
	Eina_Bool (*getVMvifs)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*updateNetIfConfig)(DB *db, Zentific_Ctx *ctx, const char *uuid, int nextrevision, NETIF *vif);
	Eina_Bool (*pruneNetIfStats)(DB *db, Zentific_Ctx *ctx, const char * uuid, const char *mac);
	Eina_Bool (*isNetIfInUse)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *mac );


	/* zrpcdb_vm.c */

	Eina_Bool (*isVM)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*isVmIgnored)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*needsReboot)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*setVmNeedsReboot)(DB *db, Zentific_Ctx *ctx, const char *uuid, Eina_Bool needreboot);
	Eina_Bool (*getVMparentNode)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*updateVMparentNode)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *puuid);
	Eina_Bool (*createVM)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*createVmTemplate)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*saveVMrevision)(DB *db, Zentific_Ctx *ctx, const char *uuid, int uid, const char *name, const char *description);
	Eina_Bool (*getVMsSorted)(DB *db, Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
	Eina_Bool (*getVMs)(DB *db, Zentific_Ctx *ctx, int offset, int recordcount);
	Eina_Bool (*getVMsByUIDSorted)(DB *db, Zentific_Ctx *ctx, int uid, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
	Eina_Bool (*getVMsByUID)(DB *db, Zentific_Ctx *ctx, int uid, int offset, int recordcount);
	Eina_Bool (*pruneVmStats)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*setVmState)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *state);
	Eina_Bool (*updateVmTimestamp)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*updateVmStats)(DB *db, Zentific_Ctx *ctx, VM *vm);
	Eina_Bool (*getTopCpuConsumers)(DB *db, Zentific_Ctx *ctx, int intervalminutes, int number);
	Eina_Bool (*getTopBandwidthConsumers)(DB *db, Zentific_Ctx *ctx, int intervalminutes, int number);
	Eina_Bool (*ignoreVm)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*removeVm)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getVM)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getVmByRev)(DB *db, Zentific_Ctx *ctx, const char *uuid, int rev);
	Eina_Bool (*getVMsFull)(DB *db, Zentific_Ctx *ctx);
	Eina_Bool (*updateVmConfig)(DB *db, Zentific_Ctx *ctx, VM *input);
	Eina_Bool (*getAllUsersByVm)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getVMstat)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *stat);
	Eina_Bool (*getVMattribute)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *attribute);
	Eina_Bool (*saveVmNotes)(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *notes);
	Eina_Bool (*getVmNotes)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getVMparentAddress)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getVmLogs)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getTemplateByName)(DB *db, Zentific_Ctx *ctx, const char *name);
	Eina_Bool (*getTemplate)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*getAllTemplates)(DB *db, Zentific_Ctx *ctx);
	Eina_Bool (*getAllTemplatesFull)(DB *db, Zentific_Ctx *ctx);
	Eina_Bool (*getTemplateConfig)(DB *db, Zentific_Ctx *ctx, int id);
	Eina_Bool (*getTemplateConfigByName)(DB *db, Zentific_Ctx *ctx, const char *name);
	Eina_Bool (*modifyTemplate)(DB *db, Zentific_Ctx *ctx, int id, TEMPLATE *template);
	Eina_Bool (*addTemplate)(DB *db, Zentific_Ctx *ctx, TEMPLATE *template);
	Eina_Bool (*hasGuestTools)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getAllVmsByNode)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*getAllVmsByNodeByUID)(DB *db, Zentific_Ctx *ctx, const char *uuid, int uid);
	Eina_Bool (*getVmByName)(DB *db, Zentific_Ctx *ctx, const char *name);
	Eina_Bool (*getVMparentNodeAndAddress)(DB *db, Zentific_Ctx *ctx, const char *uuid);
	Eina_Bool (*ztoolsUpdate)(DB *db, Zentific_Ctx *ctx, const char *uuid, Eina_List *info);
	Eina_Bool (*getVmCount)(DB *db, Zentific_Ctx *ctx);
	Eina_Bool (*getVmCountByUID)(DB *db, Zentific_Ctx *ctx, int uid);



	/* zrpcdb_zentific.c */

	Eina_Bool (*getLatestRevision)(DB *db, Zentific_Ctx *ctx, const char *uuid, unsigned char type);
	Eina_Bool (*getConfigValue)(DB *db, Zentific_Ctx *, const char *key, const char *subsystem,
		int mid, int uid, int gid, int vgid, int ngid, int vm,
		int node, int platform, int role);
	Eina_Bool (*getConfigs)(DB *db, Zentific_Ctx *, const char *subsystem,
		int mid, int uid, int gid, int vgid, int ngid, int vm,
		int node, int platform, int role);
	Eina_Bool (*getConfig)(DB *db, Zentific_Ctx *, const char *key, const char *subsystem,
		int mid, int uid, int gid, int vgid, int ngid, int vm,
		int node, int platform, int role);
	Eina_Bool (*setConfig)(DB *db, Zentific_Ctx *, const char *key, const char *value, const char *subsystem,
		int mid, int uid, int gid, int vgid, int ngid, int vm,
		int node, int platform, int role);
	Eina_Bool (*setBlobConfig)(DB *db, Zentific_Ctx *,
		const char *key, const unsigned char *value, const char *subsystem,
		int mid, int uid, int gid, int vgid, int ngid, int vm,
		int node, int platform, int role );
	Eina_Bool (*getConfigsFull)(DB *db, Zentific_Ctx *);
	Eina_Bool (*isFrontendPluginEnabled)(DB *db, Zentific_Ctx *, const char *name);
	Eina_Bool (*getAllPlugins)(DB *db, Zentific_Ctx *, int which);
	Eina_Bool (*getFrontendPlugins)(DB *db, Zentific_Ctx *, int which);
	Eina_Bool (*modifyPlugin)(DB *db, Zentific_Ctx *, const char *name, Eina_Bool enabled, Eina_Bool xmlrpc, Eina_Bool frontend, Eina_Bool scheduler, Eina_Bool poller);
	Eina_Bool (*cleanDB)(DB *db, Zentific_Ctx *);
	Eina_Bool (*getOperatingSystems)(DB *db, Zentific_Ctx *ctx);

};

#endif //ZENTIFIC_TYPES_H
