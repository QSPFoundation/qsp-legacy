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

#include "../../declarations.h"

#ifdef _DEFAULT_BINDING

#include "../../callbacks.h"
#include "../../common.h"
#include "../../errors.h"
#include "../../game.h"
#include "../../locations.h"
#include "../../mathops.h"
#include "../../menu.h"
#include "../../objects.h"
#include "../../statements.h"
#include "../../text.h"
#include "../../variables.h"

/* ------------------------------------------------------------ */
QSP_BOOL QSPIsInCallBack()
{
	return qspIsInCallBack;
}
/* ------------------------------------------------------------ */
/* Debug */

/* Debugging mode control */
void QSPEnableDebugMode(QSP_BOOL isDebug)
{
	qspIsDebug = isDebug;
}
/* Retrieving current status data */
void QSPGetCurStateData(QSP_CHAR **loc, int *actIndex, int *line)
{
	*loc = (qspRealCurLoc >= 0 && qspRealCurLoc < qspLocsCount ? qspLocs[qspRealCurLoc].Name : 0);
	*actIndex = qspRealActIndex;
	*line = qspRealLine;
}
/* ------------------------------------------------------------ */
/*  */

/* Get version of the qsp-legacy */
const QSP_CHAR *QSPGetVersion()
{
	return QSP_VER;
}

/* Get build datetime of the qsp-legacy */
const QSP_CHAR *QSPGetCompiledDateTime()
{
	return QSP_FMT(__DATE__) QSP_FMT(", ") QSP_FMT(__TIME__);
}

/* ------------------------------------------------------------ */
/* Number of full location updates */
int QSPGetFullRefreshCount()
{
	return qspFullRefreshCount;
}
/* ------------------------------------------------------------ */
/* Full path to the downloaded game file */
const QSP_CHAR *QSPGetQstFullPath()
{
	return qspQstFullPath;
}
/* ------------------------------------------------------------ */
/* Current location */
const QSP_CHAR *QSPGetCurLoc()
{
	return qspCurLoc >= 0 ? qspLocs[qspCurLoc].Name : 0;
}
/* ------------------------------------------------------------ */
/* Main description */

/* Get text of the main description */
const QSP_CHAR *QSPGetMainDesc()
{
	return qspCurDesc;
}

/* Option to edit the main description */
QSP_BOOL QSPIsMainDescChanged()
{
	return qspIsMainDescChanged;
}
/* ------------------------------------------------------------ */
/* Additional description */

/* Get text of the additional description */
const QSP_CHAR *QSPGetVarsDesc()
{
	return qspCurVars;
}

/* Option to edit the additional description */
QSP_BOOL QSPIsVarsDescChanged()
{
	return qspIsVarsDescChanged;
}
/* ------------------------------------------------------------ */
/* Get the value of the specified expression */
QSP_BOOL QSPGetExprValue(const QSP_CHAR *expr, QSP_BOOL *isString, int *numVal, QSP_CHAR *strVal, int strValBufSize)
{
	QSPVariant v;
	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspResetError();
	if (qspIsDisableCodeExec) return QSP_FALSE;
	v = qspExprValue((QSP_CHAR *)expr);
	if (qspErrorNum) return QSP_FALSE;
	*isString = v.IsStr;
	if (v.IsStr)
	{
		qspStrNCopy(strVal, QSP_STR(v), strValBufSize - 1);
		free(QSP_STR(v));
		strVal[strValBufSize - 1] = 0;
	}
	else
		*numVal = QSP_NUM(v);
	return QSP_TRUE;
}
/* ------------------------------------------------------------ */
/* Text of the input field */
void QSPSetInputStrText(const QSP_CHAR *val)
{
	qspCurInputLen = qspAddText(&qspCurInput, (QSP_CHAR *)val, 0, -1, QSP_FALSE);
}
/* ------------------------------------------------------------ */
/* Actions */

