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

#ifdef _JVM_BINDING

#include "../../actions.h"
#include "../../callbacks.h"
#include "../../common.h"
#include "../../errors.h"
#include "../../game.h"
#include "../../locations.h"
#include "../../menu.h"
#include "../../objects.h"
#include "../../statements.h"
#include "../../text.h"
#include "../../variables.h"

JavaVM *ndkJvm;
jclass ndkApiClass;
jobject ndkApiObject;

jclass ndkListItemClass;
jclass ndkExecutionStateClass;
jclass ndkErrorInfoClass;
jclass ndkVarValResp;

jfieldID fieldImageID = NULL;
jfieldID fieldTextID = NULL;

void initFieldIDs(JNIEnv *env) {
	if (fieldImageID == NULL) {
		fieldImageID = (*env)->GetFieldID(env, ndkListItemClass, "image", "Ljava/lang/String;");
	}
	if (fieldTextID == NULL) {
		fieldTextID = (*env)->GetFieldID(env, ndkListItemClass, "text", "Ljava/lang/String;");
	}
}

JNIListItem ndkToJavaListItem(JNIEnv *env, QSP_CHAR *image, QSP_CHAR *text)
{
	JNIListItem res;
	initFieldIDs(env);

	jobject jniListItem = (*env)->AllocObject(env, ndkListItemClass);

	res.ListItem = jniListItem;
	res.Image = ndkToJavaString(env, image);
	res.Name = ndkToJavaString(env, text);

	(*env)->SetObjectField(env, jniListItem, fieldImageID, res.Image);
	(*env)->SetObjectField(env, jniListItem, fieldTextID, res.Name);

	return res;
}

void ndkReleaseJavaListItem(JNIEnv *env, JNIListItem *listItem)
{
	if (listItem->ListItem) (*env)->DeleteLocalRef(env, listItem->ListItem);
	if (listItem->Image) (*env)->DeleteLocalRef(env, listItem->Image);
	if (listItem->Name) (*env)->DeleteLocalRef(env, listItem->Name);
}

/* ------------------------------------------------------------ */
/* Debugging */

/* Managing the debugging mode */
JNIEXPORT void JNICALL Java_com_libsdhqs_jni_QSLibSDH_enableDebugMode(JNIEnv *env, jobject this, jboolean isDebug)
{
	qspIsDebug = isDebug;
}

/* Getting current state data */
JNIEXPORT jobject JNICALL Java_com_libsdhqs_jni_QSLibSDH_getCurStateData(JNIEnv *env, jobject this)
{
	jobject jniExecutionState = (*env)->AllocObject(env, ndkExecutionStateClass);

	QSP_CHAR *locName = ((qspRealCurLoc >= 0 && qspRealCurLoc < qspLocsCount) ? qspLocs[qspRealCurLoc].Name : 0);

	jfieldID fieldId = (*env)->GetFieldID(env, ndkExecutionStateClass, "loc", "Ljava/lang/String;");
	(*env)->SetObjectField(env, jniExecutionState, fieldId, ndkToJavaString(env, locName));

	fieldId = (*env)->GetFieldID(env, ndkExecutionStateClass , "actIndex", "I");
	(*env)->SetIntField(env, jniExecutionState, fieldId, qspRealActIndex);

	fieldId = (*env)->GetFieldID(env, ndkExecutionStateClass , "lineNum", "I");
	(*env)->SetIntField(env, jniExecutionState, fieldId, qspRealLine);

	return jniExecutionState;
}
/* ------------------------------------------------------------ */
/* Version Information */

/* Version */
JNIEXPORT jstring JNICALL Java_com_libsdhqs_jni_QSLibSDH_getVersion(JNIEnv *env, jobject this)
{
	return ndkToJavaString(env, QSP_VER);
}

