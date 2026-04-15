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

#ifndef QSP_JVMDEFINES
	#define QSP_JVMDEFINES

	#include <jni.h>
	static int qspEndiannessTestValue = 1;

	#ifdef _UNICODE
		typedef unsigned short QSP_CHAR;
		#define QSP_FMT2(x) u##x
		#define QSP_FMT(x) QSP_FMT2(x)

		#define QSP_STRCOLL qspStrsComp
		#define QSP_CHRLWR qspToWLower
		#define QSP_CHRUPR qspToWUpper
		#define QSP_WCSTOMBSLEN(a) (int)wcstombs(0, a, 0)
		#define QSP_WCSTOMBS wcstombs
		#define QSP_MBTOSB(a) ((a) % 256)
		#define QSP_ONIG_ENC (sizeof(QSP_CHAR) == 2 ? ONIG_ENCODING_UTF16_LE : ONIG_ENCODING_UTF32_LE)
		#define QSP_FROM_OS_CHAR(a) qspReverseConvertUC(a, qspCP1251ToUnicodeTable)
		#define QSP_TO_OS_CHAR(a) qspDirectConvertUC(a, qspCP1251ToUnicodeTable)
		#define QSP_WCTOB
		#define QSP_BTOWC

		#ifdef _MSC_VER
			#define QSP_FOPEN _wfopen
		#else
			#define QSP_FOPEN qspFileOpen
		#endif
	#else
		#error "Non-Unicode build using JVM binding is not supported"
	#endif

	#define QSP_FIXBYTESORDER(a) ((*(char *)&(qspEndiannessTestValue) == 1) ? \
			(a) : \
			((unsigned short)(((a) << 8) | ((a) >> 8))))

	#ifdef _MSC_VER
		#define QSP_TIME _time64
	#else
		#define QSP_TIME time
	#endif

	typedef jmethodID QSP_CALLBACK;

	#include "../qsp.h"
	#include "com_libsdhqs_jni_QSLibSDH.h"

	extern JavaVM *ndkJvm;
	extern jclass ndkApiClass;
	extern jobject ndkApiObject;

	extern jclass ndkListItemClass;
	extern jclass ndkExecutionStateClass;
	extern jclass ndkErrorInfoClass;
	extern jclass ndkVarValResp;

	typedef struct
	{
		jstring Image;
		jstring Name;
		jobject ListItem;
	} JNIListItem;

	char *qspW2C(QSP_CHAR *);
	QSP_CHAR *qspC2W(char *);
	jstring ndkToJavaString(JNIEnv *env, QSP_CHAR *str);
	QSP_CHAR *ndkFromJavaString(JNIEnv *env, jstring str);

	void qspOpenQuestFromFILE(FILE *, QSP_CHAR *, QSP_BOOL);
	void qspSaveGameStatusToFILE(FILE *);
	void qspOpenGameStatusFromFILE(FILE *);

#endif
