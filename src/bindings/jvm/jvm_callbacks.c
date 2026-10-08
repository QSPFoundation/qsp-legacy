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

#include "../../callbacks.h"
#include "../../errors.h"
#include "../../text.h"

INLINE JNIEnv* ndkGetJniEnv()
{
	JNIEnv* ndkEnv;
	/* Callbacks should be called on the JVM threads only, */
	/* a callback isn't called on a thread not attached to the JVM */
	if ((*ndkJvm)->GetEnv(ndkJvm, (void**)&ndkEnv, JNI_VERSION_1_6) != JNI_OK) return 0;
	return ndkEnv;
}

INLINE QSP_BOOL ndkCheckException(JNIEnv *env)
{
	if (!(*env)->ExceptionCheck(env)) return QSP_FALSE;
	(*env)->ExceptionDescribe(env);
	(*env)->ExceptionClear(env);
	return QSP_TRUE;
}

void qspInitCallBacks()
{
	int i;
	qspIsInCallBack = QSP_FALSE;
	qspIsDisableCodeExec = QSP_FALSE;
	qspIsExitOnError = QSP_FALSE;
	for (i = 0; i < QSP_CALL_DUMMY; ++i)
		qspCallBacks[i] = 0;
}

void qspSetCallBack(int type, QSP_CALLBACK func)
{
	qspCallBacks[type] = func;
}

void qspCallDebug(QSP_CHAR* str)
{
	if (str == NULL) return;
	JNIEnv *javaEnv;
	if (qspCallBacks[QSP_CALL_DEBUG] && (javaEnv = ndkGetJniEnv()))
	{
		QSPCallState state;
		jstring qspText = ndkToJavaString(javaEnv, str);

		qspSaveCallState(&state, QSP_FALSE, QSP_FALSE);
		(*javaEnv)->CallVoidMethod(javaEnv, ndkApiObject, qspCallBacks[QSP_CALL_DEBUG], qspText);
		ndkCheckException(javaEnv);
		(*javaEnv)->DeleteLocalRef(javaEnv, qspText);
		qspRestoreCallState(&state);
	}
}

void qspCallSetTimer(int msecs)
{
	/* Set timer interval */
	JNIEnv *javaEnv;
	if (qspCallBacks[QSP_CALL_SETTIMER] && (javaEnv = ndkGetJniEnv()))
	{
		QSPCallState state;

		qspSaveCallState(&state, QSP_TRUE, QSP_FALSE);
		(*javaEnv)->CallVoidMethod(javaEnv, ndkApiObject, qspCallBacks[QSP_CALL_SETTIMER], msecs);
		ndkCheckException(javaEnv);
		qspRestoreCallState(&state);
	}
}

void qspCallRefreshInt(QSP_BOOL isRedraw)
{
	/* Refresh UI to show the latest state */
	JNIEnv *javaEnv;
	if (qspCallBacks[QSP_CALL_REFRESHINT] && (javaEnv = ndkGetJniEnv()))
	{
		QSPCallState state;

		qspSaveCallState(&state, QSP_TRUE, QSP_FALSE);
		(*javaEnv)->CallVoidMethod(javaEnv, ndkApiObject, qspCallBacks[QSP_CALL_REFRESHINT], isRedraw);
		ndkCheckException(javaEnv);
		qspRestoreCallState(&state);
	}
}

void qspCallSetInputStrText(QSP_CHAR* text)
{
	JNIEnv *javaEnv;
	if (qspCallBacks[QSP_CALL_SETINPUTSTRTEXT] && (javaEnv = ndkGetJniEnv()))
	{
		QSPCallState state;
		jstring qspText = ndkToJavaString(javaEnv, text);

		qspSaveCallState(&state, QSP_TRUE, QSP_FALSE);
		(*javaEnv)->CallVoidMethod(javaEnv, ndkApiObject, qspCallBacks[QSP_CALL_SETINPUTSTRTEXT], qspText);
		ndkCheckException(javaEnv);
		(*javaEnv)->DeleteLocalRef(javaEnv, qspText);
		qspRestoreCallState(&state);
	}
}

