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

uint32_t module_fetchmail_init(struct ocsim_context *ctx)
{
	int			ret;
	struct ocsim_module	*module = NULL;

	module = openchangesim_module_init(ctx, "fetchmail", "fetchmail scenario");
	ret = openchangesim_module_register(ctx, module);

	return ret;
}

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
	retval = GetProps(obj_message, SPropTagArray, &lpProps, &count);
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

uint32_t module_fetchmail_run(TALLOC_CTX *mem_ctx, struct mapi_session *session)
{
	enum MAPISTATUS		retval;
	mapi_object_t		obj_store;
	mapi_object_t		obj_inbox;
	mapi_object_t		obj_table;
	mapi_object_t		obj_message;
	uint64_t		id_inbox = 0;
	struct SPropTagArray	*SPropTagArray;
	struct SRowSet		SRowSet;
	uint32_t		i;
	uint32_t		count;

	/* Log onto the store */
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
				retval = GetProps(&obj_message, SPropTagArray, &lpProps, &count);
				MAPIFreeBuffer(SPropTagArray);
				if (retval) {
					mapi_errstr("GetProps", GetLastError());		
					return OCSIM_ERROR;
				}

				aRow.ulAdrEntryPad = 0;
				aRow.cValues = count;
				aRow.lpProps = lpProps;

				retval = fetchmail_get_contents(mem_ctx, &obj_message);
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