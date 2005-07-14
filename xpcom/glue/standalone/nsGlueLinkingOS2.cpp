/* -*- Mode: C++; tab-width: 6; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla XPCOM.
 *
 * The Initial Developer of the Original Code is
 * Benjamin Smedberg <benjamin@smedbergs.us>
 *
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsGlueLinking.h"

#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>
#include <stdlib.h>

struct DependentLib
{
    HMODULE     libHandle;
    DependentLib *next;
};

static DependentLib *sTop;

static void
AppendDependentLib(HMODULE libHandle)
{
    DependentLib *d = new DependentLib;
    if (!d)
        return;

    d->next = sTop;
    d->libHandle = libHandle;

    sTop = d;
}

static void
ReadDependentCB(const char *aDependentLib)
{
    CHAR pszError[_MAX_PATH];
    ULONG ulRc = NO_ERROR;
    HMODULE h;

    ulRc = DosLoadModule(pszError, _MAX_PATH, aDependentLib, &h);

    if (ulRc != NO_ERROR)
        return;

    AppendDependentLib(h);
}

// like strpbrk but finds the *last* char, not the first
static char*
ns_strrpbrk(char *string, const char *strCharSet)
{
    char *found = NULL;
    for (; *string; ++string) {
        for(const char *search = strCharSet; *search; ++search) {
            if (*search == *string) {
                found = string;
            }
        }
    }

    return found;
}

GetFrozenFunctionsFunc
XPCOMGlueLoad(const char *xpcomFile)
{
    if (xpcomFile[0] == '.' && xpcomFile[1] == '\0') {
        xpcomFile = XPCOM_DLL;
    }
    else {
        char xpcomDir[MAXPATHLEN];

        _fullpath(xpcomDir, xpcomFile, sizeof(xpcomDir));
        char *lastSlash = ns_strrpbrk(xpcomDir, "/\\");
        if (lastSlash) {
            *lastSlash = '\0';

            XPCOMGlueLoadDependentLibs(xpcomDir, ReadDependentCB);
        }
    }

    CHAR pszError[_MAX_PATH];
    ULONG ulRc = NO_ERROR;
    HMODULE h;

    ulRc = DosLoadModule(pszError, _MAX_PATH, xpcomFile, &h);

    if (ulRc != NO_ERROR)
        return nsnull;

    AppendDependentLib(h);

    ULONG ulrc = NO_ERROR;

    GetFrozenFunctionsFunc sym;

    ulrc = DosQueryProcAddr(h, 0, "NS_GetFrozenFunctions", &sym);

    if (ulrc != NO_ERROR)
        XPCOMGlueUnload();

    return sym;
}

void
XPCOMGlueUnload()
{
    while (sTop) {
        DosFreeModule(sTop->libHandle);

        DependentLib *temp = sTop;
        sTop = sTop->next;

        delete temp;
    }
}