/* Date and time of compilation */
JNIEXPORT jstring JNICALL Java_com_libsdhqs_jni_QSLibSDH_getCompiledDateTime(JNIEnv *env, jobject this)
{
	return ndkToJavaString(env, QSP_FMT(__DATE__) QSP_FMT(", ") QSP_FMT(__TIME__));
}
/* ------------------------------------------------------------ */
/* Number of full location updates */
JNIEXPORT jint JNICALL Java_com_libsdhqs_jni_QSLibSDH_getFullRefreshCount(JNIEnv *env, jobject this)
{
	return qspFullRefreshCount;
}
/* ------------------------------------------------------------ */
/* Full path to the downloaded game file */
JNIEXPORT jstring JNICALL Java_com_libsdhqs_jni_QSLibSDH_getQstFullPath(JNIEnv *env, jobject this)
{
	return ndkToJavaString(env, qspQstFullPath);
}
/* ------------------------------------------------------------ */
/* Name of the current location */
JNIEXPORT jstring JNICALL Java_com_libsdhqs_jni_QSLibSDH_getCurLoc(JNIEnv *env, jobject this)
{
	return ndkToJavaString(env, qspCurLoc >= 0 ? qspLocs[qspCurLoc].Name : 0);
}
/* ------------------------------------------------------------ */
/* Basic description of the location */

/* Text of the main location description window */
JNIEXPORT jstring JNICALL Java_com_libsdhqs_jni_QSLibSDH_getMainDesc(JNIEnv *env, jobject this)
{
	return ndkToJavaString(env, qspCurDesc);
}

/* The ability to change the text of the main description */
JNIEXPORT jboolean JNICALL Java_com_libsdhqs_jni_QSLibSDH_isMainDescChanged(JNIEnv *env, jobject this)
{
	return qspIsMainDescChanged;
}
/* ------------------------------------------------------------ */
/* Additional description of the location */

/* Text of the additional location description window */
JNIEXPORT jstring JNICALL Java_com_libsdhqs_jni_QSLibSDH_getVarsDesc(JNIEnv *env, jobject this)
{
	return ndkToJavaString(env, qspCurVars);
}

/* Possibility to change the text of the additional description */
JNIEXPORT jboolean JNICALL Java_com_libsdhqs_jni_QSLibSDH_isVarsDescChanged(JNIEnv *env, jobject this)
{
	return qspIsVarsDescChanged;
}

/* ------------------------------------------------------------ */
/* Text of the input line */
JNIEXPORT void JNICALL Java_com_libsdhqs_jni_QSLibSDH_setInputStrText(JNIEnv *env, jobject this, jstring val)
{
	QSP_CHAR *strConverted = ndkFromJavaString(env, val);
	qspCurInputLen = qspAddText(&qspCurInput, strConverted, 0, -1, QSP_FALSE);
	free(strConverted);
}
/* ------------------------------------------------------------ */
/* List of actions */

/* Number of actions */
JNIEXPORT jint JNICALL Java_com_libsdhqs_jni_QSLibSDH_getActionsCount(JNIEnv *env, jobject this)
{
	return qspCurActionsCount;
}

/* Data actions with the specified index */
JNIEXPORT jobjectArray JNICALL Java_com_libsdhqs_jni_QSLibSDH_getActions(JNIEnv *env, jobject this)
{
	int i;
	JNIListItem item;

	if (qspCurActionsCount < 0) return NULL;

	jobjectArray res = (*env)->NewObjectArray(env, qspCurActionsCount, ndkListItemClass, NULL);

	for (i = 0; i < qspCurActionsCount; ++i)
	{
		item = ndkToJavaListItem(env, qspCurActions[i].Image, qspCurActions[i].Desc);
		(*env)->SetObjectArrayElement(env, res, i, item.ListItem);
		ndkReleaseJavaListItem(env, &item);
	}

	return res;
}

/* Executing the code of the selected action */
JNIEXPORT jboolean JNICALL Java_com_libsdhqs_jni_QSLibSDH_executeSelActionCode(JNIEnv *env, jobject this, jboolean isRefresh)
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
JNIEXPORT jboolean JNICALL Java_com_libsdhqs_jni_QSLibSDH_setSelActionIndex(JNIEnv *env, jobject this, jint ind, jboolean isRefresh)
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
JNIEXPORT jint JNICALL Java_com_libsdhqs_jni_QSLibSDH_getSelActionIndex(JNIEnv *env, jobject this)
{
	return qspCurSelAction;
}

