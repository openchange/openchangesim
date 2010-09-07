/*
   OpenChangeSim cleanup module

   OpenChange Project

   Copyright (C) Julien Kerihuel 2010

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "src/openchangesim.h"

/**
   \details Very basic cleanup script: empty Inbox, Outbox and Sent Items folders
 */
uint32_t module_cleanup_run(TALLOC_CTX *mem_ctx, struct mapi_session *session)
{
	enum MAPISTATUS		retval;
	mapi_object_t		obj_store;
	mapi_object_t		obj_folder;
	mapi_id_t		id;
	uint32_t		folders[] = { olFolderInbox, olFolderOutbox, olFolderSentMail, 0};
	int			i;

	/* Log onto the store */
	mapi_object_init(&obj_store);
	retval = OpenMsgStore(session, &obj_store);
	if (retval) {
		mapi_errstr("OpenMsgStore", GetLastError());
		return OCSIM_ERROR;
	}

	/* Open and cleanup folders */
	for (i = 0; folders[i] != 0; i++) {
		retval = GetDefaultFolder(&obj_store, &id, folders[i]);
		if (retval) {
			mapi_errstr("GetDefaultFolder", GetLastError());
			return OCSIM_ERROR;
		}

		mapi_object_init(&obj_folder);
		retval = OpenFolder(&obj_store, id, &obj_folder);
		if (retval) {
			mapi_errstr("OpenFolder", GetLastError());
			return OCSIM_ERROR;
		}

		retval = EmptyFolder(&obj_folder);
		if (retval) {
			mapi_errstr("EmptyFolder", GetLastError());
		}

		mapi_object_release(&obj_folder);
	}

	return OCSIM_SUCCESS;
}


uint32_t module_cleanup_init(struct ocsim_context *ctx)
{
	int			ret;
	struct ocsim_module	*module = NULL;

	module = openchangesim_module_init(ctx, "cleanup", "cleanup mailboxes");
	module->set_ref_count = module_set_ref_count;
	module->get_ref_count = module_get_ref_count;
	module->private_data = module_get_scenario_data(ctx, FETCHMAIL_MODULE_NAME);

	ret = openchangesim_module_register(ctx, module);

	return ret;
}
