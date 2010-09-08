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

struct attach {
	const char		*filename;
	struct Binary_r		bin;
	int			fd;
};

struct sendmail_data {
	struct Binary_r		pr_html;
	struct Binary_r		pr_rtf;
	struct attach		attach;
	uint32_t		attach_num;
};

/**
 * read a file and store it in the appropriate structure element
 */
static bool sdata_read_file(TALLOC_CTX *mem_ctx, const char *filename, 
			      struct sendmail_data *sdata, uint32_t mapitag)
{
	struct stat	sb;
	int		fd;

	if ((fd = open(filename, O_RDONLY)) == -1) {
		printf("Error while opening %s\n", filename);
		return false;
	}
	/* stat the file */
	if (fstat(fd, &sb) != 0) {
		close(fd);
		return false;
	}

	switch (mapitag) {
	case PR_HTML:
		sdata->pr_html.lpb = talloc_size(mem_ctx, sb.st_size);
		sdata->pr_html.cb = read(fd, sdata->pr_html.lpb, sb.st_size);
		close(fd);
		break;
	case PR_RTF_COMPRESSED:
		sdata->pr_rtf.lpb = talloc_size(mem_ctx, sb.st_size);
		sdata->pr_rtf.cb = read(fd, sdata->pr_rtf.lpb, sb.st_size);
		close(fd);
		break;
	case PR_ATTACH_DATA_BIN:
		sdata->attach.bin.lpb = talloc_size(mem_ctx, sb.st_size);
		sdata->attach.bin.cb = sb.st_size;
		if ((sdata->attach.bin.lpb = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0)) == (void *) -1) {
			perror("mmap");
			close(fd);
			return false;
		}
		sdata->attach.fd = fd;
		close(fd);
		break;
	default:
		printf("unsupported MAPITAG: %s\n", get_proptag_name(mapitag));
		close(fd);
		return false;
		break;
	}

	return true;
}


static const char *get_filename(const char *filename)
{
	const char *substr;

	if (!filename) return NULL;

	substr = rindex(filename, '/');
	if (substr) return substr;

	return filename;
}


/**
 * Write a stream with MAX_READ_SIZE chunks
 */

#define	MAX_READ_SIZE	0x1000

static bool sendmail_stream(TALLOC_CTX *mem_ctx, mapi_object_t obj_parent, 
			    mapi_object_t obj_stream, uint32_t mapitag, 
			    uint32_t access_flags, struct Binary_r bin)
{
	enum MAPISTATUS	retval;
	DATA_BLOB	stream;
	uint32_t	size;
	uint32_t	offset;
	uint16_t	read_size;

	/* Open a stream on the parent for the given property */
	retval = OpenStream(&obj_parent, mapitag, access_flags, &obj_stream);
	if (retval != MAPI_E_SUCCESS) return false;

	/* WriteStream operation */
	size = (bin.cb > MAX_READ_SIZE) ? MAX_READ_SIZE : bin.cb;
	offset = 0;
	while (offset <= bin.cb) {
		stream.length = size;
		stream.data = talloc_size(mem_ctx, size);
		memcpy(stream.data, bin.lpb + offset, size);
		
		retval = WriteStream(&obj_stream, &stream, &read_size);
		talloc_free(stream.data);
		if (retval != MAPI_E_SUCCESS) return false;

		/* Exit when there is nothing left to write */
		if (!read_size) return true;
		
		offset += read_size;

		if ((offset + size) > bin.cb) {
			size = bin.cb - offset;
		}

		if (!size) break;
	}

	return true;
}


/**
   \details Create a sample mail with attachment
 */
static uint32_t _module_sendmail_run(TALLOC_CTX *mem_ctx, 
				     struct ocsim_scenario_sendmail *sendmail, 
				     struct mapi_session *session)
{
	enum MAPISTATUS		retval;
	mapi_object_t		obj_store;
	mapi_object_t		obj_outbox;
	mapi_object_t		obj_message;
	mapi_object_t		obj_stream;
	struct SPropTagArray	*SPropTagArray;
	struct SRowSet		*SRowSet = NULL;
	struct SPropTagArray   	*flaglist = NULL;
	struct SPropValue	SPropValue;
	struct SPropValue	lpProps[4];
	const char	       	*username[2];
	mapi_id_t		id_outbox;
	char			*subject;
	char			*body = NULL;
	uint32_t		msgflag;
	uint32_t		format;
	bool			bret;
	int			prop_index = 0;
	int			i;
	
