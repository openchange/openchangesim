/*
   OpenChangeSim fetchmail module

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

static void *fetchmail_get_propval(struct SRow *aRow, 
				   uint32_t proptag)
{
	const char	*str;

	if (((proptag & 0xFFFF) == PT_STRING8) ||
	    ((proptag & 0xFFFF) == PT_UNICODE)) {
		proptag = (proptag & 0xFFFF0000) | PT_UNICODE;
		str = (const char *) find_SPropValue_data(aRow, proptag);
		if (str) return (void *)str;

		proptag = (proptag & 0xFFFF0000) | PT_STRING8;
		str = (const char *) find_SPropValue_data(aRow, proptag);
		return (void *)str;
	} 

	return (void *)find_SPropValue_data(aRow, proptag);
}


static enum MAPISTATUS fetchmail_get_stream(TALLOC_CTX *mem_ctx,
					    mapi_object_t *obj_stream, 
					    DATA_BLOB *body)
{
	enum MAPISTATUS	retval;
	uint16_t	read_size;
	uint8_t		buf[0x1000];

	body->length = 0;
	body->data = talloc_zero(mem_ctx, uint8_t);

	do {
		retval = ReadStream(obj_stream, buf, 0x1000, &read_size);
		MAPI_RETVAL_IF(retval, GetLastError(), body->data);
		if (read_size) {
			body->data = talloc_realloc(mem_ctx, body->data, uint8_t,
						    body->length + read_size);
			memcpy(&(body->data[body->length]), buf, read_size);
			body->length += read_size;
		}
	} while (read_size);

	errno = 0;
	return MAPI_E_SUCCESS;
}


/*
 * Fetch the body given PR_MSG_EDITOR_FORMAT property value
 */
static enum MAPISTATUS fetchmail_get_body(TALLOC_CTX *mem_ctx,
					  mapi_object_t *obj_message,
					  struct SRow *aRow,
					  DATA_BLOB *body)
{
	enum MAPISTATUS			retval;
	const struct SBinary_short	*bin;
	mapi_object_t			obj_stream;
	char				*data;
	uint8_t				format;

	/* Sanity checks */
	MAPI_RETVAL_IF(!obj_message, MAPI_E_INVALID_PARAMETER, NULL);

	/* initialize body DATA_BLOB */
	body->data = NULL;
	body->length = 0;

	retval = GetBestBody(obj_message, &format);
	MAPI_RETVAL_IF(retval, retval, NULL);

	switch (format) {
	case olEditorText:
		data = fetchmail_get_propval(aRow, PR_BODY_UNICODE);
		if (data) {
			body->data = talloc_memdup(mem_ctx, data, strlen(data));
			body->length = strlen(data);
		} else {
			mapi_object_init(&obj_stream);
			retval = OpenStream(obj_message, PR_BODY_UNICODE, 0, &obj_stream);
			MAPI_RETVAL_IF(retval, GetLastError(), NULL);
			
			retval = fetchmail_get_stream(mem_ctx, &obj_stream, body);
			MAPI_RETVAL_IF(retval, GetLastError(), NULL);
			
			mapi_object_release(&obj_stream);
		}
		break;
	case olEditorHTML:
		bin = (const struct SBinary_short *) fetchmail_get_propval(aRow, PR_HTML);
		if (bin) {
			body->data = talloc_memdup(mem_ctx, bin->lpb, bin->cb);
			body->length = bin->cb;
		} else {
			mapi_object_init(&obj_stream);
			retval = OpenStream(obj_message, PR_HTML, 0, &obj_stream);
			MAPI_RETVAL_IF(retval, GetLastError(), NULL);

			retval = fetchmail_get_stream(mem_ctx, &obj_stream, body);
			MAPI_RETVAL_IF(retval, GetLastError(), NULL);

			mapi_object_release(&obj_stream);
		}			
		break;
	case olEditorRTF:
		mapi_object_init(&obj_stream);

		retval = OpenStream(obj_message, PR_RTF_COMPRESSED, 0, &obj_stream);
		MAPI_RETVAL_IF(retval, GetLastError(), NULL);

		retval = WrapCompressedRTFStream(&obj_stream, body);
		MAPI_RETVAL_IF(retval, GetLastError(), NULL);

		mapi_object_release(&obj_stream);
		break;
	default:
		DEBUG(0, ("Undefined Body\n"));
		break;
	}

	return MAPI_E_SUCCESS;
}