/* Ability to change the list of actions */
JNIEXPORT jboolean JNICALL Java_com_libsdhqs_jni_QSLibSDH_isActionsChanged(JNIEnv *env, jobject this)
{
	return qspIsActionsChanged;
}
/* ------------------------------------------------------------ */
/* List of objects */

/* Number of objects */
JNIEXPORT jint JNICALL Java_com_libsdhqs_jni_QSLibSDH_getObjectsCount(JNIEnv *env, jobject this)
{
	return qspCurObjectsCount;
}

/* Object data with the specified index */
JNIEXPORT jobjectArray JNICALL Java_com_libsdhqs_jni_QSLibSDH_getObjects(JNIEnv *env, jobject this)
{
	int i;
	JNIListItem item;

	if (qspCurObjectsCount < 0) return NULL;

	jobjectArray res = (*env)->NewObjectArray(env, qspCurObjectsCount, ndkListItemClass, NULL);

	for (i = 0; i < qspCurObjectsCount; ++i)
	{
		item = ndkToJavaListItem(env, qspCurObjects[i].Image, qspCurObjects[i].Desc);
		(*env)->SetObjectArrayElement(env, res, i, item.ListItem);
		ndkReleaseJavaListItem(env, &item);
	}

	return res;
}

/* Set the index of the selected object */
JNIEXPORT jboolean JNICALL Java_com_libsdhqs_jni_QSLibSDH_setSelObjectIndex(JNIEnv *env, jobject this, jint ind, jboolean isRefresh)
{
	if (ind >= 0 && ind < qspCurObjectsCount && ind != qspCurSelObject)
	{
		if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
		qspPrepareExecution();
		if (qspIsDisableCodeExec) return QSP_FALSE;
		qspCurSelObject = ind;
		qspExecLocByVarNameWithArgs(QSP_FMT("ONOBJSEL"), 0, 0);
		if (qspErrorNum) return QSP_FALSE;
		if ((QSP_BOOL)isRefresh) qspCallRefreshInt(QSP_FALSE);
	}
	return QSP_TRUE;
}

/* Get the index of the selected object */
JNIEXPORT jint JNICALL Java_com_libsdhqs_jni_QSLibSDH_getSelObjectIndex(JNIEnv *env, jobject this)
{
	return qspCurSelObject;
}

/* Ability to change the list of objects */
JNIEXPORT jboolean JNICALL Java_com_libsdhqs_jni_QSLibSDH_isObjectsChanged(JNIEnv *env, jobject this)
{
	return qspIsObjectsChanged;
}
/* ------------------------------------------------------------ */
/* Variables */

/* Get the values of the specified array element */
//QSP_BOOL QSPGetVarValues(const QSP_CHAR *name, int ind, int *numVal, QSP_CHAR **strVal)
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

JNIEXPORT jobject JNICALL Java_com_libsdhqs_jni_QSLibSDH_getVarValues(JNIEnv *env, jobject this, jstring name, jint ind)
{
	//Convert array name to QSP string
	QSP_CHAR *strConverted = ndkFromJavaString(env, name);

	//Call QSP function
	int numVal = 0;
	QSP_CHAR *strVal;
	QSP_BOOL result = QSPGetVarValues(strConverted, ind, &numVal, &strVal);

	// If this class does not exist then return null.
	if (ndkVarValResp == 0)
		return NULL;
	jobject obj = (*env)->AllocObject(env, ndkVarValResp);

	jfieldID successFid = (*env)->GetFieldID(env, ndkVarValResp, "isSuccess", "Z");
	if (successFid == 0)
		return NULL;
	if (result == QSP_TRUE) {
		(*env)->SetBooleanField(env, obj, successFid, JNI_TRUE);
		jstring jstringVal = ndkToJavaString(env, strVal);
		jfieldID stringValueFid = (*env)->GetFieldID(env, ndkVarValResp, "stringValue", "Ljava/lang/String;");
		if (stringValueFid == 0)
			return NULL;
		(*env)->SetObjectField(env, obj, stringValueFid, jstringVal);

		jfieldID intValueFid = (*env)->GetFieldID(env, ndkVarValResp, "intValue", "I");
		if (intValueFid == 0)
			return NULL;
		(*env)->SetIntField(env, obj, intValueFid, numVal);
	} else {
		(*env)->SetBooleanField(env, obj, successFid, JNI_FALSE);
	}

	return obj;
}