/* Get current actions */
int QSPGetActions(QSPListItem *items, int itemsBufSize)
{
	int i;
	for (i = 0; i < qspCurActionsCount && i < itemsBufSize; ++i)
	{
		items[i].Name = qspCurActions[i].Desc;
		items[i].Image = qspCurActions[i].Image;
	}
	return qspCurActionsCount;
}
/* Executing the code for the selected action */
QSP_BOOL QSPExecuteSelActionCode(QSP_BOOL isRefresh)
{
	if (qspCurSelAction >= 0)
	{
		if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
		qspPrepareExecution();
		if (qspIsDisableCodeExec) return QSP_FALSE;
		qspExecAction(qspCurSelAction);
		if (qspErrorNum) return QSP_FALSE;
		if (isRefresh) qspCallRefreshInt(QSP_FALSE);
	}
	return QSP_TRUE;
}
/* Set the index of the selected action */
QSP_BOOL QSPSetSelActionIndex(int ind, QSP_BOOL isRefresh)
{
	if (ind >= 0 && ind < qspCurActionsCount && ind != qspCurSelAction)
	{
		if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
		qspPrepareExecution();
		if (qspIsDisableCodeExec) return QSP_FALSE;
		qspCurSelAction = ind;
		qspExecLocByVarNameWithArgs(QSP_FMT("ONACTSEL"), 0, 0);
		if (qspErrorNum) return QSP_FALSE;
		if (isRefresh) qspCallRefreshInt(QSP_FALSE);
	}
	return QSP_TRUE;
}
/* Get the index of the selected action */
int QSPGetSelActionIndex()
{
	return qspCurSelAction;
}
/* Ability to modify the list of actions */
QSP_BOOL QSPIsActionsChanged()
{
	return qspIsActionsChanged;
}
/* ------------------------------------------------------------ */
/* Objects */

/* Get current objects */
int QSPGetObjects(QSPListItem *items, int itemsBufSize)
{
	int i;
	for (i = 0; i < qspCurObjectsCount && i < itemsBufSize; ++i)
		qspGetObjectInfoByIndex(i, items + i);
	return qspCurObjectsCount;
}
/* Set the index of the selected object */
QSP_BOOL QSPSetSelObjectIndex(int ind, QSP_BOOL isRefresh)
{
	if (ind >= 0 && ind < qspCurObjectsCount && ind != qspCurSelObject)
	{
		if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
		qspPrepareExecution();
		if (qspIsDisableCodeExec) return QSP_FALSE;
		qspCurSelObject = ind;
		qspExecLocByVarNameWithArgs(QSP_FMT("ONOBJSEL"), 0, 0);
		if (qspErrorNum) return QSP_FALSE;
		if (isRefresh) qspCallRefreshInt(QSP_FALSE);
	}
	return QSP_TRUE;
}
/* Get the index of the selected object */
int QSPGetSelObjectIndex()
{
	return qspCurSelObject;
}
/* Ability to modify the list of objects */
QSP_BOOL QSPIsObjectsChanged()
{
	return qspIsObjectsChanged;
}
/* ------------------------------------------------------------ */
/* Show / Hide Windows */
void QSPShowWindow(int type, QSP_BOOL isShow)
{
	switch (type)
	{
	case QSP_WIN_ACTS:
		qspCurIsShowActs = isShow;
		break;
	case QSP_WIN_OBJS:
		qspCurIsShowObjs = isShow;
		break;
	case QSP_WIN_VARS:
		qspCurIsShowVars = isShow;
		break;
	case QSP_WIN_INPUT:
		qspCurIsShowInput = isShow;
		break;
	}
}
/* ------------------------------------------------------------ */
/* Variables */

