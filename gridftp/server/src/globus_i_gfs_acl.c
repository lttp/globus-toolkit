/*
 * Portions of this file Copyright 1999-2005 University of Chicago
 * Portions of this file Copyright 1999-2005 The University of Southern California.
 *
 * This file or a portion of this file is licensed under the
 * terms of the Globus Toolkit Public License, found at
 * http://www.globus.org/toolkit/download/license.html.
 * If you redistribute this file, with or without
 * modifications, you must include this notice in the file.
 */

#include "globus_i_gridftp_server.h"
#include "globus_i_gfs_acl.h"

static globus_list_t *                  globus_l_acl_module_list;

typedef struct globus_l_gfs_acl_request_s
{
    void *                              user_handle;
    globus_gfs_acl_module_t *           module;
} globus_l_gfs_acl_request_t;


static int
globus_l_gfs_acl_next(
    globus_i_gfs_acl_handle_t *         acl_handle,
    globus_result_t *                   out_res)
{
    int                                 rc = GLOBUS_GFS_ACL_COMPLETE;
    globus_l_gfs_acl_request_t *        acl_request;
    GlobusGFSName(globus_l_gfs_acl_next);
    GlobusGFSDebugEnter();

    *out_res = GLOBUS_SUCCESS;

    while(rc == GLOBUS_GFS_ACL_COMPLETE &&
        *out_res == GLOBUS_SUCCESS &&
        !globus_list_empty(acl_handle->current_list))
    {
        acl_request = (globus_l_gfs_acl_request_t *) globus_list_remove(
            &acl_handle->current_list, acl_handle->current_list);
        globus_assert(acl_request->module != NULL);

        switch(acl_handle->type)
        {
            case GLOBUS_L_GFS_ACL_TYPE_INIT:
                rc = acl_request->module->init_func(
                    &acl_request->user_handle,
                    &acl_handle->acl_info,
                    acl_handle,
                    out_res);
                break;

            case GLOBUS_L_GFS_ACL_TYPE_AUTHORIZE:
                rc = acl_request->module->authorize_func(
                    acl_request->user_handle,
                    acl_handle->auth_action, 
                    acl_handle->auth_object,
                    &acl_handle->acl_info,
                    acl_handle, 
                    out_res);
                break;

            default:
                globus_assert(0 && "possible memory corruption");
                break;
        }
    } 

    GlobusGFSDebugExit();
    return rc;
}

static void
globus_l_gfs_acl_kickout(
    void *                              user_arg)
{
    int                                 rc;
    globus_i_gfs_acl_handle_t *         acl_handle;
    GlobusGFSName(globus_l_gfs_acl_kickout);
    GlobusGFSDebugEnter();

    acl_handle = (globus_i_gfs_acl_handle_t *) user_arg;

    /* if done call the user callback */
    if(globus_list_empty(acl_handle->current_list))
    {
        acl_handle->cb(
            acl_handle->auth_action,
            acl_handle->user_arg,
            acl_handle->cached_res);
    }
    else
    {
        rc = globus_l_gfs_acl_next(acl_handle, &acl_handle->cached_res);
        if(rc == GLOBUS_GFS_ACL_COMPLETE)
        {
            acl_handle->cb(
                acl_handle->auth_action,
                acl_handle->user_arg,
                acl_handle->cached_res);
        }
    }
    
    GlobusGFSDebugExit();
}

