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
#include "functions.h"
#include "functions-network.h"
#include "functions-vif.h"
#include "functions-vm.h"
#include "functions-platform.h"
#include "functions-node.h"
#include "functions-jobs.h"
#include <inttypes.h>

static JOB *
network_job_init(int uid, const char *module_function, const char *input_string, const char *extra){
	JOB *ret;

	ret =  JOB_new();

	ret->uid		= uid;
	ret->module_function	= eina_stringshare_add(module_function);
	ret->input_string	= input_string;
	ret->extra		= eina_stringshare_add(extra);
	return ret;
}


static void
network_job_init_node(Zentific_Ctx *ctx, const char *cb, const char *uuid, int uid, const char *module_function, const char *input_string, const char *extra){
	JOB *ret;

	ret = network_job_init(uid, module_function, input_string, extra);
	ret->target_vm = eina_stringshare_add("");
	ret->target_node = eina_stringshare_add(uuid);
	_getNodeHostname(ctx, uuid);
	CTX_CB_ADD(cb_job_target_node_addr);
	CTX_PARAM_ADD(cb_job_target_node_addr, ret);
	CTX_PARAM_ADD_FROM_VARIABLE(cb, ret);
}

static void
network_job_init_vm(Zentific_Ctx *ctx, const char *cb, const char *uuid, int uid, const char *module_function, const char *input_string, const char *extra){
	JOB *ret;

	ret = network_job_init(uid, module_function, input_string, extra);
	ret->target_vm = eina_stringshare_add(uuid);
	db->getVMparentNodeAndAddress(db, ctx, uuid);
	CTX_CB_ADD(cb_job_target_node_addr);
	CTX_PARAM_ADD(cb_job_target_node_addr, ret);
	CTX_PARAM_ADD_FROM_VARIABLE(cb, ret);
}

const char *
network_job_string(Zentific_Ctx *ctx, const char *function, const void *info, NETWORK *net){
	const char *ret;

	if (!strstr(function + 6, "Network")){
		intptr_t *vlan;
		Eina_Strbuf *vlans;
		Eina_List *l;
		vlans = eina_strbuf_new();
		EINA_LIST_FOREACH(net->vlans, l, vlan){
			if (vlan && (*vlan >= -1)){
				eina_strbuf_append_printf(vlans, "%"PRIiPTR, *vlan);
				if (l->next)
					eina_strbuf_append_char(vlans, ',');
			}
		}
		if (function[0] == 'm')
			ret = eina_stringshare_printf(
			"host=%s::name=%s::type=%s::network=%s::netmask=%s"
			"::gateway=%s::mac=%s::interface=%s::vlans='%s'",
			net->host, net->name, net->type, net->network,
			net->netmask, net->gateway, net->mac, net->interface, eina_strbuf_string_get(vlans));
		else
			ret = eina_stringshare_printf("iface=%s::mac=%s::vlans=%s", net->interface, net->mac, eina_strbuf_string_get(vlans));
		eina_strbuf_free(vlans);
	} else if ((!zshare_strcmp(function, "attachNetIf")) || (!zshare_strcmp(function, "detachNetIf")))
		ret = eina_stringshare_printf("vifmac=%s::nwiface=%s::nwmac=%s", (const char*)info, net->interface, net->mac);
	else if (!zshare_strcmp(function, "addNetIf")){
		const NETIF *iface = info;
		ret = eina_stringshare_printf(
		"mac=%s::ip=%s::netmask=%s::broadcast=%s::"
		"bridge=%s::type=%s::gateway=%s::script=%s::"
		"name=%s::mtu=%d",
		iface->mac, iface->ip, iface->netmask, iface->broadcast,
		iface->bridge, iface->type, iface->gateway, iface->script,
		iface->name, iface->mtu);
	} else if (!zshare_strcmp(function, "removeNetIf"))
		ret = eina_stringshare_printf("mac=%s", (const char *)info);
	else if (!zshare_strcmp(function, "modifyNetIf")){
		NETIF *iface = 	eina_hash_find(ctx->params, __func__);
		eina_hash_del_by_key(ctx->params, __func__);
		ret = eina_stringshare_printf("mac=%s::newmac=%s::label=%s::name=%s::bridge=%s::"
			"script=%s::type=%s::ip=%s::netmask=%s::"
			"gateway=%s::broadcast=%s::mtu=%d",
			(const char*)info, iface->mac, iface->label, iface->name, iface->bridge,
			iface->script, iface->type, iface->ip, iface->netmask,
			iface->gateway, iface->broadcast, iface->mtu);
	}

	return ret;
}