/* Get the number of elements in an array */
QSP_BOOL QSPGetVarValuesCount(const QSP_CHAR *name, int *count)
{
	QSPVar *var;
	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspResetError();
	var = qspVarReference((QSP_CHAR *)name, QSP_FALSE);
	if (qspErrorNum) return QSP_FALSE;
	*count = var->ValsCount;
	return QSP_TRUE;
}
/* Get numeric value of the specified array item */
QSP_BOOL QSPGetNumVarValue(const QSP_CHAR *name, int ind, int *res)
{
	QSPVar *var;
	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspResetError();
	var = qspVarReference((QSP_CHAR *)name, QSP_FALSE);
	if (qspErrorNum || ind < 0 || ind >= var->ValsCount) return QSP_FALSE;
	*res = var->Values[ind].Num;
	return QSP_TRUE;
}
/* Get string value of the specified array item */
QSP_BOOL QSPGetStrVarValue(const QSP_CHAR *name, int ind, QSP_CHAR **res)
{
	QSPVar *var;
	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspResetError();
	var = qspVarReference((QSP_CHAR *)name, QSP_FALSE);
	if (qspErrorNum || ind < 0 || ind >= var->ValsCount) return QSP_FALSE;
	*res = var->Values[ind].Str;
	return QSP_TRUE;
}
/* Get the values of the specified array element */
QSP_BOOL QSPGetVarValues(const QSP_CHAR *name, int ind, int *numVal, QSP_CHAR **strVal)
{
	QSPVar *var;
	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspResetError();
	var = qspVarReference((QSP_CHAR *)name, QSP_FALSE);
	if (qspErrorNum || ind < 0 || ind >= var->ValsCount) return QSP_FALSE;
	*numVal = var->Values[ind].Num;
	*strVal = var->Values[ind].Str;
	return QSP_TRUE;
}
/* Get the maximum number of variables */
int QSPGetMaxVarsCount()
{
	return QSP_VARSCOUNT;
}
/* Get the name of the variable with the specified index */
QSP_BOOL QSPGetVarNameByIndex(int index, QSP_CHAR **name)
{
	if (index < 0 || index >= QSP_VARSCOUNT || !qspVars[index].Name) return QSP_FALSE;
	*name = qspVars[index].Name;
	return QSP_TRUE;
}
/* ------------------------------------------------------------ */
/* Code execution */

/* Execution of a line of code */
QSP_BOOL QSPExecString(const QSP_CHAR *s, QSP_BOOL isRefresh)
{
	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspPrepareExecution();
	if (qspIsDisableCodeExec) return QSP_FALSE;
	qspExecStringAsCodeWithArgs((QSP_CHAR *)s, 0, 0);
	if (qspErrorNum) return QSP_FALSE;
	if (isRefresh) qspCallRefreshInt(QSP_FALSE);
	return QSP_TRUE;
}
/* Execution of the code at the specified location */
QSP_BOOL QSPExecLocationCode(const QSP_CHAR *name, QSP_BOOL isRefresh)
{
	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspPrepareExecution();
	if (qspIsDisableCodeExec) return QSP_FALSE;
	qspExecLocByName((QSP_CHAR *)name, QSP_FALSE);
	if (qspErrorNum) return QSP_FALSE;
	if (isRefresh) qspCallRefreshInt(QSP_FALSE);
	return QSP_TRUE;
}
/* Implementation of the location-counter code */
QSP_BOOL QSPExecCounter(QSP_BOOL isRefresh)
{
	if (!qspIsInCallBack)
	{
		qspPrepareExecution();
		qspExecLocByVarNameWithArgs(QSP_FMT("COUNTER"), 0, 0);
		if (qspErrorNum) return QSP_FALSE;
		if (isRefresh) qspCallRefreshInt(QSP_FALSE);
	}
	return QSP_TRUE;
}
/* Execution of the code for the input string handler */
QSP_BOOL QSPExecUserInput(QSP_BOOL isRefresh)
{
	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspPrepareExecution();
	if (qspIsDisableCodeExec) return QSP_FALSE;
	qspExecLocByVarNameWithArgs(QSP_FMT("USERCOM"), 0, 0);
	if (qspErrorNum) return QSP_FALSE;
	if (isRefresh) qspCallRefreshInt(QSP_FALSE);
	return QSP_TRUE;
}
/* ------------------------------------------------------------ */
/* Errors */

/* Get details of the last error */
QSPErrorInfo QSPGetLastErrorData(void)
{
	return qspLastError;
}
/* Get error description by numeric code */
QSP_CHAR *QSPGetErrorDesc(int errorNum)
{
	return qspGetErrorDesc(errorNum);
}
/* ------------------------------------------------------------ */
/* Game controls */

