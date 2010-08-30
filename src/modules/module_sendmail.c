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

uint32_t module_sendmail_init(struct ocsim_context *ctx)
{
	int			ret;
	struct ocsim_module	*module = NULL;

	module = openchangesim_module_init(ctx, "sendmail", "sendmail scenario");
	ret = openchangesim_module_register(ctx, module);

	return ret;
}

/**
   \details Create a sample mail with attachment
 */
uint32_t module_sendmail_run(TALLOC_CTX *mem_ctx, struct mapi_session *session)
{
	enum MAPISTATUS		retval;
	mapi_object_t		obj_store;
	mapi_object_t		obj_outbox;
	mapi_object_t		obj_message;
	struct SPropTagArray	*SPropTagArray;
	struct SRowSet		*SRowSet = NULL;
	struct SPropTagArray   	*flaglist = NULL;
	struct SPropValue	SPropValue;
	struct SPropValue	lpProps[4];
	const char	       	*username[2];
	mapi_id_t		id_outbox;
	char			*subject;
	char			*body;
	uint32_t		msgflag;
	uint32_t		format;
	
	/* Log onto the store */
	mapi_object_init(&obj_store);
	retval = OpenMsgStore(session, &obj_store);
	if (retval) {
		mapi_errstr("OpenMsgStore", GetLastError());
		return OCSIM_ERROR;
	}

	/* Open default outbox folder */
	/* retval = GetDefaultFolder(&obj_store, &id_outbox, olFolderSentMail); */
	retval = GetDefaultFolder(&obj_store, &id_outbox, olFolderInbox);
	if (retval) {
		mapi_errstr("GetDefaultFolder", GetLastError());
		return OCSIM_ERROR;
	}

	mapi_object_init(&obj_outbox);
	retval = OpenFolder(&obj_store, id_outbox, &obj_outbox);
	if (retval) {
		mapi_errstr("OpenFolder", GetLastError());
		return OCSIM_ERROR;
	}

	/* Create the message */
	mapi_object_init(&obj_message);
	retval = CreateMessage(&obj_outbox, &obj_message);
	if (retval) {
		mapi_errstr("CreateMessage", GetLastError());
		return OCSIM_ERROR;
	}


	/* Resolve current username */
	SPropTagArray = set_SPropTagArray(mem_ctx, 0xA, 
					  PR_ENTRYID,
					  PR_DISPLAY_NAME_UNICODE,
					  PR_OBJECT_TYPE,
					  PR_DISPLAY_TYPE,
					  PR_TRANSMITTABLE_DISPLAY_NAME_UNICODE,
					  PR_EMAIL_ADDRESS_UNICODE,
					  PR_ADDRTYPE_UNICODE,
					  PR_SEND_RICH_INFO,
					  PR_7BIT_DISPLAY_NAME_UNICODE,
					  PR_SMTP_ADDRESS_UNICODE);
	username[0] = (char *)session->profile->mailbox;
	username[1] = NULL;

	SRowSet = talloc_zero(mem_ctx, struct SRowSet);
	flaglist = talloc_zero(mem_ctx, struct SPropTagArray);

	retval = ResolveNames(mapi_object_get_session(&obj_message), username, SPropTagArray, 
			      &SRowSet, &flaglist, MAPI_UNICODE);
	MAPIFreeBuffer(SPropTagArray);
	if (retval != MAPI_E_SUCCESS) {
		mapi_errstr("ResolveNames", GetLastError());
		talloc_free(SRowSet);
		talloc_free(SPropTagArray);
		talloc_free(flaglist);
		return OCSIM_ERROR;
	}

	/* Set Recipients */
	SetRecipientType(&(SRowSet->aRow[0]), MAPI_TO);

	SPropValue.ulPropTag = PR_SEND_INTERNET_ENCODING;
	SPropValue.value.l = 0;
	SRowSet_propcpy(mem_ctx, SRowSet, SPropValue);

	retval = ModifyRecipients(&obj_message, SRowSet);
	MAPIFreeBuffer(SRowSet);
	MAPIFreeBuffer(flaglist);
	if (retval != MAPI_E_SUCCESS) {
		mapi_errstr("ModifyRecipient", GetLastError());
		return OCSIM_ERROR;
	}

	/* Set message properties */
	msgflag = MSGFLAG_SUBMIT;
	subject = talloc_asprintf(mem_ctx, "%s Mail from %s\n", DFLT_SUBJECT_PREFIX, session->profile->mailbox);
	set_SPropValue_proptag(&lpProps[0], PR_SUBJECT, (const void *) subject);
	set_SPropValue_proptag(&lpProps[1], PR_MESSAGE_FLAGS, (const void *)&msgflag);
	body = talloc_asprintf(mem_ctx, "Body of message with subject: %s", subject);
	set_SPropValue_proptag(&lpProps[2], PR_BODY, (const void *)body);
	format = EDITOR_FORMAT_PLAINTEXT;
	set_SPropValue_proptag(&lpProps[3], PR_MSG_EDITOR_FORMAT, (const void *)&format);

	retval = SetProps(&obj_message, lpProps, 4);
	talloc_free(subject);
	talloc_free(body);
	if (retval != MAPI_E_SUCCESS) {
		mapi_errstr("SetProps", GetLastError());
		return OCSIM_ERROR;
	}

	/* Save changes on message */
	retval = SaveChangesMessage(&obj_outbox, &obj_message, KeepOpenReadOnly);
	if (retval) {
		mapi_errstr("SaveChangesMessage", GetLastError());
		return OCSIM_ERROR;
	}

	/* Submit the message */
	/* retval = SubmitMessage(&obj_message); */
	/* if (retval) { */
	/* 	fprintf(stderr, "error in SubmitMessage: 0x%x\n", GetLastError()); */
	/* 	mapi_errstr("SubmitMessage", GetLastError()); */
	/* 	return OCSIM_ERROR; */
	/* } */

	mapi_object_release(&obj_message);
	mapi_object_release(&obj_outbox);
	mapi_object_release(&obj_store);

	return OCSIM_SUCCESS;
}