void qspCallSystem(QSP_CHAR* cmd)
{
	if (cmd == NULL) return;
	JNIEnv *javaEnv;
	if (qspCallBacks[QSP_CALL_SYSTEM] && (javaEnv = ndkGetJniEnv()))
	{
		QSPCallState state;
		jstring qspText = ndkToJavaString(javaEnv, cmd);

		qspSaveCallState(&state, QSP_FALSE, QSP_FALSE);
		(*javaEnv)->CallVoidMethod(javaEnv, ndkApiObject, qspCallBacks[QSP_CALL_SYSTEM], qspText);
		ndkCheckException(javaEnv);
		(*javaEnv)->DeleteLocalRef(javaEnv, qspText);
		qspRestoreCallState(&state);
	}
}

void qspCallOpenQuest(QSP_CHAR* fileName, QSP_BOOL isAddLocs)
{
	if (fileName == NULL) return;
	JNIEnv *javaEnv;
	if (qspCallBacks[QSP_CALL_OPENGAME] && (javaEnv = ndkGetJniEnv())) {
		QSPCallState state;
		jstring jniFile = ndkToJavaString(javaEnv, fileName);

		qspSaveCallState(&state, QSP_FALSE, QSP_FALSE);
		(*javaEnv)->CallVoidMethod(javaEnv, ndkApiObject, qspCallBacks[QSP_CALL_OPENGAME], jniFile, isAddLocs);
		ndkCheckException(javaEnv);
		(*javaEnv)->DeleteLocalRef(javaEnv, jniFile);
		qspRestoreCallState(&state);
	}
}

void qspCallOpenGame(QSP_CHAR* file)
{
	JNIEnv *javaEnv;
	if (qspCallBacks[QSP_CALL_OPENGAMESTATUS] && (javaEnv = ndkGetJniEnv())) {
		QSPCallState state;
		jstring qspText = ndkToJavaString(javaEnv, file);

		qspSaveCallState(&state, QSP_FALSE, QSP_TRUE);
		(*javaEnv)->CallVoidMethod(javaEnv, ndkApiObject, qspCallBacks[QSP_CALL_OPENGAMESTATUS], qspText);
		ndkCheckException(javaEnv);
		(*javaEnv)->DeleteLocalRef(javaEnv, qspText);
		qspRestoreCallState(&state);
	}
}

void qspCallSaveGame(QSP_CHAR* file)
{
	JNIEnv *javaEnv;
	if (qspCallBacks[QSP_CALL_SAVEGAMESTATUS] && (javaEnv = ndkGetJniEnv())) {
		QSPCallState state;
		jstring qspText = ndkToJavaString(javaEnv, file);

		qspSaveCallState(&state, QSP_FALSE, QSP_TRUE);
		(*javaEnv)->CallVoidMethod(javaEnv, ndkApiObject, qspCallBacks[QSP_CALL_SAVEGAMESTATUS], qspText);
		ndkCheckException(javaEnv);
		(*javaEnv)->DeleteLocalRef(javaEnv, qspText);
		qspRestoreCallState(&state);
	}
}

void qspCallShowMessage(QSP_CHAR* text)
{
	JNIEnv *javaEnv;
	if (qspCallBacks[QSP_CALL_SHOWMSGSTR] && (javaEnv = ndkGetJniEnv())) {
		QSPCallState state;
		jstring qspText = ndkToJavaString(javaEnv, text);

		qspSaveCallState(&state, QSP_TRUE, QSP_FALSE);
		(*javaEnv)->CallVoidMethod(javaEnv, ndkApiObject, qspCallBacks[QSP_CALL_SHOWMSGSTR], qspText);
		ndkCheckException(javaEnv);
		(*javaEnv)->DeleteLocalRef(javaEnv, qspText);
		qspRestoreCallState(&state);
	}
}