static void
network_job_create_node(Zentific_Ctx *ctx, const char *uuid, int nwid, int caller_uid, const char *function){
	JOB *ret;
	NETWORK *net;

	switch (ctx->count){
		case 0:
			_isNODE(ctx, uuid);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		case 1:
			CTX_CB_ADD(cb_bool_rewind);
			break;
		case 2:
			db->getNetwork(db, ctx, nwid);
			CTX_VALUE_RETURN;
			break;
		case 3:
			net = CTX_PARAM_GET;
			if (!net){
				if (!ctx->error)
					ctx->error = AZY_ERR(UNKNOWN_RESOURCE);
				break;
			}
			network_job_init_node(ctx, __func__, uuid, caller_uid, function,
				network_job_string(ctx, function, NULL, net), "");
			NETWORK_free(net);
			break;
		case 4:
			ret = CTX_PARAM_PEEK;
			_getControlModuleNameByNetwork(ctx, nwid);
			CTX_CB_ADD(cb_job_module_name);
			CTX_PARAM_ADD(cb_job_module_name, ret);
			break;
		case 5:
			ret = CTX_PARAM_GET;
			job_add(ctx, ret, caller_uid);
			JOB_free(ret);
			break;
		default:
			ctx->error = AZY_ERR(SERVER_FAILURE);
	}
}

static void
network_job_create_vm(Zentific_Ctx *ctx, const char *uuid, const void *info, int nwid, int caller_uid, const char *function){
	JOB *ret;
	NETWORK *net;
	intptr_t *id;


	switch (ctx->count){
		case 0:
			if(!info || nwid < 0){
				ctx->error = AZY_ERR(INVALID_PARAMETER);
				break;
			}
			_isVM(ctx, uuid);
			CTX_CB_ADD(cb_bool_rewind);
		case 1:
			if ((!zshare_strcmp(function, "addNetIf")) || (!zshare_strcmp(function, "removeNetIf"))){
				_isNetworkId(ctx, nwid);
				CTX_CB_ADD(cb_bool_rewind);
				if (function[0] == 'r') ctx->count += 9;
				break;
			} else {
				db->getNetwork(db, ctx, nwid);
				CTX_VALUE_RETURN;
				break;
			}
		case 2:
			net = CTX_PARAM_GET;
			if (!net){
				if (!ctx->error)
					ctx->error = AZY_ERR(UNKNOWN_RESOURCE);
				break;
			}
			network_job_init_vm(ctx, __func__, uuid, caller_uid, function,
				network_job_string(ctx, function, info, net), "");
			if (net) NETWORK_free(net);
			break;
		case 3:
			ret = CTX_PARAM_PEEK;
			_getControlModuleNameByNetwork(ctx, nwid);
			CTX_CB_ADD(cb_job_module_name);
			CTX_PARAM_ADD(cb_job_module_name, ret);
			break;
		case 4:
			CTX_CB_ADD(cb_bool_rewind);
			break;
		case 5:
			ret = CTX_PARAM_GET;
			job_add(ctx, ret, caller_uid);
			JOB_free(ret);
			break;
		/* removeNetIf and modifyNetIf only */
		case 10:
			db->isNetworkInUse(db, ctx, nwid);
			CTX_CB_ADD(cb_int_rewind);
			break;
		case 11:
			if (ctx->error){
				if (ctx->error == AZY_ERR(UNKNOWN_RESOURCE)){ /* in use */
					ctx->error = AZY_ERR(RESOURCE_IN_USE);
					WRN("cannot remove network '%i' because it is currently utilized by one or more resources", nwid);
				}
				break;
			}
			db->getNetworkByNetIf(db, ctx, uuid, (const char *)info);
			CTX_INT_RETURN;
		case 12:
		{
			ctx->retval = CTX_PARAM_GET;
			network_job_init_vm(ctx, __func__, uuid, caller_uid, function,
				network_job_string(ctx, function, info, NULL), "");
			break;
		}
		case 13:
		{
			ret = CTX_PARAM_PEEK;
			id = ctx->retval;
			ctx->retval = NULL;
			ctx->count -= 10;
			_getControlModuleNameByNetwork(ctx, id ? *id : 0);
			CTX_CB_ADD(cb_job_module_name);
			CTX_PARAM_ADD(cb_job_module_name, ret);
			break;
		}
		default:
			ctx->error = AZY_ERR(SERVER_FAILURE);
	}
}

