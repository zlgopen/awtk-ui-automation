/**
 * File:   automation_agent.h
 * Author: AWTK Develop Team
 * Brief:  automation_agent
 *
 * Copyright (c) 2020 - 2020  Guangzhou ZHIYUAN Electronics Co.,Ltd.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * License file for more details.
 *
 */

/**
 * History:
 * ================================================================
 * 2020-08-02 Li XianJing <xianjimli@hotmail.com> created
 *
 */

#ifndef TK_AUTOMATION_AGENT_H
#define TK_AUTOMATION_AGENT_H

#include "httpd/httpd.h"

BEGIN_C_DECLS

/**
 * @method automation_agent_start
 * 销毁darray对象。
 * @param {darray_t*} darray 数组对象。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t automation_agent_start(httpd_t* httpd);

END_C_DECLS

#endif /*TK_AUTOMATION_AGENT_H*/