int qspCallShowMenu(QSPListItem *items, int count)
{
	JNIEnv *javaEnv;
	if (qspCallBacks[QSP_CALL_SHOWMENU] && ndkListItemClass && (javaEnv = ndkGetJniEnv())) {
		QSPCallState state;
		int i, index;
		JNIListItem jniItem;
		jobjectArray jniMenuArray;

		qspSaveCallState(&state, QSP_FALSE, QSP_TRUE);

		/* Allocate an array, the array keeps the items so their local references are released at once */
		jniMenuArray = (*javaEnv)->NewObjectArray(javaEnv, count, ndkListItemClass, 0);
		if (ndkCheckException(javaEnv))
		{
			qspRestoreCallState(&state);
			return -1;
		}
		for (i = 0; i < count; ++i)
		{
			jniItem = ndkToJavaListItem(javaEnv, items[i].Name, items[i].Image);
			(*javaEnv)->SetObjectArrayElement(javaEnv, jniMenuArray, i, jniItem.ListItem);
			ndkReleaseJavaListItem(javaEnv, &jniItem);
		}

		/* Process user input */
		index = (*javaEnv)->CallIntMethod(javaEnv, ndkApiObject, qspCallBacks[QSP_CALL_SHOWMENU], jniMenuArray);
		if (ndkCheckException(javaEnv)) index = -1;

		/* Deallocate the resources */
		(*javaEnv)->DeleteLocalRef(javaEnv, jniMenuArray);

		qspRestoreCallState(&state);

		return index;
	}
	return -1;
}

void qspCallShowPicture(QSP_CHAR* file)
{
	JNIEnv *javaEnv;
	if (qspCallBacks[QSP_CALL_SHOWIMAGE] && (javaEnv = ndkGetJniEnv())) {
		QSPCallState state;
		jstring qspText = ndkToJavaString(javaEnv, file);

		qspSaveCallState(&state, QSP_TRUE, QSP_FALSE);
		(*javaEnv)->CallVoidMethod(javaEnv, ndkApiObject, qspCallBacks[QSP_CALL_SHOWIMAGE], qspText);
		ndkCheckException(javaEnv);
		(*javaEnv)->DeleteLocalRef(javaEnv, qspText);
		qspRestoreCallState(&state);
	}
}

void qspCallShowWindow(int type, QSP_BOOL isShow)
{
	JNIEnv *javaEnv;
	if (qspCallBacks[QSP_CALL_SHOWWINDOW] && (javaEnv = ndkGetJniEnv())) {
		QSPCallState state;

		qspSaveCallState(&state, QSP_TRUE, QSP_FALSE);
		(*javaEnv)->CallVoidMethod(javaEnv, ndkApiObject, qspCallBacks[QSP_CALL_SHOWWINDOW], type, isShow);
		ndkCheckException(javaEnv);
		qspRestoreCallState(&state);
	}
}

void qspCallPlayFile(QSP_CHAR* file, int volume)
{
	if (file == NULL) return;
	JNIEnv *javaEnv;
	if (qspCallBacks[QSP_CALL_PLAYFILE] && (javaEnv = ndkGetJniEnv())) {
		QSPCallState state;
		jstring qspText = ndkToJavaString(javaEnv, file);

		qspSaveCallState(&state, QSP_TRUE, QSP_FALSE);
		(*javaEnv)->CallVoidMethod(javaEnv, ndkApiObject, qspCallBacks[QSP_CALL_PLAYFILE], qspText, volume);
		ndkCheckException(javaEnv);
		(*javaEnv)->DeleteLocalRef(javaEnv, qspText);
		qspRestoreCallState(&state);
	}
}