Eina_Bool _isNetwork(Zentific_Ctx *ctx, NETWORK *net){
	db->isNetwork(db, ctx, net);
	CTX_RETURN;
}

Eina_Bool _isNetworkId(Zentific_Ctx *ctx, int id){
	if (id < 0){
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		CTX_RETURN;
	}
	db->isNetworkId(db, ctx, id);
	CTX_RETURN;
}

Eina_Bool _getNetwork(Zentific_Ctx *ctx, int id){
	db->getNetwork(db, ctx, id);
	CTX_RETURN;
}

Eina_Bool _getAllNetworksByType(Zentific_Ctx *ctx, const char *type, int offset, int recordcount){
	db->getAllNetworksByType(db, ctx, type, offset, recordcount);
	CTX_RETURN;
}


Eina_Bool _getAllNetworks(Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	if (sortfield)
		db->getAllNetworksSorted(db, ctx, sortfield, desc, offset, recordcount);
	else
		db->getAllNetworks(db, ctx, offset, recordcount);
	CTX_RETURN;
}

Eina_Bool _getAllNetworksFull(Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	if (sortfield)
		db->getAllNetworksFullSorted(db, ctx, sortfield, desc, offset, recordcount);
	else
		db->getAllNetworksFull(db, ctx, offset, recordcount);
	CTX_RETURN;
}

/* TODO
 * 	determine if a distinction shouould be made between adding and referencing resources
 * 		addResource, and defineResource. i.e., addNewResource and defineExistingResource
 */
Eina_Bool _defineNetwork(Zentific_Ctx *ctx, NETWORK *net){
	//intentionally ignore whatever was passed in a control module
	net->mid = -1;
	db->defineNetwork(db, ctx, net);
	CTX_RETURN;
}

Eina_Bool _getNetworksByNode(Zentific_Ctx *ctx, const char *uuid){
	db->getNetworksByNode(db, ctx, uuid);
	CTX_RETURN;
}

Eina_Bool _attachNetworkToNode(Zentific_Ctx *ctx, const char *uuid, int nwid, int caller_uid){
	if (ctx->count == 1)
		db->attachNetworkToNode(db, ctx, uuid, nwid);
	network_job_create_node(ctx, uuid, nwid, caller_uid, "attachNetwork");
	CTX_RETURN;
}

Eina_Bool _detachNetworkFromNode(Zentific_Ctx *ctx, const char *uuid, int nwid, int caller_uid){
	if (ctx->count == 1)
		db->detachNetworkFromNode(db, ctx, uuid, nwid);
	network_job_create_node(ctx, uuid, nwid, caller_uid, "detachNetwork");
	CTX_RETURN;
}