static enum MAPISTATUS fetchmail_get_contents(TALLOC_CTX *mem_ctx,
					      mapi_object_t *obj_message)
{
	enum MAPISTATUS			retval;
	struct SPropTagArray		*SPropTagArray;
	struct SPropValue		*lpProps;
	struct SRow			aRow;
	uint32_t			count;
	DATA_BLOB			body;

	/* Build the array of properties we want to fetch */
	SPropTagArray = set_SPropTagArray(mem_ctx, 0x13,
					  PR_INTERNET_MESSAGE_ID,
					  PR_INTERNET_MESSAGE_ID_UNICODE,
					  PR_CONVERSATION_TOPIC,
					  PR_CONVERSATION_TOPIC_UNICODE,
					  PR_MSG_EDITOR_FORMAT,
					  PR_BODY,
					  PR_BODY_UNICODE,
					  PR_HTML,
					  PR_RTF_COMPRESSED,
					  PR_SENT_REPRESENTING_NAME,
					  PR_SENT_REPRESENTING_NAME_UNICODE,
					  PR_DISPLAY_TO,
					  PR_DISPLAY_TO_UNICODE,
					  PR_DISPLAY_CC,
					  PR_DISPLAY_CC_UNICODE,
					  PR_DISPLAY_BCC,
					  PR_DISPLAY_BCC_UNICODE,
					  PR_HASATTACH,
					  PR_MESSAGE_CODEPAGE);
	lpProps = talloc_zero(mem_ctx, struct SPropValue);
	retval = GetProps(obj_message, 0, SPropTagArray, &lpProps, &count);
	MAPIFreeBuffer(SPropTagArray);
	MAPI_RETVAL_IF(retval, retval, NULL);

	/* Build a SRow structure */
	aRow.ulAdrEntryPad = 0;
	aRow.cValues = count;
	aRow.lpProps = lpProps;

	retval = fetchmail_get_body(mem_ctx, obj_message, &aRow, &body);
	MAPI_RETVAL_IF(retval, GetLastError(), NULL);
	
	if (body.length) {
		talloc_free(body.data);
	} 

	return MAPI_E_SUCCESS;
}

static uint32_t _module_fetchmail_run(TALLOC_CTX *mem_ctx, 
				      struct mapi_session *session)
{
	enum MAPISTATUS		retval;
	mapi_object_t		obj_store;
	mapi_object_t		obj_inbox;
	mapi_object_t		obj_table;
	mapi_object_t		obj_message;
	mapi_object_t		obj_table_attach;
	mapi_object_t		obj_attach;
	mapi_object_t		obj_stream;
	uint64_t		id_inbox = 0;
	struct SPropTagArray	*SPropTagArray;
	struct SRowSet		SRowSet;
	struct SRowSet		SRowSet_attach;
	uint32_t		i, j;
	uint32_t		count;
	const uint8_t		*has_attach;
	const uint32_t		*attach_num;
	uint16_t		read_size;
	unsigned char		buf[MAX_READ_SIZE];

	/* Log onto the store */
	memset(&obj_store, 0, sizeof(mapi_object_t));
	memset(&obj_inbox, 0, sizeof(mapi_object_t));
	memset(&obj_table, 0, sizeof(mapi_object_t));
	memset(&obj_message, 0, sizeof(mapi_object_t));
	memset(&obj_table_attach, 0, sizeof(mapi_object_t));
	memset(&obj_attach, 0, sizeof(mapi_object_t));
	memset(&obj_stream, 0, sizeof(mapi_object_t));

	mapi_object_init(&obj_store);
	retval = OpenMsgStore(session, &obj_store);
	if (retval) {
		mapi_errstr("OpenMsgStore", GetLastError());
		return OCSIM_ERROR;
	}

	/* Open default receive folder (Inbox) */
	retval = GetReceiveFolder(&obj_store, &id_inbox, NULL);
	if (retval) {
		mapi_errstr("GetReceiveFolder", GetLastError());
		return OCSIM_ERROR;
	}

	retval = OpenFolder(&obj_store, id_inbox, &obj_inbox);
	if (retval) {
		mapi_errstr("OpenFolder", GetLastError());
		return OCSIM_ERROR;
	}

	/* Open the contents table and customize the view */
	mapi_object_init(&obj_table);
	retval = GetContentsTable(&obj_inbox, &obj_table, 0, &count);
	if (retval) {
		mapi_errstr("GetContentsTable", GetLastError());
		return OCSIM_ERROR;
	}

	SPropTagArray = set_SPropTagArray(mem_ctx, 0x5,
					  PR_FID,
					  PR_MID,
					  PR_INST_ID,
					  PR_INSTANCE_NUM,
					  PR_SUBJECT);
	retval = SetColumns(&obj_table, SPropTagArray);
	MAPIFreeBuffer(SPropTagArray);
	if (retval) {
		mapi_errstr("SetColumns", GetLastError());		
		return OCSIM_ERROR;
	}

