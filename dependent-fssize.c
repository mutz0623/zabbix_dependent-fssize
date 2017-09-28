/*
 * zabbix loadable module sample
 *
 * -load config
 *  same path of  module
 *  file name describe on this code.
 * -determine daemon type(server, agent or proxy )
 *
 * mutsumi Iwata
 *
 *
 */

/*
** Zabbix
** Copyright (C) 2001-2014 Zabbix SIA
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**/

#include "common.h"

#include "sysinc.h"
#include "module.h"
#include "log.h"
#include "cfg.h"
#include "zbxjson.h"

#define MODULE_NAME "dummy_load-conf.so"

extern unsigned char program_type ;
extern char *CONFIG_LOAD_MODULE_PATH ;

/* the variable keeps timeout setting for item processing */
static int	item_timeout = 0;

int	zbx_module_fssize(AGENT_REQUEST *request, AGENT_RESULT *result);

static ZBX_METRIC keys[] =
/*      KEY                     FLAG		FUNCTION        	TEST PARAMETERS */
{
	{"vfs.fs.size.master",		CF_HAVEPARAMS,	zbx_module_fssize,		"/"},
	{NULL}
};

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_api_version                                           *
 *                                                                            *
 * Purpose: returns version number of the module interface                    *
 *                                                                            *
 * Return value: ZBX_MODULE_API_VERSION_ONE - the only version supported by   *
 *               Zabbix currently                                             *
 *                                                                            *
 ******************************************************************************/
int	zbx_module_api_version()
{
	return ZBX_MODULE_API_VERSION_ONE;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_item_timeout                                          *
 *                                                                            *
 * Purpose: set timeout value for processing of items                         *
 *                                                                            *
 * Parameters: timeout - timeout in seconds, 0 - no timeout set               *
 *                                                                            *
 ******************************************************************************/
void	zbx_module_item_timeout(int timeout)
{
	item_timeout = timeout;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_item_list                                             *
 *                                                                            *
 * Purpose: returns list of item keys supported by the module                 *
 *                                                                            *
 * Return value: list of item keys                                            *
 *                                                                            *
 ******************************************************************************/
ZBX_METRIC	*zbx_module_item_list()
{
	return keys;
}

int	zbx_module_fssize(AGENT_REQUEST *request, AGENT_RESULT *result)
{
	char	*fs;
	struct	zbx_json j;
	struct	statvfs s;

	if ( 1 != request->nparam){
		SET_MSG_RESULT(result, strdup("Invalid number of parameters."));
		return SYSINFO_RET_FAIL;
	}

	fs = get_rparam(request, 0);

	if (NULL == fs || '\0' == *fs){
		SET_MSG_RESULT(result, "Filesystem name cannot be empty.");
		return SYSINFO_RET_FAIL;
	}

	if( 0 != statvfs( fs, &s) ){
		SET_MSG_RESULT(result, strdup("Cannot obtain filesystem information"));
		return SYSINFO_RET_FAIL;
	}

	if( 0 == s.f_blocks ){
		SET_MSG_RESULT(result, strdup("s.f_blocks is zero"));
		return SYSINFO_RET_FAIL;
	}

	zbx_json_init(&j, ZBX_JSON_STAT_BUF_LEN);
	zbx_json_addstring(&j, "FS_NAME", fs, ZBX_JSON_TYPE_STRING);
	zbx_json_adduint64(&j, "FS_TOTAL", s.f_blocks * s.f_bsize);
	zbx_json_adduint64(&j, "FS_FREE", s.f_bfree * s.f_bsize);
	zbx_json_adduint64(&j, "FS_USED", (s.f_blocks - s.f_bfree) * s.f_bsize);
	zbx_json_addstring(&j, "FS_PFREE",
	                   zbx_dsprintf(NULL, "%lf",
	                                (double)(100*s.f_bfree)/s.f_blocks ),
	                   ZBX_JSON_TYPE_INT);
	zbx_json_addstring(&j, "FS_PUSED",
	                   zbx_dsprintf(NULL, "%lf",
	                                (double)(100*(s.f_blocks - s.f_bfree))/s.f_blocks ),
	                   ZBX_JSON_TYPE_INT);
	zbx_json_close(&j);

        SET_STR_RESULT(result, zbx_strdup(NULL, j.buffer));

	zbx_json_free(&j);

	return SYSINFO_RET_OK;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_init                                                  *
 *                                                                            *
 * Purpose: the function is called on agent startup                           *
 *          It should be used to call any initialization routines             *
 *                                                                            *
 * Return value: ZBX_MODULE_OK - success                                      *
 *               ZBX_MODULE_FAIL - module initialization failed               *
 *                                                                            *
 * Comment: the module won't be loaded in case of ZBX_MODULE_FAIL             *
 *                                                                            *
 ******************************************************************************/
int	zbx_module_init()
{
	int ret = ZBX_MODULE_FAIL;

	/* determine daemon process */
	switch (program_type){
		case ZBX_PROGRAM_TYPE_SERVER:
		case ZBX_PROGRAM_TYPE_PROXY_ACTIVE:
		case ZBX_PROGRAM_TYPE_PROXY_PASSIVE:
		case ZBX_PROGRAM_TYPE_PROXY:
		case ZBX_PROGRAM_TYPE_AGENTD:
			zabbix_log(LOG_LEVEL_DEBUG, "[%s] module loaded by %s process. [%d]",
			              MODULE_NAME, get_program_type_string(program_type), program_type);
			ret = ZBX_MODULE_OK;
			break;
		default:
			zabbix_log(LOG_LEVEL_DEBUG, "[%s] unknown value program_type [%d]", MODULE_NAME, program_type );
	}

	// read parameter from config file
	//zbx_module_load_config();

	return ret;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_uninit                                                *
 *                                                                            *
 * Purpose: the function is called on agent shutdown                          *
 *          It should be used to cleanup used resources if there are any      *
 *                                                                            *
 * Return value: ZBX_MODULE_OK - success                                      *
 *               ZBX_MODULE_FAIL - function failed                            *
 *                                                                            *
 ******************************************************************************/
int	zbx_module_uninit()
{
	return ZBX_MODULE_OK;
}