//permissions should be checked before invocation
Eina_Bool _removeNetwork(Zentific_Ctx *ctx, int id){
	switch (ctx->count){
		case 0:
			_isNetworkId(ctx, id);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		case 1:
			if (ctx->error) break; /* not found */
			db->isNetworkInUse(db, ctx, id);
			CTX_CB_ADD(cb_int_rewind);
			break;
		case 2:
			if (ctx->error){
				if (ctx->error == AZY_ERR(UNKNOWN_RESOURCE)){ /* in use */
					ctx->error = AZY_ERR(RESOURCE_IN_USE);
					WRN("cannot remove network '%d' because it is currently utilized by one or more resources", id);
				}
				break;
			}
			db->removeNetwork(db, ctx, id);
			break;
		default:
			ctx->error = AZY_ERR(SERVER_FAILURE);
	}
	CTX_RETURN;
}

/* TODO warning need to fire a job in _removeNetwork and and other functions below.
 * what order? db add first, then fire job? or fire job, wait for completion, and then save to db?
 *
 * TODO	what about vm config revisioning for tasks like 'attachNetIf' ?
 */
Eina_Bool _attachNetIf(Zentific_Ctx *ctx, const char *uuid, const char *mac, int nwid, int caller_uid){
	if (ctx->count == 4)
		db->attachNetIf(db, ctx, uuid, mac, nwid);
	network_job_create_vm(ctx, uuid, mac, nwid, caller_uid, "attachNetIf");
	CTX_RETURN;
}

Eina_Bool _detachNetIf(Zentific_Ctx *ctx, const char *uuid, const char *mac, int nwid, int caller_uid){
	if (ctx->count == 4)
		db->detachNetIf(db, ctx, uuid, mac, nwid);
	network_job_create_vm(ctx, uuid, mac, nwid, caller_uid, "detachNetIf");
	CTX_RETURN;
}

Eina_Bool _addNetIf(Zentific_Ctx *ctx, const char *uuid, int nwid, NETIF *iface, int caller_uid){
	if (ctx->count == 4)
		db->addNetIf(db, ctx, uuid, iface);
	network_job_create_vm(ctx, uuid, iface, nwid, caller_uid, "addNetIf");
	CTX_RETURN;
}

Eina_Bool _removeNetIf(Zentific_Ctx *ctx, const char *uuid, const char *mac, int caller_uid){
	if (ctx->count == 4)
		db->removeNetIf(db, ctx, uuid, mac);
	network_job_create_vm(ctx, uuid, mac, -1, caller_uid, "removeNetIf");
	CTX_RETURN;
}

Eina_Bool _modifyNetIf(Zentific_Ctx *ctx, const char *uuid, const char *mac, NETIF *netif, int caller_uid){
	NETIF *stored;

	switch (ctx->count){
		case 0:
			_getNetIfByMAC(ctx, uuid, mac);
			CTX_VALUE_RETURN;
			break;
		case 1:
			stored = CTX_PARAM_GET;
			if (!stored){
				if (!ctx->error)
					ctx->error = AZY_ERR(UNKNOWN_RESOURCE);
				break;
			}
			if (!NETIF_config_cmp(netif, stored)){
				NETIF_free(stored);
				AZY_RETVAL(NULL);
				break;
			}
			NETIF_free(stored);
			CTX_PARAM_ADD(network_job_string, netif);
			network_job_create_vm(ctx, uuid, mac, -1, caller_uid, "modifyNetIf");
			break;
		case 4:
			db->modifyNetIf(db, ctx, uuid, mac, netif);
		default:
			network_job_create_vm(ctx, uuid, mac, -1, caller_uid, "modifyNetIf");
	}
	CTX_RETURN;
}

Eina_Bool _setNetworkControlModule(Zentific_Ctx *ctx, int nwid, int mid){
//FIXME check for control module existence
	db->setNetworkControlModule(db, ctx, nwid, mid);
	CTX_RETURN;
}

Eina_Bool _getNetworkControlModule(Zentific_Ctx *ctx, int nwid){
//FIXME check for network id existence
	db->getNetworkControlModule(db, ctx, nwid);
	CTX_RETURN;
}

Eina_Bool  _getNetworkControlModules(Zentific_Ctx *ctx) {
	/* just list of ints, return directly */
	db->getNetworkControlModules(db, ctx);
	CTX_RETURN;
}

