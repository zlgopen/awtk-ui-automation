/**
 * File:   automation_agent.h
 * Author: AWTK Develop Team
 * Brief:  automation_agent
 *
 * Copyright (c) 2020 - 2022  Guangzhou ZHIYUAN Electronics Co.,Ltd.
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
 * @class automation_agent_t
 * @annotation ["fake"]
 * 测试代理。
 */

/**
 * @method automation_agent_start
 * 启动测试代理。
 * @param {int} port 监听端口号。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t automation_agent_start(int port);

/**
 * @method automation_agent_stop
 * 停止测试代理。
 *
 * @return {ret_t} 返回RET_OK表示成功，否则表示失败。
 */
ret_t automation_agent_stop(void);

END_C_DECLS

#endif /*TK_AUTOMATION_AGENT_H*/