	/* Retrieve the messages and attachments */
	while ((retval = QueryRows(&obj_table, count, TBL_ADVANCE, &SRowSet)) != MAPI_E_NOT_FOUND && SRowSet.cRows) {
		count -= SRowSet.cRows;
		for (i = 0; i < SRowSet.cRows; i++) {
			mapi_object_init(&obj_message);
			retval = OpenMessage(&obj_store,
					     SRowSet.aRow[i].lpProps[0].value.d,
					     SRowSet.aRow[i].lpProps[0].value.d,
					     &obj_message, 0);
			if (GetLastError() == MAPI_E_SUCCESS) {
				struct SPropValue	*lpProps;
				struct SRow		aRow;

				SPropTagArray = set_SPropTagArray(mem_ctx, 0x1, PR_HASATTACH);
				lpProps = talloc_zero(mem_ctx, struct SPropValue);
				retval = GetProps(&obj_message, 0, SPropTagArray, &lpProps, &count);
				MAPIFreeBuffer(SPropTagArray);
				if (retval) {
					mapi_errstr("GetProps", GetLastError());
					return OCSIM_ERROR;
				}

				aRow.ulAdrEntryPad = 0;
				aRow.cValues = count;
				aRow.lpProps = lpProps;

				retval = fetchmail_get_contents(mem_ctx, &obj_message);

				has_attach = (const uint8_t *) get_SPropValue_SRow_data(&aRow, PR_HASATTACH);
				if (has_attach && *has_attach) {
					mapi_object_init(&obj_table_attach);
					retval = GetAttachmentTable(&obj_message, &obj_table_attach);
					if (retval == MAPI_E_SUCCESS) {
						SPropTagArray = set_SPropTagArray(mem_ctx, 0x1, PR_ATTACH_NUM);
						retval = SetColumns(&obj_table_attach, SPropTagArray);
						if (retval != MAPI_E_SUCCESS) return retval;
						MAPIFreeBuffer(SPropTagArray);

						retval = QueryRows(&obj_table_attach, 0xA, TBL_ADVANCE, &SRowSet_attach);
						if (retval != MAPI_E_SUCCESS) return retval;

						for (j = 0; j < SRowSet_attach.cRows; j++) {
							attach_num = (const uint32_t *) find_SPropValue_data(&(SRowSet_attach.aRow[j]), PR_ATTACH_NUM);
							mapi_object_init(&obj_attach);
							retval = OpenAttach(&obj_message, *attach_num, &obj_attach);
							if (retval == MAPI_E_SUCCESS) {
								struct SPropValue	*lpProps2;
								uint32_t		count2;

								SPropTagArray = set_SPropTagArray(mem_ctx, 0x3,
												  PR_ATTACH_FILENAME,
												  PR_ATTACH_LONG_FILENAME,
												  PR_ATTACH_SIZE);
								lpProps2 = talloc_zero(mem_ctx, struct SPropValue);
								retval = GetProps(&obj_attach, 0, SPropTagArray, &lpProps2, &count2);
								MAPIFreeBuffer(SPropTagArray);
								if (retval != MAPI_E_SUCCESS) {
									MAPIFreeBuffer(lpProps2);
									return retval;
								}
								MAPIFreeBuffer(lpProps2);

								mapi_object_init(&obj_stream);
								retval = OpenStream(&obj_attach, PR_ATTACH_DATA_BIN, 0, &obj_stream);
								if (retval != MAPI_E_SUCCESS) return retval;

								read_size = 0;
								do {
									retval = ReadStream(&obj_stream, buf, MAX_READ_SIZE, &read_size);
									if (retval != MAPI_E_SUCCESS) break;
								} while (read_size);

								mapi_object_release(&obj_stream);
								mapi_object_release(&obj_attach);
							}
						}
					}
				}

				MAPIFreeBuffer(lpProps);
			}
			mapi_object_release(&obj_message);
		}
	}

	Logoff(&obj_store);

	/* talloc_free(mem_ctx); */
	mapi_object_release(&obj_store);

	return OCSIM_SUCCESS;
}

static uint32_t module_fetchmail_run(TALLOC_CTX *mem_ctx, 
				     struct ocsim_scenario_case *cases, 
				     struct mapi_session *session)
{
	struct ocsim_log		*log;
	int ret;

	log = openchangesim_log_init(mem_ctx);
	openchangesim_log_start(log);
	ret = _module_fetchmail_run(mem_ctx, session);
	if (ret != OCSIM_SUCCESS) {
		openchangesim_log_string("%s module returned: %s",
						FETCHMAIL_MODULE_NAME,
						mapi_get_errstr(GetLastError()));
	}
	openchangesim_log_end(log, FETCHMAIL_MODULE_NAME, NULL, session->profile->localaddr);
	openchangesim_log_close(log);

	return OCSIM_SUCCESS;
}


/**
   \details Initialize the fetchmail module

   \param ctx pointer to the ocsim_context

   \return OCSIM_SUCCESS on success, otherwise OCSIM_ERROR
 */
uint32_t module_fetchmail_init(struct ocsim_context *ctx)
{
	int			ret;
	struct ocsim_module	*module = NULL;

	module = openchangesim_module_init(ctx, "fetchmail", "fetchmail scenario");
	module->run = module_fetchmail_run;
	module->set_ref_count = module_set_ref_count;
	module->get_ref_count = module_get_ref_count;
	module->scenario = module_get_scenario(ctx, FETCHMAIL_MODULE_NAME);
	module->cases = module_get_scenario_data(ctx, FETCHMAIL_MODULE_NAME);

	if (module->scenario)
		ret = openchangesim_module_register(ctx, module);

	return ret;
}