/* Get the maximum number of variables */
JNIEXPORT jint JNICALL Java_com_libsdhqs_jni_QSLibSDH_getMaxVarsCount(JNIEnv *env, jobject this)
{
	return QSP_VARSCOUNT;
}
/* ------------------------------------------------------------ */
/* Code Execution */

/* Executing a line of code */
JNIEXPORT jboolean JNICALL Java_com_libsdhqs_jni_QSLibSDH_execString(JNIEnv *env, jobject this, jstring s, jboolean isRefresh)
{
	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspPrepareExecution();
	if (qspIsDisableCodeExec) return QSP_FALSE;

	if (s == NULL) return QSP_FALSE;

	QSP_CHAR *strConverted = ndkFromJavaString(env, s);
	if (strConverted == NULL) return QSP_FALSE;

	qspExecStringAsCodeWithArgs(strConverted, 0, 0);

	free(strConverted);

	if (qspErrorNum) return QSP_FALSE;
	if (isRefresh) qspCallRefreshInt(QSP_FALSE);

	return QSP_TRUE;
}

/* Executing the code of the specified location */
JNIEXPORT jboolean JNICALL Java_com_libsdhqs_jni_QSLibSDH_execLocationCode(JNIEnv *env, jobject this, jstring name, jboolean isRefresh)
{
	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspPrepareExecution();
	if (qspIsDisableCodeExec) return QSP_FALSE;

	if (name == NULL) return JNI_FALSE;

	QSP_CHAR *strConverted = ndkFromJavaString(env, name);
	if (strConverted == NULL) return QSP_FALSE;

	qspExecLocByName(strConverted, QSP_FALSE);

	free(strConverted);

	if (qspErrorNum) return QSP_FALSE;
	if (isRefresh) qspCallRefreshInt(QSP_FALSE);

	return JNI_TRUE;
}

/* Execution of the location-counter code */
JNIEXPORT jboolean JNICALL Java_com_libsdhqs_jni_QSLibSDH_execCounter(JNIEnv *env, jobject this, jboolean isRefresh)
{
	if (!qspIsInCallBack)
	{
		qspPrepareExecution();
		qspExecLocByVarNameWithArgs(QSP_FMT("COUNTER"), 0, 0);
		if (qspErrorNum) return JNI_FALSE;
		if (isRefresh) qspCallRefreshInt(QSP_FALSE);
	}
	return JNI_TRUE;
}

/* Execution of the code of the input line handler location */
JNIEXPORT jboolean JNICALL Java_com_libsdhqs_jni_QSLibSDH_execUserInput(JNIEnv *env, jobject this, jboolean isRefresh)
{
	if (qspIsExitOnError && qspErrorNum) return JNI_FALSE;
	qspPrepareExecution();
	if (qspIsDisableCodeExec) return JNI_FALSE;
	qspExecLocByVarNameWithArgs(QSP_FMT("USERCOM"), 0, 0);
	if (qspErrorNum) return JNI_FALSE;
	if (isRefresh) qspCallRefreshInt(QSP_FALSE);
	return JNI_TRUE;
}
/* ------------------------------------------------------------ */
/* Errors */

/* Get information about the latest error */
JNIEXPORT jobject JNICALL Java_com_libsdhqs_jni_QSLibSDH_getLastErrorData(JNIEnv *env, jobject this)
{
	if (ndkErrorInfoClass == 0)
		return NULL;

	jfieldID fid = (*env)->GetFieldID(env, ndkErrorInfoClass, "locName", "Ljava/lang/String;");
	jfieldID fid2 = (*env)->GetFieldID(env, ndkErrorInfoClass, "errorNum", "I");
	jfieldID fid3 = (*env)->GetFieldID(env, ndkErrorInfoClass, "index", "I");
	jfieldID fid4 = (*env)->GetFieldID(env, ndkErrorInfoClass, "line", "I");
	if (fid == 0 || fid2 == 0 || fid3 == 0 || fid4 == 0)
		return NULL;
	jobject obj = (*env)->AllocObject(env, ndkErrorInfoClass);

	int errorNum = qspErrorNum;
	QSP_CHAR *errorLoc = (qspErrorLoc >= 0 && qspErrorLoc < qspLocsCount ? qspLocs[qspErrorLoc].Name : 0);

	(*env)->SetObjectField(env, obj, fid, ndkToJavaString(env, errorLoc));
	(*env)->SetIntField(env, obj, fid2, errorNum);
	(*env)->SetIntField(env, obj, fid3, qspErrorActIndex);
	(*env)->SetIntField(env, obj, fid4, qspErrorLine);
	return obj;
}

