/****************************************************************************
*
*                   SciTech Nucleus Graphics Architecture
*
*               Copyright (C) 1991-1998 SciTech Software, Inc.
*                            All rights reserved.
*
*  ======================================================================
*  |REMOVAL OR MODIFICATION OF THIS HEADER IS STRICTLY PROHIBITED BY LAW|
*  |                                                                    |
*  |This copyrighted computer code contains proprietary technology      |
*  |owned by SciTech Software, Inc., located at 505 Wall Street,        |
*  |Chico, CA 95928 USA (http://www.scitechsoft.com).                   |
*  |                                                                    |
*  |The contents of this file are subject to the SciTech Nucleus        |
*  |License; you may *not* use this file or related software except in  |
*  |compliance with the License. You may obtain a copy of the License   |
*  |at http://www.scitechsoft.com/nucleus-license.txt                   |
*  |                                                                    |
*  |Software distributed under the License is distributed on an         |
*  |"AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or      |
*  |implied. See the License for the specific language governing        |
*  |rights and limitations under the License.                           |
*  |                                                                    |
*  |REMOVAL OR MODIFICATION OF THIS HEADER IS STRICTLY PROHIBITED BY LAW|
*  ======================================================================
*
* Language:     ANSI C
* Environment:  MSDOS
*
* Description:  OS specific Nucleus Graphics Architecture services for
*               the MSDOS operating system.
*
****************************************************************************/

#include "pm_help.h"
#include "pmapi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*-------------------------- Implementation -------------------------------*/

/****************************************************************************
PARAMETERS:
path    - Local path to the Nucleus driver files.

REMARKS:
This function is used by the application program to override the location
of the Nucleus driver files that are loaded. Normally the loader code
will look in the system Nucleus directories first, then in the 'drivers'
directory relative to the current working directory, and finally relative
to the MGL_ROOT environment variable.
****************************************************************************/
void NAPI GA_setLocalPath(
    const char *path)
{
    PM_setLocalBPDPath(path);
}

/****************************************************************************
RETURNS:
Pointer to the system wide PM library imports, or the internal version if none

REMARKS:
Nothing to do here for DOS. Basically since DOS has no system wide shared
library mechanism we are essentially screwed if the binary API changes.
By default for 32-bit DOS apps the local Nucleus drivers should always be
used in preference to the system wide Nucleus drivers.
****************************************************************************/
PM_imports * NAPI GA_getSystemPMImports(void)
{
    return &_PM_imports;
}

/****************************************************************************
REMARKS:
Nothing special for this OS.
****************************************************************************/
ibool NAPI GA_getSharedExports(
    GA_exports *gaExp,
    ibool shared)
{
    (void)gaExp;
    (void)shared;
    return false;
}

#if !defined(TEST_HARNESS) && !defined(VBETEST)
/****************************************************************************
REMARKS:
Nothing special for this OS
****************************************************************************/
ibool NAPI GA_queryFunctions(
    GA_devCtx *dc,
    N_uint32 id,
    void _FAR_ *funcs)
{
    return __GA_exports.GA_queryFunctions(dc,id,funcs);
}

/****************************************************************************
REMARKS:
Nothing special for this OS
****************************************************************************/
ibool NAPI REF2D_queryFunctions(
    REF2D_driver *ref2d,
    N_uint32 id,
    void _FAR_ *funcs)
{
    return __GA_exports.REF2D_queryFunctions(ref2d,id,funcs);
}
#endif

/****************************************************************************
REMARKS:
This function initialises the high precision timing functions for the DOS
Nucleus loader library.
****************************************************************************/
ibool NAPI GA_TimerInit(void)
{
    if (_GA_haveCPUID() && (_GA_getCPUIDFeatures() & CPU_HaveRDTSC) != 0)
	return true;
    return false;
}

/****************************************************************************
REMARKS:
This function reads the high resolution timer.
****************************************************************************/
void NAPI GA_TimerRead(
    GA_largeInteger *value)
{
    _GA_readTimeStamp(value);
}
