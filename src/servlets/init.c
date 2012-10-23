#include "zentific.h"
#include <Eina.h>
#include <Network.azy_server.h>
#include <Node.azy_server.h>
#include <Platform.azy_server.h>
#include <Scheduler.azy_server.h>
#include <Session.azy_server.h>
#include <Storage.azy_server.h>
//#include <system.azy_server.h>
#include <User.azy_server.h>
#include <VM.azy_server.h>
#include <Zentific.azy_server.h>

Eina_Array *defs;

Eina_Array *
zrpc_base_def(void)
{
	defs = eina_array_new(10);
	eina_array_push(defs, Network_module_def());
	eina_array_push(defs, Node_module_def());
	eina_array_push(defs, Platform_module_def());
	eina_array_push(defs, Scheduler_module_def());
	eina_array_push(defs, Session_module_def());
	eina_array_push(defs, Storage_module_def());
//	eina_array_push(defs, system_module_def());
	eina_array_push(defs, User_module_def());
	eina_array_push(defs, VM_module_def());
	eina_array_push(defs, Zentific_module_def());

	return defs;
}