/* Get a description of the error by its number */
JNIEXPORT jstring JNICALL Java_com_libsdhqs_jni_QSLibSDH_getErrorDesc(JNIEnv *env, jobject this, jint errorNum)
{
	return ndkToJavaString(env, qspGetErrorDesc(errorNum));
}
/* ------------------------------------------------------------ */
/* Menu */
JNIEXPORT void JNICALL Java_com_libsdhqs_jni_QSLibSDH_selectMenuItem(JNIEnv *env, jobject this, jint ind)
{
	QSPVariant arg;
	if (ind >= 0 && ind < qspCurMenuItems)
	{
		if (qspIsDisableCodeExec) return;
		arg.IsStr = QSP_FALSE;
		QSP_NUM(arg) = ind + 1;
		qspExecLocByNameWithArgs(qspCurMenuLocs[ind], &arg, 1);
	}
}

/* ------------------------------------------------------------ */
/* Game Management */

#ifdef __ANDROID__

#include <unistd.h>

/* Working with FileDescriptor */

/* Loading a new game from FileDescriptor */
JNIEXPORT jboolean JNICALL Java_com_libsdhqs_jni_QSLibSDH_loadGameWorldFromFD(JNIEnv *env, jobject this, jint fileDescriptor, jstring fileName, jboolean isAddLocs)
{
	if (fileDescriptor < 0) return QSP_FALSE;

	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspResetError();

	if (qspIsDisableCodeExec) return QSP_FALSE;

	const int native_fd = dup(fileDescriptor);
	if (native_fd < 0) return QSP_FALSE;

    FILE *f = fdopen(native_fd, "rb");
	if (f == NULL)
	{
		close(native_fd);
		qspSetError(QSP_ERR_FILENOTFOUND);
		return QSP_FALSE;
	}

	QSP_CHAR* name = ndkFromJavaString(env, fileName);

	qspOpenQuestFromFILE(f, name, isAddLocs);

	fclose(f);
	free(name);

	if (qspErrorNum) return QSP_FALSE;

	return QSP_TRUE;
}

/* Saving state by FileDescriptor */
JNIEXPORT jboolean JNICALL Java_com_libsdhqs_jni_QSLibSDH_saveGameByFD(JNIEnv *env, jobject this, jint fileDescriptor, jboolean isRefresh)
{
	if (fileDescriptor < 0) return QSP_FALSE;

	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspPrepareExecution();

	if (qspIsDisableCodeExec) return QSP_FALSE;

	const int native_fd = dup(fileDescriptor);
	if (native_fd < 0) return QSP_FALSE;

	FILE *f = fdopen(native_fd, "wb");
	if (!f) {
		close(native_fd);
		qspSetError(QSP_ERR_FILENOTFOUND);
		return QSP_FALSE;
	}
	qspSaveGameStatusToFILE(f);
	fclose(f);

	if (qspErrorNum) return QSP_FALSE;
	if (isRefresh) qspCallRefreshInt(QSP_FALSE);

	return QSP_TRUE;
}

/* Loading state from FileDescriptor */
JNIEXPORT jboolean JNICALL Java_com_libsdhqs_jni_QSLibSDH_openSavedGameFromFD(JNIEnv *env, jobject this, jint fileDescriptor, jboolean isRefresh)
{
	if (fileDescriptor < 0) return QSP_FALSE;

	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspPrepareExecution();

	if (qspIsDisableCodeExec) return QSP_FALSE;

	const int native_fd = dup(fileDescriptor);
	if (native_fd < 0) return QSP_FALSE;

	FILE *f = fdopen(native_fd, "rb");
	if (!f) {
		close(native_fd);
		qspSetError(QSP_ERR_FILENOTFOUND);
		return QSP_FALSE;
	}
	qspOpenGameStatusFromFILE(f);
	fclose(f);

	if (qspErrorNum) return QSP_FALSE;
	if (isRefresh) qspCallRefreshInt(QSP_FALSE);

	return QSP_TRUE;
}