/* Load new game from file */
QSP_BOOL QSPLoadGameWorldFromFile(const QSP_CHAR *fileName, QSP_BOOL isAddLocs)
{
	if (fileName == NULL) return QSP_FALSE;

	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspResetError();

	if (qspIsDisableCodeExec) return QSP_FALSE;

	FILE *f = QSP_FOPEN(fileName, QSP_FMT("rb"));
	if (f == NULL)
	{
		qspSetError(QSP_ERR_FILENOTFOUND);
		return QSP_FALSE;
	}

	qspOpenQuestFromFILE(f, fileName, isAddLocs);

	fclose(f);

	if (qspErrorNum) return QSP_FALSE;

	return QSP_TRUE;
}
/* Load game from memory */
QSP_BOOL QSPLoadGameWorldFromData(const char *data, int dataSize, const QSP_CHAR *fileName, QSP_BOOL isAddLocs)
{
	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspResetError();
	if (qspIsDisableCodeExec) return QSP_FALSE;
	qspOpenQuestFromData((char *)data, dataSize, (QSP_CHAR *)fileName, isAddLocs);
	if (qspErrorNum) return QSP_FALSE;
	return QSP_TRUE;
}
/* Save game state to file */
QSP_BOOL QSPSaveGameAsFile(const QSP_CHAR *fileName, QSP_BOOL isRefresh)
{
	if (fileName == NULL) return QSP_FALSE;

	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspPrepareExecution();

	if (qspIsDisableCodeExec) return QSP_FALSE;

	FILE *f = QSP_FOPEN(fileName, QSP_FMT("wb"));
	if (f == NULL)
	{
		qspSetError(QSP_ERR_FILENOTFOUND);
		return QSP_FALSE;
	}

	qspSaveGameStatusToFILE(f);

	fclose(f);

	if (qspErrorNum) return QSP_FALSE;
	if (isRefresh) qspCallRefreshInt(QSP_FALSE);

	return QSP_TRUE;
}
/* Save game state to memory */
QSP_BOOL QSPSaveGameAsData(QSP_CHAR *strBuf, int strBufSize, int *realSize, QSP_BOOL isRefresh)
{
	int len, size;
	QSP_CHAR *data;
	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspPrepareExecution();
	if (qspIsDisableCodeExec) return QSP_FALSE;
	if (!(len = qspSaveGameStatusToString(&data)))
	{
		*realSize = 0;
		return QSP_FALSE;
	}
	size = len + 1;
	*realSize = size;
	if (size > strBufSize)
	{
		free(data);
		return QSP_FALSE;
	}
	qspStrNCopy(strBuf, data, strBufSize - 1);
	free(data);
	strBuf[strBufSize - 1] = 0;
	if (isRefresh) qspCallRefreshInt(QSP_FALSE);
	return QSP_TRUE;
}
/* Load game state from file */
QSP_BOOL QSPOpenSavedGameFromFile(const QSP_CHAR *fileName, QSP_BOOL isRefresh)
{
	if (fileName == NULL) return QSP_FALSE;

	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspPrepareExecution();

	if (qspIsDisableCodeExec) return QSP_FALSE;

	FILE *f = QSP_FOPEN(fileName, QSP_FMT("wb"));
	if (f == NULL)
	{
		qspSetError(QSP_ERR_FILENOTFOUND);
		return QSP_FALSE;
	}

	qspOpenGameStatusFromFILE(f);

	fclose(f);

	if (qspErrorNum) return QSP_FALSE;
	if (isRefresh) qspCallRefreshInt(QSP_FALSE);

	return QSP_TRUE;
}
/* Load game state from memory */
QSP_BOOL QSPOpenSavedGameFromData(const QSP_CHAR *str, QSP_BOOL isRefresh)
{
	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspPrepareExecution();
	if (qspIsDisableCodeExec) return QSP_FALSE;
	qspOpenGameStatusFromString((QSP_CHAR *)str);
	if (qspErrorNum) return QSP_FALSE;
	if (isRefresh) qspCallRefreshInt(QSP_FALSE);
	return QSP_TRUE;
}
/* Restart current game */
QSP_BOOL QSPRestartGame(QSP_BOOL isRefresh)
{
	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspPrepareExecution();
	if (qspIsDisableCodeExec) return QSP_FALSE;
	qspNewGame(QSP_TRUE);
	if (qspErrorNum) return QSP_FALSE;
	if (isRefresh) qspCallRefreshInt(QSP_FALSE);
	return QSP_TRUE;
}
/* ------------------------------------------------------------ */
/* Configure callbacks */
void QSPSetCallBack(int type, QSP_CALLBACK func)
{
	qspSetCallBack(type, func);
}
/* ------------------------------------------------------------ */
/* Initialization of the engine */
void QSPInit(void)
{
	#ifdef _DEBUG
		mwInit();
	#endif
	qspInitRuntime();
}
/* Deallocate all resources */
void QSPTerminate(void)
{
	qspTerminateRuntime();
	#ifdef _DEBUG
		mwTerm();
	#endif
}
#endif