	/* Log onto the store */
	mapi_object_init(&obj_store);
	retval = OpenMsgStore(session, &obj_store);
	if (retval) {
		mapi_errstr("OpenMsgStore", GetLastError());
		return OCSIM_ERROR;
	}

	/* Open default outbox folder */
	retval = GetDefaultFolder(&obj_store, &id_outbox, olFolderSentMail);
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
	msgflag = MSGFLAG_UNSENT|MSGFLAG_FROMME;
	subject = talloc_asprintf(mem_ctx, "%s Mail from %s\n", DFLT_SUBJECT_PREFIX, session->profile->mailbox);
	set_SPropValue_proptag(&lpProps[prop_index], PR_SUBJECT, (const void *) subject);
	prop_index++;
	set_SPropValue_proptag(&lpProps[prop_index], PR_MESSAGE_FLAGS, (const void *)&msgflag);
	prop_index++;

	/* Set the message body given sendmail cae parameters */
	if (sendmail->body_type != OCSIM_BODY_NONE) {
		switch (sendmail->body_type) {
		case OCSIM_BODY_UTF8_INLINE:
			format = EDITOR_FORMAT_PLAINTEXT;
			if (strlen(sendmail->body_inline) > MAX_READ_SIZE) {
				struct Binary_r bin;
				
				bin.lpb = (uint8_t *)sendmail->body_inline;
				bin.cb = strlen(sendmail->body_inline);
				sendmail_stream(mem_ctx, obj_message, obj_stream, PR_BODY_UNICODE, 2, bin);
			} else {
				set_SPropValue_proptag(&lpProps[prop_index], PR_BODY_UNICODE, (const void *)sendmail->body_inline);
				prop_index++;
			}
			break;
		case OCSIM_BODY_HTML_INLINE:
			format = EDITOR_FORMAT_HTML;
			if (strlen(sendmail->body_inline) > MAX_READ_SIZE) {
				struct Binary_r bin;
				
				bin.lpb = (uint8_t *)sendmail->body_inline;
				bin.cb = strlen(sendmail->body_inline);
				sendmail_stream(mem_ctx, obj_message, obj_stream, PR_HTML, 2, bin);
			} else {
				struct SBinary_short bin;

				bin.cb = strlen(sendmail->body_inline);
				bin.lpb = (uint8_t *)sendmail->body_inline;
				set_SPropValue_proptag(&lpProps[prop_index], PR_HTML, (void *)&bin);
				prop_index++;
			}
			break;
		case OCSIM_BODY_UTF8_FILE:
		{
			
		}
			break;
		case OCSIM_BODY_HTML_FILE:
		{
			struct sendmail_data	sdata;

			format = EDITOR_FORMAT_HTML;
			bret = sdata_read_file(mem_ctx, sendmail->body_file, &sdata, PR_HTML);
			if (bret == false) {
				fprintf(stderr, "Unable to read file %s\n", sendmail->body_file);
				return OCSIM_ERROR;
			} 

			mapi_object_init(&obj_stream);
			sendmail_stream(mem_ctx, obj_message, obj_stream, PR_HTML, 2, sdata.pr_html);
			mapi_object_release(&obj_stream);

			talloc_free(sdata.pr_html.lpb);
		}
			break;
		case OCSIM_BODY_RTF_FILE:
		{
			struct sendmail_data	sdata;

			format = EDITOR_FORMAT_RTF;
			bret = sdata_read_file(mem_ctx, sendmail->body_file, &sdata, PR_RTF_COMPRESSED);
			if (bret == false) {
				fprintf(stderr, "Unable to read file %s\n", sendmail->body_file);
				return OCSIM_ERROR;
			}
			mapi_object_init(&obj_stream);
			sendmail_stream(mem_ctx, obj_message, obj_stream, PR_RTF_COMPRESSED, 2, sdata.pr_rtf);
			mapi_object_release(&obj_stream);

			talloc_free(sdata.pr_rtf.lpb);
		}
			break;
		default:
			break;
		}
	} else {
		format = EDITOR_FORMAT_PLAINTEXT;
		body = talloc_asprintf(mem_ctx, "Body of message with subject: %s", subject);
		set_SPropValue_proptag(&lpProps[prop_index], PR_BODY, (const void *)body);
		prop_index++;
	}

	set_SPropValue_proptag(&lpProps[prop_index], PR_MSG_EDITOR_FORMAT, (const void *)&format);
	prop_index++;