#else

/* Working with file */

/* Loading a new game from file */
JNIEXPORT jboolean JNICALL Java_com_libsdhqs_jni_QSLibSDH_loadGameWorldFromPath(JNIEnv *env, jobject this, jstring filePath, jstring fileName, jboolean isAddLocs)
{
	if (filePath == NULL) return QSP_FALSE;

	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspResetError();

	if (qspIsDisableCodeExec) return QSP_FALSE;

	QSP_CHAR* path = ndkFromJavaString(env, filePath);
	QSP_CHAR* name = ndkFromJavaString(env, fileName);

	FILE *f = QSP_FOPEN(path, QSP_FMT("rb"));
	if (f == NULL)
	{
		free(path);
		free(name);
		qspSetError(QSP_ERR_FILENOTFOUND);
		return QSP_FALSE;
	}

	qspOpenQuestFromFILE(f, name, isAddLocs);

	fclose(f);
	free(path);
	free(name);

	if (qspErrorNum) return QSP_FALSE;

	return QSP_TRUE;
}

/* Saving state to file */
JNIEXPORT jboolean JNICALL Java_com_libsdhqs_jni_QSLibSDH_saveGameByPath(JNIEnv *env, jobject this, jstring filePath, jboolean isRefresh)
{
	if (filePath == NULL) return QSP_FALSE;

	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspPrepareExecution();

	if (qspIsDisableCodeExec) return QSP_FALSE;

	QSP_CHAR* path = ndkFromJavaString(env, filePath);

	FILE *f = QSP_FOPEN(path, QSP_FMT("wb"));
	if (f == NULL)
	{
		free(path);
		qspSetError(QSP_ERR_FILENOTFOUND);
		return QSP_FALSE;
	}

	qspSaveGameStatusToFILE(f);

	fclose(f);
	free(path);

	if (qspErrorNum) return QSP_FALSE;
	if (isRefresh) qspCallRefreshInt(QSP_FALSE);

	return QSP_TRUE;
}

/* Loading state from file */
JNIEXPORT jboolean JNICALL Java_com_libsdhqs_jni_QSLibSDH_openSavedGameFromPath(JNIEnv *env, jobject this, jstring filePath, jboolean isRefresh)
{
	if (filePath == NULL) return QSP_FALSE;

	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspPrepareExecution();

	if (qspIsDisableCodeExec) return QSP_FALSE;

	QSP_CHAR* path = ndkFromJavaString(env, filePath);

	FILE *f = QSP_FOPEN(path, QSP_FMT("rb"));
	if (f == NULL)
	{
		free(path);
		qspSetError(QSP_ERR_FILENOTFOUND);
		return QSP_FALSE;
	}

	qspOpenGameStatusFromFILE(f);

	fclose(f);
	free(path);

	if (qspErrorNum) return QSP_FALSE;
	if (isRefresh) qspCallRefreshInt(QSP_FALSE);

	return QSP_TRUE;
}

#endif

/* Working with memory */

/* Loading a new game from memory */
JNIEXPORT jboolean JNICALL Java_com_libsdhqs_jni_QSLibSDH_loadGameWorldFromData(JNIEnv *env, jobject this, jbyteArray data, jstring fileName, jboolean isAddLocs)
{
	if (data == NULL || fileName == NULL) return QSP_FALSE;

	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspResetError();

	if (qspIsDisableCodeExec) return QSP_FALSE;

	const jint dataSize = (*env)->GetArrayLength(env, data);
	if (dataSize <= 0) return QSP_FALSE;

	char *ptr = malloc(dataSize + 3);
	if (ptr == NULL) return QSP_FALSE;

	(*env)->GetByteArrayRegion(env, data, 0, dataSize, (jbyte *)ptr);

	ptr[dataSize] = ptr[dataSize + 1] = ptr[dataSize + 2] = 0;

	QSP_CHAR* name = ndkFromJavaString(env, fileName);

	qspOpenQuestFromData(ptr, dataSize + 3, name, isAddLocs);

	free(name);
	free(ptr);

	if (qspErrorNum) return QSP_FALSE;

	return QSP_TRUE;
}