Eina_Bool _getNetworkControlModulesFull(Zentific_Ctx *ctx) {
	db->getNetworkControlModulesFull(db, ctx);
	CTX_RETURN;
}

static void
network_cb_jid_collect(Zentific_Ctx *ctx, int ret){
	Eina_List *jids;
	MARK_CB;
	DBG("(ctx=%p, ret=%i)", ctx, ret);
	jids = ctx->retval;
	if (ctx->count == 10){
		/* list init */
		jids = eina_list_append(NULL, (intptr_t*)&ret);
	}
	jids = eina_list_append(jids, (intptr_t*)&ret);
	if (ctx->count == eina_list_count(jids) + 10){
		ctx->count = 6;
		ctx->retval = jids;
		REWIND;
		RESUME;
	}

}

static void
network_cb_target_host(Zentific_Ctx *ctx, Eina_List *ret){
	Eina_List *jobs, *l;
	JOB *job;
	const char *s;
	MARK_CB;
	DBG("(ctx=%p, ret=%p)", ctx, ret);
	jobs = ctx->retval;
	if ((!ctx->error) && ret && (eina_list_count(ret) == 2)){
		ctx->count++;
		EINA_LIST_FOREACH(jobs, l, job){
			if (job->target_node == ret->data)
				job->target_host = ret->next->data;
			eina_stringshare_del(ret->data);
			eina_list_free(ret);
		}
		if (--ctx->count == 10){
			ctx->count = 4;
			ctx->retval = jobs;
			REWIND;
			RESUME;
		}
		return;
	}
	EINA_LIST_FREE(jobs, job)
		JOB_free(job);
	EINA_LIST_FREE(ret, s)
		eina_stringshare_del(s);
	RESUME;
}

Eina_Bool _detachAllNodesByNetwork(Zentific_Ctx *ctx, int nwid, int caller_uid){
	JOB *job;
	Eina_List *jobs = NULL, *nodes, *l;
	const char *uuid;
	NETWORK *net;

	switch (ctx->count){
		case 0:
			_isNetworkId(ctx, nwid);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		case 1:
			/* if no nodes are connected to the network, this is a NO-OP */
			_getNodesByNetwork(ctx, nwid);
			CTX_VALUE_RETURN;
			break;
		case 2:
			nodes = CTX_PARAM_GET;
			if (!nodes){
				if (!ctx->error)
					ctx->error = AZY_ERR(UNKNOWN_RESOURCE);
				break;
			}
			/* temp storage */
			ctx->retval = nodes;
			db->getNetwork(db, ctx, nwid);
			if (ctx->error){
				EINA_LIST_FREE(nodes, uuid)
					eina_stringshare_del(uuid);
				ctx->retval = NULL;
				break;
			}
			CTX_VALUE_RETURN;
			break;
		case 3:
			net = CTX_PARAM_GET;
			ctx->count = 10 + eina_list_count(ctx->retval);
			EINA_LIST_FREE(ctx->retval, uuid){
				job = network_job_init(caller_uid, "detachNetwork", network_job_string(ctx, "detachNetwork", NULL, net), "");
				job->target_vm = eina_stringshare_add("");
				job->target_node = uuid;
				db->getNodeNameAndUuid(db, ctx, uuid);
				CTX_CB_ADD(network_cb_target_host);
				ctx->retval = jobs = eina_list_append(jobs, job);
			}
			NETWORK_free(net);

			break;
		case 4:
			jobs = ctx->retval;
			EINA_LIST_FOREACH(jobs, l, job)
				db->detachNetworkFromNode(db, ctx, job->target_node, nwid);
				CTX_VALUE_RETURN;
			ctx->count = 10 + eina_list_count(jobs);
			break;
		case 5:
			ctx->count = 10 + eina_list_count(ctx->retval);
			EINA_LIST_FREE(ctx->retval, job){
				_addJob(ctx, job, caller_uid);
				CTX_CB_ADD(network_cb_jid_collect);
				JOB_free(job);
			}
			break;
		case 6:
			job = JOB_new();
			job->dependencies = CTX_PARAM_GET;
			job->module_name = eina_stringshare_add("parentJob");
			job->module_function = eina_stringshare_add("detachAllNodesByNetwork");
			job->uid = caller_uid;
			job_add(ctx, job, caller_uid);
			JOB_free(job);
			break;
		default:
			ctx->count -= 2;
			if (ctx->count == 10){
				ctx->count = 5;
				REWIND;
				/* trigger immediate rerun */
				RESUME;
			}
			break;
	}
	CTX_RETURN;
}

