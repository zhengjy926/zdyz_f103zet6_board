/*-----------------------------------------------------------------------------
 * Name:    stderr_user.c
 * Purpose: STDERR User Template
 * Rev.:    1.0.0
 *-----------------------------------------------------------------------------*/

/*
 * Copyright (C) 2023 ARM Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "RTE_Components.h"
#include "SEGGER_RTT.h"

#if defined(RTE_CMSIS_Compiler_File_Interface)
#include <errno.h>
#include "retarget_fs.h"
#endif

#ifdef RTE_CMSIS_Compiler_STDERR
#include "retarget_stderr.h"
/**
  Put a character to the stderr

  \param[in]   ch  Character to output
  \return          The character written, or -1 on write error.
*/
int stderr_putchar (int ch)
{
    SEGGER_RTT_PutChar(0, ch);
    return (-1);
}
#endif

#ifdef RTE_CMSIS_Compiler_STDIN
#include "retarget_stdin.h"
#endif

#ifdef RTE_CMSIS_Compiler_STDOUT
#include "retarget_stdout.h"
/**
  Put a character to the stdout

  \param[in]   ch  Character to output
  \return          The character written, or -1 on write error.
*/
int stdout_putchar (int ch)
{
    SEGGER_RTT_PutChar(0, ch);
    return (-1);
}
#endif

#ifdef RTE_CMSIS_Compiler_TTY
#include "retarget_tty.h"
#endif

