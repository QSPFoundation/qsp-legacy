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

#include <uchar.h>

#ifndef QSP_DEFAULTDEFINES
	#define QSP_DEFAULTDEFINES

	static int qspEndiannessTestValue = 1;

	#ifdef _UNICODE
		typedef char16_t QSP_CHAR;
		#define QSP_FMT2(x) u##x
		#define QSP_FMT(x) QSP_FMT2(x)

		#define QSP_ONIG_ENC ((*(unsigned char *)&(qspEndiannessTestValue) == 1) ? \
                             (sizeof(QSP_CHAR) == 2 ? ONIG_ENCODING_UTF16_LE : ONIG_ENCODING_UTF32_LE) : \
                             (sizeof(QSP_CHAR) == 2 ? ONIG_ENCODING_UTF16_BE : ONIG_ENCODING_UTF32_BE))

		#define QSP_STRCOLL qspStrsComp
		#define QSP_CHRLWR qspToWLower
		#define QSP_CHRUPR qspToWUpper
		#define QSP_MBTOSB(a) ((a) % 256)

		#define QSP_FROM_OS_CHAR(a) qspReverseConvertUC(a, qspCP1251ToUnicodeTable)
		#define QSP_TO_OS_CHAR(a) qspDirectConvertUC(a, qspCP1251ToUnicodeTable)
		#define QSP_WCTOB
		#define QSP_BTOWC

	#else
		typedef unsigned char QSP_CHAR;
		#define QSP_FMT(x) x

		#define QSP_FOPEN fopen
		#define QSP_WCSTOMBSLEN qspStrLen
		#define QSP_WCSTOMBS qspStrNCopy
		#define QSP_MBTOSB(a) ((unsigned char)(a))
		#if defined(_WIN) || defined(_PSP)
			#define QSP_FROM_OS_CHAR
			#define QSP_TO_OS_CHAR
			#define QSP_WCTOB(a) qspReverseConvertUC(a, qspCP1251ToUnicodeTable)
			#define QSP_BTOWC(a) qspDirectConvertUC(a, qspCP1251ToUnicodeTable)
			#define QSP_CHRLWR(a) qspCP1251ToLowerTable[(unsigned char)(a)]
			#define QSP_CHRUPR(a) qspCP1251ToUpperTable[(unsigned char)(a)]
			#define QSP_STRCOLL(a, b) qspStrCmpSB(a, b, qspCP1251OrderTable)
			#define QSP_ONIG_ENC ONIG_ENCODING_CP1251
		#else
			#define QSP_FROM_OS_CHAR(a) qspReverseConvertSB(a, qspCP1251ToKOI8RTable)
			#define QSP_TO_OS_CHAR(a) qspDirectConvertSB(a, qspCP1251ToKOI8RTable)
			#define QSP_WCTOB(a) qspReverseConvertUC(a, qspKOI8RToUnicodeTable)
			#define QSP_BTOWC(a) qspDirectConvertUC(a, qspKOI8RToUnicodeTable)
			#define QSP_CHRLWR(a) qspKOI8RToLowerTable[(unsigned char)(a)]
			#define QSP_CHRUPR(a) qspKOI8RToUpperTable[(unsigned char)(a)]
			#define QSP_STRCOLL(a, b) qspStrCmpSB(a, b, qspKOI8ROrderTable)
			#define QSP_ONIG_ENC ONIG_ENCODING_KOI8_R
		#endif
	#endif

	#define QSP_FIXBYTESORDER(a) ((*(char *)&(qspEndiannessTestValue) == 1) ? \
			(a) : \
			((unsigned short)(((a) << 8) | ((a) >> 8))))

	#ifdef _MSC_VER
		#define QSP_TIME _time64
	#else
		#define QSP_TIME time
	#endif

	#ifdef __cplusplus
		typedef int (*QSP_CALLBACK)(...);
	#else
		typedef int (*QSP_CALLBACK)();
	#endif

	#include "../qsp.h"

	#ifdef __cplusplus
	extern "C"
	{
	#endif

	QSP_LEGACY_EXTERN void QSPInit();
	QSP_LEGACY_EXTERN void QSPTerminate();
	QSP_LEGACY_EXTERN QSP_BOOL QSPIsInCallBack();
	QSP_LEGACY_EXTERN void QSPSetCallBack(int type, QSP_CALLBACK func);
	QSP_LEGACY_EXTERN const QSP_CHAR *QSPGetVersion();
	QSP_LEGACY_EXTERN const QSP_CHAR *QSPGetCompiledDateTime();
	/* Debugging */
	QSP_LEGACY_EXTERN void QSPEnableDebugMode(QSP_BOOL isDebug);
	QSP_LEGACY_EXTERN void QSPGetCurStateData(QSP_CHAR **loc, int *actIndex, int *line);
	/* Main desc */
	QSP_LEGACY_EXTERN const QSP_CHAR *QSPGetMainDesc();
	QSP_LEGACY_EXTERN QSP_BOOL QSPIsMainDescChanged();
	/* Vars desc */
	QSP_LEGACY_EXTERN const QSP_CHAR *QSPGetVarsDesc();
	QSP_LEGACY_EXTERN QSP_BOOL QSPIsVarsDescChanged();
	/* Input string */
	QSP_LEGACY_EXTERN void QSPSetInputStrText(const QSP_CHAR *str);
	/* Actions */
	QSP_LEGACY_EXTERN int QSPGetActions(QSPListItem *items, int itemsBufSize);
	QSP_LEGACY_EXTERN QSP_BOOL QSPExecuteSelActionCode(QSP_BOOL isRefresh);
	QSP_LEGACY_EXTERN QSP_BOOL QSPSetSelActionIndex(int ind, QSP_BOOL isRefresh);
	QSP_LEGACY_EXTERN int QSPGetSelActionIndex();
	QSP_LEGACY_EXTERN QSP_BOOL QSPIsActionsChanged();
	/* Objects */
	QSP_LEGACY_EXTERN int QSPGetObjects(QSPListItem *items, int itemsBufSize);
	QSP_LEGACY_EXTERN QSP_BOOL QSPSetSelObjectIndex(int ind, QSP_BOOL isRefresh);
	QSP_LEGACY_EXTERN int QSPGetSelObjectIndex();
	QSP_LEGACY_EXTERN QSP_BOOL QSPIsObjectsChanged();
	/* Windows */
	QSP_LEGACY_EXTERN void QSPShowWindow(int type, QSP_BOOL isShow);
	/* Code execution */
	QSP_LEGACY_EXTERN QSP_BOOL QSPExecString(const QSP_CHAR *str, QSP_BOOL isRefresh);
	QSP_LEGACY_EXTERN QSP_BOOL QSPExecCounter(QSP_BOOL isRefresh);
	QSP_LEGACY_EXTERN QSP_BOOL QSPExecUserInput(QSP_BOOL isRefresh);
	QSP_LEGACY_EXTERN QSP_BOOL QSPExecLocationCode(const QSP_CHAR *name, QSP_BOOL isRefresh);
	/* Errors */
	QSP_LEGACY_EXTERN QSPErrorInfo QSPGetLastErrorData(void);
	QSP_LEGACY_EXTERN QSP_CHAR *QSPGetErrorDesc(int errorNum);
	/* Game */
	QSP_LEGACY_EXTERN QSP_BOOL QSPLoadGameWorldFromFile(const QSP_CHAR *file, QSP_BOOL isAddLocs);
	QSP_LEGACY_EXTERN QSP_BOOL QSPLoadGameWorldFromData(const char *data, int dataSize, const QSP_CHAR *file, QSP_BOOL isAddLocs);
	QSP_LEGACY_EXTERN QSP_BOOL QSPSaveGameAsFile(const QSP_CHAR *file, QSP_BOOL isRefresh);
	QSP_LEGACY_EXTERN QSP_BOOL QSPSaveGameAsData(QSP_CHAR *strBuf, int strBufSize, int *realSize, QSP_BOOL isRefresh);
	QSP_LEGACY_EXTERN QSP_BOOL QSPOpenSavedGameFromFile(const QSP_CHAR *file, QSP_BOOL isRefresh);
	QSP_LEGACY_EXTERN QSP_BOOL QSPOpenSavedGameFromData(const QSP_CHAR *str, QSP_BOOL isRefresh);
	QSP_LEGACY_EXTERN QSP_BOOL QSPRestartGame(QSP_BOOL isRefresh);
	/* Variables */
	QSP_LEGACY_EXTERN QSP_BOOL QSPGetVarValuesCount(const QSP_CHAR *name, int *count);
	QSP_LEGACY_EXTERN QSP_BOOL QSPGetNumVarValue(const QSP_CHAR *name, int ind, int *res);
	QSP_LEGACY_EXTERN QSP_BOOL QSPGetStrVarValue(const QSP_CHAR *name, int ind, QSP_CHAR **res);
	QSP_LEGACY_EXTERN QSP_BOOL QSPGetVarValues(const QSP_CHAR *name, int ind, int *numVal, QSP_CHAR **strVal);
	QSP_LEGACY_EXTERN int QSPGetMaxVarsCount();
	QSP_LEGACY_EXTERN QSP_BOOL QSPGetVarNameByIndex(int ind, QSP_CHAR **name);

	QSP_LEGACY_EXTERN int QSPGetFullRefreshCount();
	QSP_LEGACY_EXTERN const QSP_CHAR *QSPGetQstFullPath();
	QSP_LEGACY_EXTERN const QSP_CHAR *QSPGetCurLoc();
	QSP_LEGACY_EXTERN QSP_BOOL QSPGetExprValue(const QSP_CHAR *str, QSP_BOOL *isString, int *numVal, QSP_CHAR *strVal, int strValBufSize);

	#ifdef __cplusplus
	}
	#endif

#endif