int
globus_i_gfs_acl_init(
    struct globus_i_gfs_acl_handle_s *  acl_handle,
    const gss_ctx_id_t                  context,
    const char *                        subject,
    const char *                        username,
    const char *                        password,
    const char *                        ipaddr,
    globus_result_t *                   out_res,
    globus_gfs_acl_cb_t                 cb,
    void *                              user_arg)
{
    globus_l_gfs_acl_request_t *        acl_request;
    globus_list_t *                     list;
    int                                 rc;
    GlobusGFSName(globus_i_gfs_acl_init);
    GlobusGFSDebugEnter();

    memset(acl_handle, '\0', sizeof(struct globus_i_gfs_acl_handle_s));
    acl_handle->type = GLOBUS_L_GFS_ACL_TYPE_INIT;
    acl_handle->cb = cb;
    acl_handle->user_arg = user_arg;
    acl_handle->context = context;
    acl_handle->hostname = globus_i_gfs_config_string("fqdn");

    if(subject)
    {
        acl_handle->subject = globus_libc_strdup(subject);
        if(acl_handle->subject == NULL)
        {
            goto alloc_error;
        }
    }
    if(username)
    {
        acl_handle->username = globus_libc_strdup(username);
        if(acl_handle->username == NULL)
        {
            goto alloc_error;
        }
    }
    if(password)
    {
        acl_handle->password = globus_libc_strdup(password);
        if(acl_handle->password == NULL)
        {
            goto alloc_error;
        }
    }
    if(ipaddr)
    {
        acl_handle->ipaddr = globus_libc_strdup(ipaddr);
        if(acl_handle->ipaddr == NULL)
        {
            goto alloc_error;
        }
    }
    acl_handle->acl_info.context = acl_handle->context;
    acl_handle->acl_info.hostname = acl_handle->hostname;
    acl_handle->acl_info.subject = acl_handle->subject;
    acl_handle->acl_info.username = acl_handle->username;
    acl_handle->acl_info.password = acl_handle->password;
    acl_handle->acl_info.ipaddr = acl_handle->ipaddr;

    /* needed memory for each module 'cause of handle back, only on init */
    for(list = globus_l_acl_module_list;
        !globus_list_empty(list);
        list = globus_list_rest(list))
    {
        acl_request = (globus_l_gfs_acl_request_t *)
            globus_calloc(sizeof(globus_l_gfs_acl_request_t), 1);
        if(acl_request == NULL)
        {
            goto err;
        }
        acl_request->module = globus_list_first(list);
        globus_list_insert(&acl_handle->module_list, acl_request);
    }
    acl_handle->current_list = globus_list_copy(acl_handle->module_list);

    rc = globus_l_gfs_acl_next(acl_handle, out_res);

    GlobusGFSDebugExit();
    return rc;

alloc_error:
err:
    globus_i_gfs_acl_destroy(acl_handle);

    GlobusGFSDebugExitWithError();
    return -1;
}

void
globus_i_gfs_acl_destroy(
    struct globus_i_gfs_acl_handle_s *  acl_handle)
{
    globus_l_gfs_acl_request_t *        acl_request;
    GlobusGFSName(globus_i_gfs_acl_destroy);
    GlobusGFSDebugEnter();

    while(!globus_list_empty(acl_handle->module_list))
    {
        acl_request = (globus_l_gfs_acl_request_t *) globus_list_remove(
            &acl_handle->module_list, acl_handle->module_list);
        acl_request->module->destroy_func(acl_request->user_handle);
        globus_free(acl_request);
    }
    if(acl_handle->auth_action != NULL)
    {
        globus_free(acl_handle->auth_action);
    }
    if(acl_handle->auth_object)
    {
        globus_free(acl_handle->auth_object);
    }
    if(acl_handle->username)
    {
        globus_free(acl_handle->username);
    }
    if(acl_handle->password)
    {
        globus_free(acl_handle->password);
    }
    if(acl_handle->subject)
    {
        globus_free(acl_handle->subject);
    }
    if(acl_handle->ipaddr)
    {
        globus_free(acl_handle->ipaddr);
    }

    GlobusGFSDebugExit();
}

int
globus_gfs_acl_authorize(
    struct globus_i_gfs_acl_handle_s *  acl_handle,
    const char *                        action,
    const char *                        object,
    globus_result_t *                   out_res,
    globus_gfs_acl_cb_t                 cb,
    void *                              user_arg)
{
    int                                 rc;
    GlobusGFSName(globus_gfs_acl_authorize);
    GlobusGFSDebugEnter();

    acl_handle->type = GLOBUS_L_GFS_ACL_TYPE_AUTHORIZE;
    acl_handle->cb = cb;
    acl_handle->user_arg = user_arg;

    if(acl_handle->auth_action)
    {
        globus_free(acl_handle->auth_action);
    }
    acl_handle->auth_action = strdup(action);
    if(acl_handle->auth_action == NULL)
    {
        goto err;
    }
    if(acl_handle->auth_object)
    {
        globus_free(acl_handle->auth_object);
    }
    acl_handle->auth_object = strdup(object);
    if(acl_handle->auth_object == NULL)
    {
        goto err;
    }
    acl_handle->current_list = globus_list_copy(acl_handle->module_list);
    rc = globus_l_gfs_acl_next(acl_handle, out_res);

    GlobusGFSDebugExit();
    return rc;

  err:
    GlobusGFSDebugExitWithError();
    return -1;
}

void
globus_gfs_acl_authorized_finished(
    globus_i_gfs_acl_handle_t *         acl_handle,
    globus_result_t                     result)
{
    GlobusGFSName(globus_gfs_acl_authorized_finished);
    GlobusGFSDebugEnter();

    acl_handle->cached_res = result;

    globus_callback_register_oneshot(
        NULL,
        NULL,
        globus_l_gfs_acl_kickout,
        acl_handle);

    GlobusGFSDebugExit();
}

void
globus_gfs_acl_add_module(
    globus_gfs_acl_module_t *           module)
{
    GlobusGFSName(globus_gfs_acl_add_module);
    GlobusGFSDebugEnter();

    globus_list_insert(&globus_l_acl_module_list, module);

    GlobusGFSDebugExit();
}
