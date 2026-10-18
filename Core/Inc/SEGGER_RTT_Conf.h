/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************
*                                                                    *
*        SEGGER RTT * Real Time Transfer for embedded targets        *
*                  https://github.com/SEGGERMicro/RTT                *
*                                                                    *
**********************************************************************

---------------------------END-OF-HEADER------------------------------
Purpose : User configuration file for RTT.
          For available configuration,
          refer to SEGGER_RTT_ConfDefaults.h.

----------------------------------------------------------------------
*/

#ifndef SEGGER_RTT_CONF_H
#define SEGGER_RTT_CONF_H


/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
// 制御ブロックとバッファを RAM 先頭の専用セクションに固定する（リンカスクリプトの .rtt）。
// ビルドで配置が変わると、OpenOCD が起動時に RAM に残った旧ファームの制御ブロックを
// つかんで出力が化けるため。バッファも同じセクションに入る（SEGGER_RTT_BUFFER_SECTION 既定値）
#define SEGGER_RTT_SECTION ".rtt"

#endif
/*************************** End of file ****************************/