QSP_BOOL qspCallIsPlayingFile(QSP_CHAR* file)
{
	if (file == NULL) return JNI_FALSE;
	JNIEnv *javaEnv;
	if (qspCallBacks[QSP_CALL_ISPLAYINGFILE] && (javaEnv = ndkGetJniEnv())) {
		QSPCallState state;
		QSP_BOOL isPlaying;
		jstring qspText = ndkToJavaString(javaEnv, file);

		qspSaveCallState(&state, QSP_TRUE, QSP_FALSE);
		isPlaying = (*javaEnv)->CallBooleanMethod(javaEnv, ndkApiObject, qspCallBacks[QSP_CALL_ISPLAYINGFILE], qspText);
		if (ndkCheckException(javaEnv)) isPlaying = QSP_FALSE;
		(*javaEnv)->DeleteLocalRef(javaEnv, qspText);
		qspRestoreCallState(&state);

		return isPlaying;
	}
	return JNI_FALSE;
}

void qspCallSleep(int msecs)
{
	JNIEnv *javaEnv;
	if (qspCallBacks[QSP_CALL_SLEEP] && (javaEnv = ndkGetJniEnv())) {
		QSPCallState state;

		qspSaveCallState(&state, QSP_TRUE, QSP_FALSE);
		(*javaEnv)->CallVoidMethod(javaEnv, ndkApiObject, qspCallBacks[QSP_CALL_SLEEP], msecs);
		ndkCheckException(javaEnv);
		qspRestoreCallState(&state);
	}
}

int qspCallGetMSCount(void)
{
	JNIEnv *javaEnv;
	if (qspCallBacks[QSP_CALL_GETMSCOUNT] && (javaEnv = ndkGetJniEnv())) {
		QSPCallState state;
		int count;

		qspSaveCallState(&state, QSP_TRUE, QSP_FALSE);
		count = (*javaEnv)->CallIntMethod(javaEnv, ndkApiObject, qspCallBacks[QSP_CALL_GETMSCOUNT]);
		if (ndkCheckException(javaEnv)) count = 0;
		qspRestoreCallState(&state);
		return count;
	}
	return 0;
}

void qspCallCloseFile(QSP_CHAR* file)
{
	JNIEnv *javaEnv;
	if (qspCallBacks[QSP_CALL_CLOSEFILE] && (javaEnv = ndkGetJniEnv()))
	{
		QSPCallState state;
		jstring qspText = ndkToJavaString(javaEnv, file);

		qspSaveCallState(&state, QSP_TRUE, QSP_FALSE);
		(*javaEnv)->CallVoidMethod(javaEnv, ndkApiObject, qspCallBacks[QSP_CALL_CLOSEFILE], qspText);
		ndkCheckException(javaEnv);
		(*javaEnv)->DeleteLocalRef(javaEnv, qspText);
		qspRestoreCallState(&state);
	}
}

QSP_CHAR* qspCallInputBox(QSP_CHAR* text)
{
	JNIEnv *javaEnv;
	if (qspCallBacks[QSP_CALL_INPUTBOX] && (javaEnv = ndkGetJniEnv())) {
		QSPCallState state;
		QSP_CHAR* buffer;
		jstring qspText = ndkToJavaString(javaEnv, text);

		qspSaveCallState(&state, QSP_TRUE, QSP_FALSE);
		jstring jResult = (*javaEnv)->CallObjectMethod(javaEnv, ndkApiObject, qspCallBacks[QSP_CALL_INPUTBOX], qspText);
		if (ndkCheckException(javaEnv)) jResult = NULL;
		if (jResult != NULL)
		{
			buffer = ndkFromJavaString(javaEnv, jResult);
			(*javaEnv)->DeleteLocalRef(javaEnv, jResult);
		}
		else
			buffer = qspGetNewText(QSP_FMT(""), 0);
		(*javaEnv)->DeleteLocalRef(javaEnv, qspText);
		qspRestoreCallState(&state);
		return buffer;
	}
	return qspGetNewText(QSP_FMT(""), 0);
}

#endif