/* Saving state to memory */
JNIEXPORT jbyteArray JNICALL Java_com_libsdhqs_jni_QSLibSDH_saveGameAsData(JNIEnv *env, jobject this, jboolean isRefresh)
{
	if (qspIsExitOnError && qspErrorNum) return NULL;
	qspPrepareExecution();

	if (qspIsDisableCodeExec) return NULL;

	QSP_CHAR *data = NULL;
	int len = qspSaveGameStatusToString(&data);
	if (len == 0) return NULL;

	if (isRefresh) qspCallRefreshInt(QSP_FALSE);

	int sizeInBytes = len * sizeof(QSP_CHAR);

	jbyteArray result = (*env)->NewByteArray(env, sizeInBytes);

	if (result != NULL && data != NULL) {
		(*env)->SetByteArrayRegion(env, result, 0, sizeInBytes, (const jbyte *)data);
	}

	if (data != NULL) free(data);

	return result;
}

/* Loading state from memory */
JNIEXPORT jboolean JNICALL Java_com_libsdhqs_jni_QSLibSDH_openSavedGameFromData(JNIEnv *env, jobject this, jbyteArray data, jboolean isRefresh)
{
	if (data == NULL) return QSP_FALSE;

	if (qspIsExitOnError && qspErrorNum) return QSP_FALSE;
	qspPrepareExecution();

	if (qspIsDisableCodeExec) return QSP_FALSE;

	const jint dataSize = (*env)->GetArrayLength(env, data);
	if (dataSize <= 0) return QSP_FALSE;
	if (dataSize % sizeof(QSP_CHAR) != 0) return QSP_FALSE;

	const int dataLen = dataSize / sizeof(QSP_CHAR);
	QSP_CHAR *ptr = malloc(dataLen * sizeof(QSP_CHAR));
	if (ptr == NULL) return QSP_FALSE;

	(*env)->GetByteArrayRegion(env, data, 0, dataSize, (jbyte *)ptr);

	ptr[dataLen] = 0;

	qspOpenGameStatusFromString(ptr);

	free(ptr);

	if (qspErrorNum) return QSP_FALSE;
	if (isRefresh) qspCallRefreshInt(QSP_FALSE);

	return QSP_TRUE;
}

/* Restarting the game */
JNIEXPORT jboolean JNICALL Java_com_libsdhqs_jni_QSLibSDH_restartGame(JNIEnv *env, jobject this, jboolean isRefresh)
{
	if (qspIsExitOnError && qspErrorNum) return JNI_FALSE;
	qspPrepareExecution();
	if (qspIsDisableCodeExec) return JNI_FALSE;
	qspNewGame(QSP_TRUE);
	if (qspErrorNum) return JNI_FALSE;
	if (isRefresh) qspCallRefreshInt(QSP_FALSE);
	return JNI_TRUE;
}