static void
network_cb_target_node_addr(Zentific_Ctx *ctx, Eina_List *ret){
	JOB *job;
	const char *s;
	Eina_List *jobs;
	MARK_CB;
	DBG("(ctx=%p, ret=%p)", ctx, ret);
	jobs = ctx->retval;
	if ((!ctx->error) && ret && (eina_list_count(ret) == 2)){
			job = eina_list_nth(jobs, eina_list_count(jobs) + 10 - ctx->count);
			job->target_node = ret->data;
			job->target_host = ret->next->data;
			eina_list_free(ret);
		if (--ctx->count == 10){
			ctx->count = 3;
			ctx->retval = jobs;
			REWIND;
			RESUME;
		}
		return;
	}
	EINA_LIST_FREE(jobs, job)
		JOB_free(job);
	EINA_LIST_FREE(ret, s)
		eina_stringshare_del(s);
}

Eina_Bool _detachAllNetIfsByNetwork(Zentific_Ctx *ctx, int nwid, int caller_uid){
	JOB *job;
	NETWORK *net;
	NETIF  *netif;
	Eina_List *netifs, *jobs = NULL;

	switch (ctx->count){
		case 0:
			if(nwid < 0){
				ctx->error = AZY_ERR(INVALID_PARAMETER);
				break;
			}
			_getNetwork(ctx, nwid);
			CTX_VALUE_RETURN;
			break;
		case 1:
			ctx->retval = net = CTX_PARAM_GET;
			if (!net){
				ctx->error = AZY_ERR(UNKNOWN_RESOURCE);
				break;
			}
			db->getVMvifsByNetwork(db, ctx, nwid);
			CTX_VALUE_RETURN;
			break;
		case 2:
			netifs = CTX_PARAM_GET;
			net = ctx->retval;
			if (!netifs){
				/* if no vms are connected to the network, this is a NO-OP */
				ctx->error = AZY_ERR(NO_CHANGE_PERFORMED);
				NETWORK_free(ctx->retval);
				ctx->retval = NULL;
				break;
			}
			ctx->count = 10 + eina_list_count(ctx->retval);
			EINA_LIST_FREE(netifs, netif){
				job = network_job_init(caller_uid, "detachNetIf",
					eina_stringshare_printf("ifmac=%s::ifname=%s::nw=%s::nwmac=%s::nwifname=%s",
					netif->mac, netif->name, net->network, net->mac, net->interface), "");
				job->target_vm = eina_stringshare_ref(netif->uuid);
				db->getVMparentNodeAndAddress(db, ctx, netif->uuid);
				CTX_CB_ADD(network_cb_target_node_addr);
				ctx->retval = jobs = eina_list_append(jobs, job);
				NETIF_free(netif);
			}
			NETWORK_free(net);
			break;
		case 3:
			db->detachAllNetIfsByNetwork(db, ctx, nwid);
			CTX_CB_ADD(cb_int_rewind);
			break;
		case 4:
			ctx->count = 10 + eina_list_count(ctx->retval);
			EINA_LIST_FREE(ctx->retval, job){
				_addJob(ctx, job, caller_uid);
				CTX_CB_ADD(network_cb_jid_collect);
				JOB_free(job);
			}
			break;
		/* NOT A MISTAKE */
		case 6:
			job = JOB_new();
			job->dependencies = CTX_PARAM_GET;
			job->module_name = eina_stringshare_add("parentJob");
			job->module_function = eina_stringshare_add("detachAllNetIfsByNetwork");
			job->uid = caller_uid;
			job_add(ctx, job, caller_uid);
			JOB_free(job);
			break;
		default:
			ERR("something crazy happened");
	}
	CTX_RETURN;
}

