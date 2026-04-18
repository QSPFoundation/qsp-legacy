/* Copyright (C) 2005-2010 Valeriy Argunov (nporep AT mail DOT ru) */
/*
* This library is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2.1 of the License, or
* (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "common.h"
#include "actions.h"
#include "errors.h"
#include "game.h"
#include "menu.h"
#include "objects.h"
#include "playlist.h"
#include "variables.h"
#include "statements.h"
#include "time.h"
#include "locations.h"
#include "mathops.h"
#include "callbacks.h"

static unsigned int qspRandX[55], qspRandY[256], qspRandZ;
static int qspRandI, qspRandJ;
QSP_BOOL qspIsDebug = QSP_FALSE;
QSP_CHAR *qspCurDesc = 0;
int qspCurDescLen = 0;
QSP_CHAR *qspCurVars = 0;
int qspCurVarsLen = 0;
QSP_CHAR *qspCurInput = 0;
int qspCurInputLen = 0;
QSP_CHAR *qspViewPath = 0;
int qspTimerInterval = 0;
QSP_BOOL qspIsMainDescChanged = QSP_FALSE;
QSP_BOOL qspIsVarsDescChanged = QSP_FALSE;
QSP_BOOL qspCurIsShowVars = QSP_TRUE;
QSP_BOOL qspCurIsShowInput = QSP_TRUE;

INLINE unsigned int qspURand();

void qspInitRuntime(void)
{
	qspIsDebug = QSP_FALSE;
	qspRefreshCount = qspFullRefreshCount = 0;
	qspQstPath = qspQstFullPath = 0;
	qspQstPathLen = 0;
	qspQstCRC = 0;
	qspRealCurLoc = -1;
	qspRealActIndex = -1;
	qspRealLine = 0;
	qspMSCount = 0;
	qspLocs = 0;
	qspLocsNames = 0;
	qspLocsCount = 0;
	qspCurLoc = -1;
	qspTimerInterval = 0;
	qspCurIsShowObjs = qspCurIsShowActs = qspCurIsShowVars = qspCurIsShowInput = QSP_TRUE;
	setlocale(LC_ALL, QSP_LOCALE);
	qspSetSeed(0);
	qspPrepareExecution();
	qspMemClear(QSP_TRUE);
	qspInitCallBacks();
	qspInitStats();
	qspInitMath();
}

void qspTerminateRuntime(void)
{
	qspMemClear(QSP_FALSE);
	qspCreateWorld(0, 0);
	if (qspQstPath) free(qspQstPath);
	if (qspQstFullPath) free(qspQstFullPath);
}

void qspPrepareExecution()
{
	qspResetError();
	qspIsMainDescChanged = qspIsVarsDescChanged = qspIsObjectsChanged = qspIsActionsChanged = QSP_FALSE;
}

void qspMemClear(QSP_BOOL isFirst)
{
	qspClearIncludes(isFirst);
	qspClearVars(isFirst);
	qspClearObjects(isFirst);
	qspClearActions(isFirst);
	qspClearPlayList(isFirst);
	if (!isFirst)
	{
		if (qspCurDesc)
		{
			free(qspCurDesc);
			if (qspCurDescLen) qspIsMainDescChanged = QSP_TRUE;
		}
		if (qspCurVars)
		{
			free(qspCurVars);
			if (qspCurVarsLen) qspIsVarsDescChanged = QSP_TRUE;
		}
		if (qspCurInput) free(qspCurInput);
		if (qspViewPath) free(qspViewPath);
	}
	qspCurDesc = 0;
	qspCurDescLen = 0;
	qspCurVars = 0;
	qspCurVarsLen = 0;
	qspCurInput = 0;
	qspCurInputLen = 0;
	qspViewPath = 0;
}

void qspSetSeed(unsigned int seed)
{
	int i;
	qspRandX[0] = 1;
	qspRandX[1] = seed;
	for (i = 2; i < 55; ++i)
		qspRandX[i] = qspRandX[i - 1] + qspRandX[i - 2];
	qspRandI = 23;
	qspRandJ = 54;
	for (i = 255; i >= 0; --i) qspURand();
	for (i = 255; i >= 0; --i) qspRandY[i] = qspURand();
	qspRandZ = qspURand();
}

INLINE unsigned int qspURand()
{
	if (--qspRandI < 0) qspRandI = 54;
	if (--qspRandJ < 0) qspRandJ = 54;
	return qspRandX[qspRandJ] += qspRandX[qspRandI];
}

int qspRand()
{
	int i = qspRandZ >> 24;
	qspRandZ = qspRandY[i];
	if (--qspRandI < 0) qspRandI = 54;
	if (--qspRandJ < 0) qspRandJ = 54;
	qspRandY[i] = qspRandX[qspRandJ] += qspRandX[qspRandI];
	return qspRandZ & QSP_RANDMASK;
}