	retval = SetProps(&obj_message, lpProps, prop_index);
	talloc_free(subject);
	talloc_free(body);
	if (retval != MAPI_E_SUCCESS) {
		mapi_errstr("SetProps", GetLastError());
		return OCSIM_ERROR;
	}

	/* Add attachments */
	if (sendmail->attachment_count) {
		for (i = 0; i < sendmail->attachment_count; i++) {
			struct sendmail_data	sdata;
			mapi_object_t		obj_attach;
			struct SPropValue	props_attach[3];
			uint32_t		count_props_attach;

			mapi_object_init(&obj_attach);
			
			retval = CreateAttach(&obj_message, &obj_attach);
			if (retval != MAPI_E_SUCCESS) return retval;
		
			props_attach[0].ulPropTag = PR_ATTACH_METHOD;
			props_attach[0].value.l = ATTACH_BY_VALUE;
			props_attach[1].ulPropTag = PR_RENDERING_POSITION;
			props_attach[1].value.l = 0;
			props_attach[2].ulPropTag = PR_ATTACH_FILENAME;
			props_attach[2].value.lpszA = get_filename(sendmail->attachments[i]);
			count_props_attach = 3;

			/* SetProps */
			retval = SetProps(&obj_attach, props_attach, count_props_attach);
			if (retval != MAPI_E_SUCCESS) return retval;

			/* Stream operations */
			memset(&sdata, 0, sizeof (struct sendmail_data));
			bret = sdata_read_file(mem_ctx, sendmail->attachments[i], &sdata, PR_ATTACH_DATA_BIN);
			if (bret == false) {
				fprintf(stderr, "Unable to read file %s\n", sendmail->body_file);
				return OCSIM_ERROR;
			}

			mapi_object_init(&obj_stream);
			sendmail_stream(mem_ctx, obj_attach, obj_stream, PR_ATTACH_DATA_BIN, 2, sdata.attach.bin);
			mapi_object_release(&obj_stream);
/* 			talloc_free(sdata.attach.bin.lpb); */

			/* Save changes on attachment */
			retval = SaveChangesAttachment(&obj_message, &obj_attach, KeepOpenReadWrite);
			if (retval != MAPI_E_SUCCESS) return retval;

			mapi_object_release(&obj_attach);
		}
	}

	/* Save changes on message */
	/* retval = SaveChangesMessage(&obj_outbox, &obj_message, KeepOpenReadOnly); */
	/* if (retval) { */
	/* 	mapi_errstr("SaveChangesMessage", GetLastError()); */
	/* 	return OCSIM_ERROR; */
	/* } */

	/* Submit the message */
	retval = SubmitMessage(&obj_message);
	if (retval) {
		fprintf(stderr, "error in SubmitMessage: 0x%x\n", GetLastError());
		mapi_errstr("SubmitMessage", GetLastError());
		return OCSIM_ERROR;
	}

	mapi_object_release(&obj_message);
	mapi_object_release(&obj_outbox);
	mapi_object_release(&obj_store);

	return OCSIM_SUCCESS;
}

static uint32_t module_sendmail_run(TALLOC_CTX *mem_ctx, 
				    struct ocsim_scenario_case *cases, 
				    struct mapi_session *session)
{
	struct ocsim_scenario_case	*el;
	struct ocsim_scenario_sendmail	*sendmail;

	for (el = cases; el; el = el->next) {
		sendmail = (struct ocsim_scenario_sendmail *) el->private_data;
		_module_sendmail_run(mem_ctx, sendmail, session);
	}

	return OCSIM_SUCCESS;
}

/**
   \details Initialize the sendmail module

   \param ctx pointer to the ocsim_context

   \return OCSIM_SUCCESS on success, otherwise OCSIM_ERROR
 */
uint32_t module_sendmail_init(struct ocsim_context *ctx)
{
	int			ret;
	struct ocsim_module	*module = NULL;

	module = openchangesim_module_init(ctx, SENDMAIL_MODULE_NAME, "sendmail scenario");
	module->run = module_sendmail_run;
	module->set_ref_count = module_set_ref_count;
	module->get_ref_count = module_get_ref_count;
	module->scenario = module_get_scenario(ctx, SENDMAIL_MODULE_NAME);
	module->cases = module_get_scenario_data(ctx, SENDMAIL_MODULE_NAME);

	ret = openchangesim_module_register(ctx, module);

	return ret;
}