Eina_Bool _getNodesByNetwork(Zentific_Ctx *ctx, int nwid){
	db->getNodesByNetwork(db, ctx, nwid);
	CTX_RETURN;
}

Eina_Bool _modifyNetwork(Zentific_Ctx *ctx, int id, NETWORK *net, int caller_uid){
	JOB *ret, *job;
	NETWORK * existing = NULL;
	Eina_List *jobs = NULL, *nodes;
	const char *uuid, *name;

	switch (ctx->count){
		case 0:
			_getNetwork(ctx, id);
			CTX_VALUE_RETURN;
			break;
		case 1:
			existing = CTX_PARAM_GET;
			if (ctx->error || (!existing)){
				if (existing) NETWORK_free(existing);
				break;
			}
			if (NETWORK_eq(net, existing)){
				ctx->error = AZY_ERR(NO_CHANGE_PERFORMED);
				NETWORK_free(existing);
				break;
			}
			NETWORK_free(existing);
			_getNodesByNetwork(ctx, id);
			CTX_VALUE_RETURN;
			break;
		case 2:
			nodes = CTX_PARAM_GET;
			ctx->count = 10 + eina_list_count(nodes);
			EINA_LIST_FREE(nodes, uuid){
				ret = network_job_init(caller_uid, "modifyNodeNetwork", network_job_string(ctx, "modifyNodeNetwork", NULL, net), "");
				ret->target_vm = eina_stringshare_add("");
				ret->target_node = uuid;
				db->getNodeNameAndUuid(db, ctx, uuid);
				CTX_CB_ADD(network_cb_target_host);
				ctx->retval = jobs = eina_list_append(jobs, ret);
			}
			break;
		/* NOT A MISTAKE */
		case 4:
			db->getControlModuleNameByNetwork(db, ctx, id);
			CTX_VALUE_RETURN;
		case 5:
			name = CTX_PARAM_GET;
			if ((!name) || ctx->error){
				eina_stringshare_del(name);
				EINA_LIST_FREE(ctx->retval, job)
					JOB_free(job);
				ctx->retval = NULL;
				ctx->error = AZY_ERR(JOB_CREATION_FAILED);
				break;
			}
			ctx->count = 10 + eina_list_count(ctx->retval);
			EINA_LIST_FREE(ctx->retval, job){
				if (name[0])
					eina_stringshare_replace(&job->module_name, name);
				else
					job->module_name = eina_stringshare_add("parentJob");
				_addJob(ctx, job, caller_uid);
				CTX_CB_ADD(network_cb_jid_collect);
				JOB_free(job);
			}
			eina_stringshare_del(name);
			ctx->retval = NULL;
			break;
		case 6:
			db->modifyNetwork(db, ctx, id, net);
			CTX_CB_ADD(cb_int_rewind);
			break;
		case 7:
			ret = JOB_new();
			ret->dependencies = ctx->retval;
			ctx->retval = NULL;
			ret->module_name = eina_stringshare_add("parentJob");
			ret->module_function = eina_stringshare_add("modifyNetwork");
			ret->uid = caller_uid;
			job_add(ctx, ret, caller_uid);
			JOB_free(ret);
			break;
		default:
			ERR("something crazy happened here");
	}
	CTX_RETURN;
}

Eina_Bool _getNetworkIdByName(Zentific_Ctx *ctx, const char *name){
	db->getNetworkIdByName(db, ctx, name);
	CTX_RETURN;
}

Eina_Bool _getNodesByNetworkFull(Zentific_Ctx *ctx, int nwid) {
	_getNodesByNetwork(ctx, nwid);
	CTX_RETURN;
}

Eina_Bool _getNetworkCount(Zentific_Ctx *ctx){
	db->getNetworkCount(db, ctx);
	CTX_RETURN;
}