/* Initialization */
JNIEXPORT void JNICALL Java_com_libsdhqs_jni_QSLibSDH_init(JNIEnv *env, jobject this)
{
	qspInitRuntime();

	/* Get JVM references */
	(*env)->GetJavaVM(env, &ndkJvm);

	jclass clazz = (*env)->FindClass(env, "com/libsdhqs/jni/QSLibSDH");
	ndkApiClass = (*env)->NewGlobalRef(env, clazz);
	ndkApiObject = (*env)->NewGlobalRef(env, this);

	clazz = (*env)->FindClass(env, "com/libsdhqs/jni/QSLibSDH$ListItem");
	ndkListItemClass = (*env)->NewGlobalRef(env, clazz);

	clazz = (*env)->FindClass(env, "com/libsdhqs/jni/QSLibSDH$ExecutionState");
	ndkExecutionStateClass = (*env)->NewGlobalRef(env, clazz);

	clazz = (*env)->FindClass(env, "com/libsdhqs/jni/QSLibSDH$ErrorData");
	ndkErrorInfoClass = (*env)->NewGlobalRef(env, clazz);

	clazz = (*env)->FindClass(env, "com/libsdhqs/jni/QSLibSDH$VarValResp");
	ndkVarValResp = (*env)->NewGlobalRef(env, clazz);

	/* Get references to callbacks */
	qspSetCallBack(QSP_CALL_DEBUG, (*env)->GetMethodID(env, ndkApiClass, "onCallDebug", "(Ljava/lang/String;)V"));
	qspSetCallBack(QSP_CALL_ISPLAYINGFILE, (*env)->GetMethodID(env, ndkApiClass, "onIsPlayingFile", "(Ljava/lang/String;)Z"));
	qspSetCallBack(QSP_CALL_PLAYFILE, (*env)->GetMethodID(env, ndkApiClass, "onPlayFile", "(Ljava/lang/String;I)V"));
	qspSetCallBack(QSP_CALL_CLOSEFILE, (*env)->GetMethodID(env, ndkApiClass, "onCloseFile", "(Ljava/lang/String;)V"));
	qspSetCallBack(QSP_CALL_SHOWIMAGE, (*env)->GetMethodID(env, ndkApiClass, "onShowImage", "(Ljava/lang/String;)V"));
	qspSetCallBack(QSP_CALL_SHOWWINDOW, (*env)->GetMethodID(env, ndkApiClass, "onShowWindow", "(IZ)V"));
	qspSetCallBack(QSP_CALL_SHOWMENU, (*env)->GetMethodID(env, ndkApiClass, "onShowMenuOld", "()V"));
	qspSetCallBack(QSP_CALL_SHOWMSGSTR, (*env)->GetMethodID(env, ndkApiClass, "onShowMessage", "(Ljava/lang/String;)V"));
	qspSetCallBack(QSP_CALL_REFRESHINT, (*env)->GetMethodID(env, ndkApiClass, "onRefreshInt", "()V"));
	qspSetCallBack(QSP_CALL_SETTIMER, (*env)->GetMethodID(env, ndkApiClass, "onSetTimer", "(I)V"));
	qspSetCallBack(QSP_CALL_OPENGAME, (*env)->GetMethodID(env, ndkApiClass, "onOpenGame", "(Ljava/lang/String;Z)V"));
	qspSetCallBack(QSP_CALL_OPENGAMESTATUS, (*env)->GetMethodID(env, ndkApiClass, "onOpenGameStatus", "(Ljava/lang/String;)V"));
	qspSetCallBack(QSP_CALL_SAVEGAMESTATUS, (*env)->GetMethodID(env, ndkApiClass, "onSaveGameStatus", "(Ljava/lang/String;)V"));
	qspSetCallBack(QSP_CALL_SLEEP, (*env)->GetMethodID(env, ndkApiClass, "onSleep", "(I)V"));
	qspSetCallBack(QSP_CALL_GETMSCOUNT, (*env)->GetMethodID(env, ndkApiClass, "onGetMsCount", "()I"));
	qspSetCallBack(QSP_CALL_INPUTBOX, (*env)->GetMethodID(env, ndkApiClass, "onInputBox", "(Ljava/lang/String;)Ljava/lang/String;"));
	qspSetCallBack(QSP_CALL_ADDMENUITEM, (*env)->GetMethodID(env, ndkApiClass, "onAddMenuItem", "(Ljava/lang/String;Ljava/lang/String;)V"));
	qspSetCallBack(QSP_CALL_CHANGEQUESTPATH, (*env)->GetMethodID(env, ndkApiClass, "onChangeQuestPath", "(Ljava/lang/String;)V"));
}

/* Deinitialization */
JNIEXPORT void JNICALL Java_com_libsdhqs_jni_QSLibSDH_terminate(JNIEnv *env, jobject this)
{
	qspTerminateRuntime();

	/* Release references */
	(*env)->DeleteGlobalRef(env, ndkApiObject);
	(*env)->DeleteGlobalRef(env, ndkApiClass);
	(*env)->DeleteGlobalRef(env, ndkListItemClass);
	(*env)->DeleteGlobalRef(env, ndkExecutionStateClass);
	(*env)->DeleteGlobalRef(env, ndkErrorInfoClass);
	(*env)->DeleteGlobalRef(env, ndkVarValResp);
}

#endif
