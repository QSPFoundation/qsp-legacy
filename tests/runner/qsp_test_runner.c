/*
 * Test runner for qsp-legacy.
 *
 * Runs scenario files (*.qspt) against the public API of the library:
 * builds a game from qsps-like text, executes commands, records the
 * callbacks of the engine as events and checks the state of the engine.
 * See tests/README.md for the description of the format.
 */

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qsp_default.h"

#define MAX_LINE 4096
#define MAX_TEXT (64 * 1024)

/* ------------------------------------------------------------ */
/* Small helpers */

static void *xmalloc(size_t size)
{
	void *ptr = malloc(size ? size : 1);
	if (!ptr)
	{
		fputs("out of memory\n", stderr);
		exit(2);
	}
	return ptr;
}

static void *xrealloc(void *ptr, size_t size)
{
	ptr = realloc(ptr, size ? size : 1);
	if (!ptr)
	{
		fputs("out of memory\n", stderr);
		exit(2);
	}
	return ptr;
}

static char *xstrdup(const char *s)
{
	size_t len = strlen(s);
	char *ret = xmalloc(len + 1);
	memcpy(ret, s, len + 1);
	return ret;
}

typedef struct
{
	char *Data;
	size_t Len;
	size_t Size;
	int Lines; /* lines added with addTextLine() */
} StrBuf;

static void sbAddN(StrBuf *sb, const char *s, size_t len)
{
	if (sb->Len + len + 1 > sb->Size)
	{
		sb->Size = (sb->Len + len + 1) * 2;
		sb->Data = xrealloc(sb->Data, sb->Size);
	}
	memcpy(sb->Data + sb->Len, s, len);
	sb->Len += len;
	sb->Data[sb->Len] = 0;
}

static void sbAdd(StrBuf *sb, const char *s)
{
	sbAddN(sb, s, strlen(s));
}

static void sbAddf(StrBuf *sb, const char *fmt, ...)
{
	char buf[MAX_LINE];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	sbAdd(sb, buf);
}

static void sbClear(StrBuf *sb)
{
	sb->Len = 0;
	sb->Lines = 0;
	if (sb->Data) sb->Data[0] = 0;
}

static const char *sbStr(StrBuf *sb)
{
	return sb->Data ? sb->Data : "";
}

/* ------------------------------------------------------------ */
/* UTF-8 <-> QSP_CHAR (UTF-16) */

static QSP_CHAR *toQsp(const char *s)
{
	size_t len = strlen(s), i = 0, n = 0;
	QSP_CHAR *ret = xmalloc((len + 1) * 2 * sizeof(QSP_CHAR));
	while (i < len)
	{
		unsigned long cp;
		unsigned char c = (unsigned char)s[i];
		int extra;
		if (c < 0x80) { cp = c; extra = 0; }
		else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; extra = 1; }
		else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; extra = 2; }
		else { cp = c & 0x07; extra = 3; }
		++i;
		while (extra-- > 0 && i < len)
			cp = (cp << 6) | ((unsigned char)s[i++] & 0x3F);
		if (cp >= 0x10000)
		{
			cp -= 0x10000;
			ret[n++] = (QSP_CHAR)(0xD800 + (cp >> 10));
			ret[n++] = (QSP_CHAR)(0xDC00 + (cp & 0x3FF));
		}
		else
			ret[n++] = (QSP_CHAR)cp;
	}
	ret[n] = 0;
	return ret;
}

static void addUtf8(StrBuf *sb, unsigned long cp)
{
	char buf[4];
	if (cp < 0x80)
	{
		buf[0] = (char)cp;
		sbAddN(sb, buf, 1);
	}
	else if (cp < 0x800)
	{
		buf[0] = (char)(0xC0 | (cp >> 6));
		buf[1] = (char)(0x80 | (cp & 0x3F));
		sbAddN(sb, buf, 2);
	}
	else if (cp < 0x10000)
	{
		buf[0] = (char)(0xE0 | (cp >> 12));
		buf[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
		buf[2] = (char)(0x80 | (cp & 0x3F));
		sbAddN(sb, buf, 3);
	}
	else
	{
		buf[0] = (char)(0xF0 | (cp >> 18));
		buf[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
		buf[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
		buf[3] = (char)(0x80 | (cp & 0x3F));
		sbAddN(sb, buf, 4);
	}
}

/* Appends a QSP string escaping \r, \n and \ so that every event fits into a line */
static void addEscaped(StrBuf *sb, const QSP_CHAR *s)
{
	if (!s) return;
	while (*s)
	{
		unsigned long cp = *s++;
		if (cp >= 0xD800 && cp < 0xDC00 && *s >= 0xDC00 && *s < 0xE000)
			cp = 0x10000 + ((cp - 0xD800) << 10) + (*s++ - 0xDC00);
		if (cp == '\r') sbAdd(sb, "\\r");
		else if (cp == '\n') sbAdd(sb, "\\n");
		else if (cp == '\\') sbAdd(sb, "\\\\");
		else addUtf8(sb, cp);
	}
}

/* Converts \r, \n, \\ escapes in expectations to the real characters */
static char *unescape(const char *s)
{
	char *ret = xmalloc(strlen(s) + 1), *dst = ret;
	while (*s)
	{
		if (*s == '\\' && s[1])
		{
			++s;
			if (*s == 'r') *dst++ = '\r';
			else if (*s == 'n') *dst++ = '\n';
			else *dst++ = *s;
			++s;
		}
		else
			*dst++ = *s++;
	}
	*dst = 0;
	return ret;
}

/* ------------------------------------------------------------ */
/* Game description and the game file writer */

typedef struct
{
	char *Name;
	char *Image;
	StrBuf Code;
} Action;

typedef struct
{
	char *Name;
	StrBuf Desc;
	StrBuf Code;
	Action *Actions;
	int ActionsCount;
} Location;

typedef struct
{
	char *Name; /* module name for OPENGAME, NULL for the main game */
	Location *Locs;
	int LocsCount;
} Game;

static Location *gameLoc(Game *game, const char *name)
{
	int i;
	Location *loc;
	for (i = 0; i < game->LocsCount; ++i)
		if (!strcmp(game->Locs[i].Name, name)) return game->Locs + i;
	game->Locs = xrealloc(game->Locs, (game->LocsCount + 1) * sizeof(Location));
	loc = game->Locs + game->LocsCount++;
	memset(loc, 0, sizeof(Location));
	loc->Name = xstrdup(name);
	return loc;
}

static void freeGame(Game *game)
{
	int i, j;
	for (i = 0; i < game->LocsCount; ++i)
	{
		Location *loc = game->Locs + i;
		free(loc->Name);
		free(loc->Desc.Data);
		free(loc->Code.Data);
		for (j = 0; j < loc->ActionsCount; ++j)
		{
			free(loc->Actions[j].Name);
			free(loc->Actions[j].Image);
			free(loc->Actions[j].Code.Data);
		}
		free(loc->Actions);
	}
	free(game->Locs);
	free(game->Name);
	memset(game, 0, sizeof(Game));
}

/* Adds a line of the text joining lines with \r\n like the editors do, empty lines count too */
static void addTextLine(StrBuf *sb, const char *line)
{
	if (sb->Lines++) sbAdd(sb, "\r\n");
	sbAdd(sb, line);
}

/* Parses locations in the qsps format: "# name", code lines, a line starting with "--" */
static void parseQsps(Game *game, char **lines, int count)
{
	int i;
	Location *loc = 0;
	for (i = 0; i < count; ++i)
	{
		const char *line = lines[i];
		if (!loc)
		{
			if (line[0] == '#')
			{
				const char *name = line + 1;
				while (*name == ' ') ++name;
				loc = gameLoc(game, name);
				sbClear(&loc->Code);
			}
		}
		else if (line[0] == '-' && line[1] == '-')
			loc = 0;
		else
			addTextLine(&loc->Code, line);
	}
}

typedef struct
{
	unsigned char *Data;
	size_t Len;
	size_t Size;
} ByteBuf;

static void bbAddUnit(ByteBuf *bb, unsigned int unit)
{
	if (bb->Len + 2 > bb->Size)
	{
		bb->Size = (bb->Len + 2) * 2;
		bb->Data = xrealloc(bb->Data, bb->Size);
	}
	bb->Data[bb->Len++] = (unsigned char)(unit & 0xFF);
	bb->Data[bb->Len++] = (unsigned char)(unit >> 8);
}

/* Writes a field of the game file in UCS-2, coded fields are shifted by 5 like the engine expects */
static void writeField(ByteBuf *bb, const char *text, int isCoded, int isFirst)
{
	QSP_CHAR *str = toQsp(text), *ptr;
	if (!isFirst)
	{
		bbAddUnit(bb, '\r');
		bbAddUnit(bb, '\n');
	}
	for (ptr = str; *ptr; ++ptr)
	{
		unsigned int ch = *ptr;
		if (isCoded) ch = (ch == 5 ? (unsigned int)(0x10000 - 5) : (ch - 5) & 0xFFFF);
		bbAddUnit(bb, ch);
	}
	free(str);
}

static void writeCodedInt(ByteBuf *bb, int value)
{
	char buf[16];
	snprintf(buf, sizeof(buf), "%d", value);
	writeField(bb, buf, 1, 0);
}

static ByteBuf buildGameFile(Game *game)
{
	int i, j;
	ByteBuf bb = {0};
	writeField(&bb, "QSPGAME", 0, 1);
	writeField(&bb, "qsp-legacy-tests", 0, 0);
	writeField(&bb, "", 1, 0);
	writeCodedInt(&bb, game->LocsCount);
	for (i = 0; i < game->LocsCount; ++i)
	{
		Location *loc = game->Locs + i;
		writeField(&bb, loc->Name, 1, 0);
		writeField(&bb, sbStr(&loc->Desc), 1, 0);
		writeField(&bb, sbStr(&loc->Code), 1, 0);
		writeCodedInt(&bb, loc->ActionsCount);
		for (j = 0; j < loc->ActionsCount; ++j)
		{
			writeField(&bb, loc->Actions[j].Image ? loc->Actions[j].Image : "", 1, 0);
			writeField(&bb, loc->Actions[j].Name, 1, 0);
			writeField(&bb, sbStr(&loc->Actions[j].Code), 1, 0);
		}
	}
	return bb;
}

/* ------------------------------------------------------------ */
/* State of the running test */

typedef struct
{
	char *Name;
	QSP_CHAR *Data;
} SaveSlot;

static struct
{
	const char *FileName;
	int Line; /* line of the current command, for messages */
	char *TestName;
	int IsSkipped;
	int IsFailed;
	int Passed, Failed, Skipped;

	Game Main;
	Game *Modules;
	int ModulesCount;
	int IsGameDirty;
	int IsGameLoaded;

	char **Events;
	int EventsCount;

	char *PendingError; /* error that hasn't been checked by the test yet */

	char **InputReplies;
	int InputRepliesCount;
	int MenuReplies[64];
	int MenuRepliesCount;
	int IsPlayReplies[64];
	int IsPlayRepliesCount;
	int MsCount;

	SaveSlot *Saves;
	int SavesCount;
} T;

static void fail(const char *fmt, ...)
{
	va_list args;
	if (T.IsFailed) return; /* report only the first problem of a test */
	T.IsFailed = 1;
	fprintf(stderr, "FAIL %s:%d [%s]\n  ", T.FileName, T.Line, T.TestName ? T.TestName : "");
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fputc('\n', stderr);
}

static void addEvent(const char *event)
{
	T.Events = xrealloc(T.Events, (T.EventsCount + 1) * sizeof(char *));
	T.Events[T.EventsCount++] = xstrdup(event);
}

static void clearEvents(void)
{
	int i;
	for (i = 0; i < T.EventsCount; ++i) free(T.Events[i]);
	free(T.Events);
	T.Events = 0;
	T.EventsCount = 0;
}

static void dumpEvents(void)
{
	int i;
	fprintf(stderr, "  events so far:\n");
	for (i = 0; i < T.EventsCount; ++i) fprintf(stderr, "    %s\n", T.Events[i]);
	if (!T.EventsCount) fprintf(stderr, "    (none)\n");
}

/* ------------------------------------------------------------ */
/* Callbacks: every call becomes an event named like in qsp-wasm-engine */

static void addListItems(StrBuf *sb, QSPListItem *items, int count)
{
	int i;
	sbAdd(sb, "[");
	for (i = 0; i < count; ++i)
	{
		if (i) sbAdd(sb, "; ");
		addEscaped(sb, items[i].Name);
		if (items[i].Image && *items[i].Image)
		{
			sbAdd(sb, "|");
			addEscaped(sb, items[i].Image);
		}
	}
	sbAdd(sb, "]");
}

static const QSP_CHAR *nonEmpty(const QSP_CHAR *text)
{
	return (text && *text) ? text : 0;
}

static void eventWithText(const char *name, const QSP_CHAR *text)
{
	StrBuf sb = {0};
	sbAdd(&sb, name);
	if (text)
	{
		sbAdd(&sb, " ");
		addEscaped(&sb, text);
	}
	addEvent(sbStr(&sb));
	free(sb.Data);
}

static void listState(StrBuf *sb, int isActions)
{
	QSPListItem items[1000];
	int count = (isActions ? QSPGetActions(items, 1000) : QSPGetObjects(items, 1000));
	addListItems(sb, items, count > 1000 ? 1000 : count);
}

/* Like onRefresh() of qsp-wasm-engine: isRedraw forces the update of every window */
static int cbRefresh(QSP_BOOL isRedraw)
{
	StrBuf sb = {0};
	/* An empty description is logged without the text, so the event has no trailing space */
	if (isRedraw || QSPIsMainDescChanged()) eventWithText("main_changed", nonEmpty(QSPGetMainDesc()));
	if (isRedraw || QSPIsVarsDescChanged()) eventWithText("stats_changed", nonEmpty(QSPGetVarsDesc()));
	if (isRedraw || QSPIsActionsChanged())
	{
		sbAdd(&sb, "actions_changed ");
		listState(&sb, 1);
		addEvent(sbStr(&sb));
		sbClear(&sb);
	}
	if (isRedraw || QSPIsObjectsChanged())
	{
		sbAdd(&sb, "objects_changed ");
		listState(&sb, 0);
		addEvent(sbStr(&sb));
	}
	free(sb.Data);
	return 0;
}

static int cbDebug(QSP_CHAR *str)
{
	/* debug LOCATION:LINE:ACTION CODE */
	StrBuf sb = {0};
	QSP_CHAR *loc = 0;
	int actIndex = 0, line = 0;
	QSPGetCurStateData(&loc, &actIndex, &line);
	sbAdd(&sb, "debug ");
	addEscaped(&sb, loc);
	sbAddf(&sb, ":%d:%d ", line, actIndex);
	addEscaped(&sb, str);
	addEvent(sbStr(&sb));
	free(sb.Data);
	return 0;
}
static int cbMsg(QSP_CHAR *str) { eventWithText("msg", str); return 0; }
static int cbView(QSP_CHAR *file) { eventWithText("view", file); return 0; }
static int cbSystem(QSP_CHAR *str) { eventWithText("system_cmd", str); return 0; }
static int cbSetInput(QSP_CHAR *text) { eventWithText("user_input", text); return 0; }
static int cbCloseFile(QSP_CHAR *file) { eventWithText("close_file", file); return 0; }

static int cbPlayFile(QSP_CHAR *file, int volume)
{
	StrBuf sb = {0};
	sbAdd(&sb, "play_file ");
	addEscaped(&sb, file);
	sbAddf(&sb, " %d", volume);
	addEvent(sbStr(&sb));
	free(sb.Data);
	return 0;
}

static int cbIsPlay(QSP_CHAR *file)
{
	int ret = 0, i;
	eventWithText("is_play", file);
	if (T.IsPlayRepliesCount)
	{
		ret = T.IsPlayReplies[0];
		for (i = 1; i < T.IsPlayRepliesCount; ++i) T.IsPlayReplies[i - 1] = T.IsPlayReplies[i];
		--T.IsPlayRepliesCount;
	}
	return ret;
}

static int cbShowWindow(int type, QSP_BOOL isShow)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "panel_visibility %d %d", type, isShow ? 1 : 0);
	addEvent(buf);
	return 0;
}

static int cbMenu(QSPListItem *items, int count)
{
	StrBuf sb = {0};
	int ret = -1, i;
	sbAdd(&sb, "menu ");
	addListItems(&sb, items, count);
	addEvent(sbStr(&sb));
	free(sb.Data);
	if (T.MenuRepliesCount)
	{
		ret = T.MenuReplies[0];
		for (i = 1; i < T.MenuRepliesCount; ++i) T.MenuReplies[i - 1] = T.MenuReplies[i];
		--T.MenuRepliesCount;
	}
	return ret;
}

static int cbInput(QSP_CHAR *text, QSP_CHAR *buffer, int maxLen)
{
	eventWithText("input", text);
	buffer[0] = 0;
	if (T.InputRepliesCount)
	{
		QSP_CHAR *reply = toQsp(T.InputReplies[0]);
		int i, len = 0;
		while (reply[len] && len < maxLen) ++len;
		memcpy(buffer, reply, len * sizeof(QSP_CHAR));
		buffer[len] = 0;
		free(reply);
		free(T.InputReplies[0]);
		for (i = 1; i < T.InputRepliesCount; ++i) T.InputReplies[i - 1] = T.InputReplies[i];
		--T.InputRepliesCount;
	}
	return 0;
}

static int cbSleep(int msecs)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "wait %d", msecs);
	addEvent(buf);
	return 0;
}

static int cbSetTimer(int msecs)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "timer %d", msecs);
	addEvent(buf);
	return 0;
}

static int cbGetMsCount(void)
{
	/* Not logged: the engine asks for the time all the time */
	return T.MsCount;
}

static char *toUtf8(const QSP_CHAR *s)
{
	StrBuf sb = {0};
	while (s && *s) addUtf8(&sb, *s++);
	if (!sb.Data) return xstrdup("");
	return sb.Data;
}

static void loadGame(Game *game, QSP_BOOL isAddLocs);
static void afterCall(QSP_BOOL isOk);

static int cbOpenGame(QSP_CHAR *file, QSP_BOOL isAddLocs)
{
	StrBuf sb = {0};
	char *name = toUtf8(file);
	int i;
	sbAdd(&sb, "open_game ");
	addEscaped(&sb, file);
	sbAddf(&sb, " %d", isAddLocs ? 1 : 0);
	addEvent(sbStr(&sb));
	free(sb.Data);
	for (i = 0; i < T.ModulesCount; ++i)
	{
		if (!strcmp(T.Modules[i].Name, name))
		{
			loadGame(T.Modules + i, isAddLocs);
			break;
		}
	}
	free(name);
	return 0;
}

static SaveSlot *findSave(const char *name, int toCreate)
{
	int i;
	for (i = 0; i < T.SavesCount; ++i)
		if (!strcmp(T.Saves[i].Name, name)) return T.Saves + i;
	if (!toCreate) return 0;
	T.Saves = xrealloc(T.Saves, (T.SavesCount + 1) * sizeof(SaveSlot));
	T.Saves[T.SavesCount].Name = xstrdup(name);
	T.Saves[T.SavesCount].Data = 0;
	return T.Saves + T.SavesCount++;
}

static void saveToSlot(const char *name)
{
	int realSize = 0;
	SaveSlot *slot = findSave(name, 1);
	QSP_CHAR *buf;
	/* The first call only reports the size */
	QSPSaveGameAsData(0, 0, &realSize, QSP_FALSE);
	if (realSize <= 0)
	{
		afterCall(QSP_FALSE);
		return;
	}
	buf = xmalloc(realSize * sizeof(QSP_CHAR));
	if (QSPSaveGameAsData(buf, realSize, &realSize, QSP_FALSE))
	{
		free(slot->Data);
		slot->Data = buf;
	}
	else
	{
		free(buf);
		afterCall(QSP_FALSE);
	}
}

static void loadFromSlot(const char *name)
{
	SaveSlot *slot = findSave(name, 0);
	if (slot && slot->Data) afterCall(QSPOpenSavedGameFromData(slot->Data, QSP_FALSE));
}

static int cbSaveGame(QSP_CHAR *file)
{
	char *name = toUtf8(file);
	eventWithText("save_game", file);
	saveToSlot(file ? name : "");
	free(name);
	return 0;
}

static int cbOpenSave(QSP_CHAR *file)
{
	char *name = toUtf8(file);
	eventWithText("load_save", file);
	loadFromSlot(file ? name : "");
	free(name);
	return 0;
}

static void setCallbacks(void)
{
	QSPSetCallBack(QSP_CALL_DEBUG, (QSP_CALLBACK)cbDebug);
	QSPSetCallBack(QSP_CALL_ISPLAYINGFILE, (QSP_CALLBACK)cbIsPlay);
	QSPSetCallBack(QSP_CALL_PLAYFILE, (QSP_CALLBACK)cbPlayFile);
	QSPSetCallBack(QSP_CALL_CLOSEFILE, (QSP_CALLBACK)cbCloseFile);
	QSPSetCallBack(QSP_CALL_SHOWIMAGE, (QSP_CALLBACK)cbView);
	QSPSetCallBack(QSP_CALL_SHOWWINDOW, (QSP_CALLBACK)cbShowWindow);
	QSPSetCallBack(QSP_CALL_SHOWMENU, (QSP_CALLBACK)cbMenu);
	QSPSetCallBack(QSP_CALL_SHOWMSGSTR, (QSP_CALLBACK)cbMsg);
	QSPSetCallBack(QSP_CALL_REFRESHINT, (QSP_CALLBACK)cbRefresh);
	QSPSetCallBack(QSP_CALL_SETTIMER, (QSP_CALLBACK)cbSetTimer);
	QSPSetCallBack(QSP_CALL_SETINPUTSTRTEXT, (QSP_CALLBACK)cbSetInput);
	QSPSetCallBack(QSP_CALL_SYSTEM, (QSP_CALLBACK)cbSystem);
	QSPSetCallBack(QSP_CALL_OPENGAME, (QSP_CALLBACK)cbOpenGame);
	QSPSetCallBack(QSP_CALL_OPENGAMESTATUS, (QSP_CALLBACK)cbOpenSave);
	QSPSetCallBack(QSP_CALL_SAVEGAMESTATUS, (QSP_CALLBACK)cbSaveGame);
	QSPSetCallBack(QSP_CALL_SLEEP, (QSP_CALLBACK)cbSleep);
	QSPSetCallBack(QSP_CALL_GETMSCOUNT, (QSP_CALLBACK)cbGetMsCount);
	QSPSetCallBack(QSP_CALL_INPUTBOX, (QSP_CALLBACK)cbInput);
}

/* ------------------------------------------------------------ */
/* Errors */

static const char *errorName(int num)
{
	static const char *names[] = {
		"DIVBYZERO", "TYPEMISMATCH", "STACKOVERFLOW", "TOOMANYITEMS", "FILENOTFOUND",
		"CANTLOADFILE", "GAMENOTLOADED", "COLONNOTFOUND", "CANTINCFILE", "CANTADDACTION",
		"EQNOTFOUND", "LOCNOTFOUND", "ENDNOTFOUND", "LABELNOTFOUND", "NOTCORRECTNAME",
		"QUOTNOTFOUND", "BRACKNOTFOUND", "BRACKSNOTFOUND", "SYNTAX", "UNKNOWNACTION",
		"ARGSCOUNT", "CANTADDOBJECT", "CANTADDMENUITEM", "TOOMANYVARS", "INCORRECTREGEXP",
		"CODENOTFOUND"
	};
	static char buf[32];
	if (num >= QSP_ERR_DIVBYZERO && num <= QSP_ERR_CODENOTFOUND) return names[num - QSP_ERR_DIVBYZERO];
	snprintf(buf, sizeof(buf), "%d", num);
	return buf;
}

static void checkNoPendingError(void)
{
	if (T.PendingError)
	{
		fail("unexpected error %s", T.PendingError);
		free(T.PendingError);
		T.PendingError = 0;
	}
}

/* Records the result of an API call that executes code */
static void afterCall(QSP_BOOL isOk)
{
	if (!isOk)
	{
		QSPErrorInfo info = QSPGetLastErrorData();
		StrBuf sb = {0};
		char *loc = info.LocName ? toUtf8(info.LocName) : xstrdup("");
		sbAddf(&sb, "error %s", errorName(info.ErrorNum));
		addEvent(sbStr(&sb));
		free(T.PendingError);
		sbClear(&sb);
		sbAddf(&sb, "%s (location '%s', line %d, action %d)", errorName(info.ErrorNum), loc, info.IntLineNum, info.ActIndex);
		T.PendingError = xstrdup(sbStr(&sb));
		free(sb.Data);
		free(loc);
	}
}

/* ------------------------------------------------------------ */
/* Loading games */

static void loadGame(Game *game, QSP_BOOL isAddLocs)
{
	ByteBuf bb = buildGameFile(game);
	if (!QSPLoadGameWorldFromData((const char *)bb.Data, (int)bb.Len, isAddLocs))
		afterCall(QSP_FALSE);
	free(bb.Data);
}

static void ensureGameLoaded(void)
{
	if (T.IsGameDirty || !T.IsGameLoaded)
	{
		if (!T.Main.LocsCount) gameLoc(&T.Main, "start");
		loadGame(&T.Main, QSP_FALSE);
		T.IsGameDirty = 0;
		T.IsGameLoaded = 1;
	}
}

/* ------------------------------------------------------------ */
/* Evaluating expressions and values */

typedef struct
{
	int IsString;
	int Num;
	char *Str;
} Value;

static int evalExpr(const char *expr, Value *val)
{
	QSP_BOOL isString = QSP_FALSE;
	int num = 0;
	QSP_CHAR *str = xmalloc(MAX_TEXT * sizeof(QSP_CHAR)), *qExpr = toQsp(expr);
	QSP_BOOL isOk;
	str[0] = 0;
	isOk = QSPGetExprValue(qExpr, &isString, &num, str, MAX_TEXT);
	free(qExpr);
	if (isOk)
	{
		StrBuf sb = {0};
		val->IsString = isString;
		val->Num = num;
		addEscaped(&sb, str);
		val->Str = sb.Data ? sb.Data : xstrdup("");
	}
	else
	{
		QSPErrorInfo info = QSPGetLastErrorData();
		val->IsString = 1;
		val->Num = 0;
		val->Str = xstrdup(errorName(info.ErrorNum));
	}
	free(str);
	return isOk;
}

/* Parses a literal: a number or a string in single quotes ('' is a quote) */
static int parseLiteral(const char *s, Value *val)
{
	while (*s == ' ') ++s;
	if (*s == '\'')
	{
		StrBuf sb = {0};
		++s;
		while (*s)
		{
			if (*s == '\'')
			{
				if (s[1] == '\'')
				{
					sbAdd(&sb, "'");
					s += 2;
					continue;
				}
				break;
			}
			sbAddN(&sb, s++, 1);
		}
		val->IsString = 1;
		val->Num = 0;
		val->Str = sb.Data ? sb.Data : xstrdup("");
		return 1;
	}
	val->IsString = 0;
	val->Num = (int)strtol(s, 0, 10);
	val->Str = 0;
	return 1;
}

/* ------------------------------------------------------------ */
/* Test lifecycle */

static void resetEngine(void)
{
	int i;
	QSPTerminate();
	QSPInit();
	setCallbacks();
	freeGame(&T.Main);
	for (i = 0; i < T.ModulesCount; ++i) freeGame(T.Modules + i);
	free(T.Modules);
	T.Modules = 0;
	T.ModulesCount = 0;
	T.IsGameDirty = 0;
	T.IsGameLoaded = 0;
	clearEvents();
	free(T.PendingError);
	T.PendingError = 0;
	for (i = 0; i < T.InputRepliesCount; ++i) free(T.InputReplies[i]);
	free(T.InputReplies);
	T.InputReplies = 0;
	T.InputRepliesCount = 0;
	T.MenuRepliesCount = 0;
	T.IsPlayRepliesCount = 0;
	T.MsCount = 0;
	for (i = 0; i < T.SavesCount; ++i)
	{
		free(T.Saves[i].Name);
		free(T.Saves[i].Data);
	}
	free(T.Saves);
	T.Saves = 0;
	T.SavesCount = 0;
}

static void finishTest(void)
{
	if (!T.TestName) return;
	if (!T.IsSkipped) checkNoPendingError();
	if (T.IsSkipped)
		++T.Skipped;
	else if (T.IsFailed)
	{
		dumpEvents();
		++T.Failed;
	}
	else
		++T.Passed;
	free(T.TestName);
	T.TestName = 0;
}

static void startTest(const char *name)
{
	finishTest();
	resetEngine();
	T.TestName = xstrdup(name);
	T.IsSkipped = 0;
	T.IsFailed = 0;
}

/* ------------------------------------------------------------ */
/* Commands */

static const char *skipSpaces(const char *s)
{
	while (*s == ' ' || *s == '\t') ++s;
	return s;
}

/* Trims spaces in place */
static char *trim(char *s)
{
	size_t len;
	while (*s == ' ' || *s == '\t') ++s;
	len = strlen(s);
	while (len && (s[len - 1] == ' ' || s[len - 1] == '\t')) s[--len] = 0;
	return s;
}

static int startsWith(const char *s, const char *prefix)
{
	return !strncmp(s, prefix, strlen(prefix));
}

/* Returns the argument after the command word, or NULL if the line has another command */
static const char *command(const char *line, const char *name)
{
	size_t len = strlen(name);
	if (strncmp(line, name, len)) return 0;
	if (line[len] && line[len] != ' ' && line[len] != '\t') return 0;
	return skipSpaces(line + len);
}

static void expectText(const char *what, const QSP_CHAR *actual, const char *expected)
{
	StrBuf sb = {0};
	addEscaped(&sb, actual);
	if (strcmp(sbStr(&sb), expected))
		fail("%s: expected '%s', got '%s'", what, expected, sbStr(&sb));
	free(sb.Data);
}

static void expectList(const char *what, int isActions, const char *expected)
{
	StrBuf sb = {0};
	listState(&sb, isActions);
	if (strcmp(sbStr(&sb), expected))
		fail("%s: expected %s, got %s", what, expected, sbStr(&sb));
	free(sb.Data);
}

static void expectValue(const char *arg)
{
	const char *sep = strstr(arg, " => ");
	char *expr;
	Value actual, expected;
	if (!sep)
	{
		fail("expected 'EXPR => VALUE'");
		return;
	}
	expr = xmalloc(sep - arg + 1);
	memcpy(expr, arg, sep - arg);
	expr[sep - arg] = 0;
	parseLiteral(sep + 4, &expected);
	if (expected.IsString)
	{
		char *unescaped = unescape(expected.Str);
		StrBuf sb = {0};
		QSP_CHAR *tmp = toQsp(unescaped);
		addEscaped(&sb, tmp);
		free(tmp);
		free(unescaped);
		free(expected.Str);
		expected.Str = sb.Data ? sb.Data : xstrdup("");
	}
	if (!evalExpr(expr, &actual))
		fail("%s: evaluation failed with %s", expr, actual.Str);
	else if (actual.IsString != expected.IsString)
	{
		if (actual.IsString)
			fail("%s: expected %d, got string '%s'", expr, expected.Num, actual.Str);
		else
			fail("%s: expected '%s', got number %d", expr, expected.Str, actual.Num);
	}
	else if (actual.IsString ? strcmp(actual.Str, expected.Str) != 0 : actual.Num != expected.Num)
	{
		if (actual.IsString)
			fail("%s: expected '%s', got '%s'", expr, expected.Str, actual.Str);
		else
			fail("%s: expected %d, got %d", expr, expected.Num, actual.Num);
	}
	free(actual.Str);
	free(expected.Str);
	free(expr);
}

static int countEvents(const char *prefix, int isExact)
{
	int i, count = 0;
	for (i = 0; i < T.EventsCount; ++i)
	{
		if (isExact ? !strcmp(T.Events[i], prefix) : startsWith(T.Events[i], prefix))
			++count;
	}
	return count;
}

static void expectEvents(char **lines, int count)
{
	int i;
	if (count != T.EventsCount)
	{
		fail("expected %d events, got %d", count, T.EventsCount);
		fprintf(stderr, "  expected:\n");
		for (i = 0; i < count; ++i) fprintf(stderr, "    %s\n", lines[i]);
		return;
	}
	for (i = 0; i < count; ++i)
	{
		if (strcmp(lines[i], T.Events[i]))
		{
			fail("event #%d: expected '%s', got '%s'", i + 1, lines[i], T.Events[i]);
			return;
		}
	}
}

/* Executes a command, block lines are passed for the commands that take a block */
static void runCommand(const char *line, char **block, int blockCount)
{
	const char *arg;
	QSP_CHAR *qStr;
	if (T.IsSkipped || T.IsFailed) return;

	/* Game definition */
	if ((arg = command(line, "@src")))
	{
		/* The same wrapper as runTestFile() in qsp-wasm-engine */
		char **lines = xmalloc((blockCount + 5) * sizeof(char *));
		int i, n = 0;
		lines[n++] = "# start";
		lines[n++] = "---";
		lines[n++] = "# test";
		for (i = 0; i < blockCount; ++i) lines[n++] = block[i];
		lines[n++] = "---";
		parseQsps(&T.Main, lines, n);
		free(lines);
		T.IsGameDirty = 1;
	}
	else if ((arg = command(line, "@game")))
	{
		parseQsps(&T.Main, block, blockCount);
		T.IsGameDirty = 1;
	}
	else if ((arg = command(line, "@module")))
	{
		Game *mod;
		T.Modules = xrealloc(T.Modules, (T.ModulesCount + 1) * sizeof(Game));
		mod = T.Modules + T.ModulesCount++;
		memset(mod, 0, sizeof(Game));
		mod->Name = xstrdup(arg);
		parseQsps(mod, block, blockCount);
	}
	else if ((arg = command(line, "@desc")))
	{
		Location *loc = gameLoc(&T.Main, arg);
		int i;
		sbClear(&loc->Desc);
		for (i = 0; i < blockCount; ++i) addTextLine(&loc->Desc, block[i]);
		T.IsGameDirty = 1;
	}
	else if ((arg = command(line, "@action")))
	{
		/* @action LOCATION | NAME [| IMAGE] */
		char *copy = xstrdup(arg), *name, *image;
		Location *loc;
		Action *act;
		int i;
		name = strchr(copy, '|');
		if (!name)
		{
			fail("expected '@action LOCATION | NAME [| IMAGE]'");
			free(copy);
			return;
		}
		*name++ = 0;
		image = strchr(name, '|');
		if (image) *image++ = 0;
		loc = gameLoc(&T.Main, trim(copy));
		loc->Actions = xrealloc(loc->Actions, (loc->ActionsCount + 1) * sizeof(Action));
		act = loc->Actions + loc->ActionsCount++;
		memset(act, 0, sizeof(Action));
		act->Name = xstrdup(trim(name));
		act->Image = image ? xstrdup(trim(image)) : 0;
		for (i = 0; i < blockCount; ++i) addTextLine(&act->Code, block[i]);
		free(copy);
		T.IsGameDirty = 1;
	}
	/* Replies of the "user" */
	else if ((arg = command(line, "@reply")))
	{
		const char *value;
		if ((value = command(arg, "input")))
		{
			T.InputReplies = xrealloc(T.InputReplies, (T.InputRepliesCount + 1) * sizeof(char *));
			T.InputReplies[T.InputRepliesCount++] = unescape(value);
		}
		else if ((value = command(arg, "menu")))
		{
			if (T.MenuRepliesCount == (int)(sizeof(T.MenuReplies) / sizeof(T.MenuReplies[0])))
				fail("too many menu replies");
			else
				T.MenuReplies[T.MenuRepliesCount++] = atoi(value);
		}
		else if ((value = command(arg, "isplay")))
		{
			if (T.IsPlayRepliesCount == (int)(sizeof(T.IsPlayReplies) / sizeof(T.IsPlayReplies[0])))
				fail("too many isplay replies");
			else
				T.IsPlayReplies[T.IsPlayRepliesCount++] = atoi(value);
		}
		else if ((value = command(arg, "mscount")))
			T.MsCount = atoi(value);
		else
			fail("unknown reply '%s'", arg);
	}
	/* Actions */
	else if ((arg = command(line, "@run")))
	{
		checkNoPendingError();
		ensureGameLoaded();
		qStr = toQsp(*arg ? arg : "test");
		afterCall(QSPExecLocationCode(qStr, QSP_TRUE));
		free(qStr);
	}
	else if ((arg = command(line, "@exec")))
	{
		checkNoPendingError();
		ensureGameLoaded();
		qStr = toQsp(arg);
		afterCall(QSPExecString(qStr, QSP_TRUE));
		free(qStr);
	}
	else if ((arg = command(line, "@counter")))
	{
		checkNoPendingError();
		ensureGameLoaded();
		afterCall(QSPExecCounter(QSP_TRUE));
	}
	else if ((arg = command(line, "@set-input")))
	{
		char *text = unescape(arg);
		qStr = toQsp(text);
		QSPSetInputStrText(qStr);
		free(qStr);
		free(text);
	}
	else if ((arg = command(line, "@user-input")))
	{
		checkNoPendingError();
		ensureGameLoaded();
		afterCall(QSPExecUserInput(QSP_TRUE));
	}
	else if ((arg = command(line, "@select-act")))
	{
		checkNoPendingError();
		ensureGameLoaded();
		afterCall(QSPSetSelActionIndex(atoi(arg), QSP_TRUE));
	}
	else if ((arg = command(line, "@exec-act")))
	{
		checkNoPendingError();
		ensureGameLoaded();
		afterCall(QSPExecuteSelActionCode(QSP_TRUE));
	}
	else if ((arg = command(line, "@select-obj")))
	{
		checkNoPendingError();
		ensureGameLoaded();
		afterCall(QSPSetSelObjectIndex(atoi(arg), QSP_TRUE));
	}
	else if ((arg = command(line, "@restart")))
	{
		checkNoPendingError();
		ensureGameLoaded();
		afterCall(QSPRestartGame(QSP_TRUE));
	}
	else if ((arg = command(line, "@save")))
	{
		ensureGameLoaded();
		saveToSlot(arg);
	}
	else if ((arg = command(line, "@load-save")))
	{
		checkNoPendingError();
		loadFromSlot(arg);
	}
	else if ((arg = command(line, "@debug")))
		QSPEnableDebugMode(atoi(arg) != 0);
	else if ((arg = command(line, "@clear-events")))
		clearEvents();
	/* Expectations */
	else if ((arg = command(line, "@expect")))
	{
		const char *value;
		if ((value = command(arg, "main")))
			expectText("main", QSPGetMainDesc(), value);
		else if ((value = command(arg, "stat")))
			expectText("stat", QSPGetVarsDesc(), value);
		else if ((value = command(arg, "acts")))
			expectList("actions", 1, value);
		else if ((value = command(arg, "objs")))
			expectList("objects", 0, value);
		else if ((value = command(arg, "selact")))
		{
			if (QSPGetSelActionIndex() != atoi(value))
				fail("selected action: expected %d, got %d", atoi(value), QSPGetSelActionIndex());
		}
		else if ((value = command(arg, "selobj")))
		{
			if (QSPGetSelObjectIndex() != atoi(value))
				fail("selected object: expected %d, got %d", atoi(value), QSPGetSelObjectIndex());
		}
		else if ((value = command(arg, "curloc")))
			expectText("current location", QSPGetCurLoc(), value);
		else if ((value = command(arg, "error")))
		{
			if (!T.PendingError)
				fail("expected error %s, got no error", value);
			else if (!startsWith(T.PendingError, value) || (T.PendingError[strlen(value)] && T.PendingError[strlen(value)] != ' '))
				fail("expected error %s, got %s", value, T.PendingError);
			free(T.PendingError);
			T.PendingError = 0;
		}
		else if ((value = command(arg, "event")))
		{
			if (!countEvents(value, 1)) fail("expected event '%s'", value);
		}
		else if ((value = command(arg, "no-event")))
		{
			if (countEvents(value, 0)) fail("unexpected event '%s...'", value);
		}
		else if ((value = command(arg, "count")))
		{
			/* @expect count N PREFIX */
			char *end;
			long n = strtol(value, &end, 10);
			const char *prefix = skipSpaces(end);
			int actual = countEvents(prefix, 0);
			if (actual != n) fail("expected %ld events '%s...', got %d", n, prefix, actual);
		}
		else if ((value = command(arg, "events")))
			expectEvents(block, blockCount);
		else
			expectValue(arg);
	}
	else if ((arg = command(line, "@skip")))
		T.IsSkipped = 1;
	else
		fail("unknown command '%s'", line);
}

/* Commands that take the following lines (till the next @-line) as a block */
static int isBlockCommand(const char *line)
{
	const char *arg = command(line, "@expect");
	return command(line, "@src") || command(line, "@game") || command(line, "@module") ||
		command(line, "@desc") || command(line, "@action") ||
		(arg && command(arg, "events"));
}

/* ------------------------------------------------------------ */
/* Reading scenario files */

static char **readLines(const char *fileName, int *count)
{
	FILE *f = fopen(fileName, "rb");
	char **lines = 0, buf[MAX_LINE];
	int n = 0;
	if (!f) return 0;
	while (fgets(buf, sizeof(buf), f))
	{
		size_t len = strlen(buf);
		while (len && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) buf[--len] = 0;
		lines = xrealloc(lines, (n + 1) * sizeof(char *));
		lines[n++] = xstrdup(buf);
	}
	fclose(f);
	*count = n;
	return lines;
}

static int runFile(const char *fileName)
{
	int count, i, j;
	char **lines = readLines(fileName, &count);
	if (!lines)
	{
		fprintf(stderr, "can't read %s\n", fileName);
		return 0;
	}
	T.FileName = fileName;
	for (i = 0; i < count; ++i)
	{
		const char *line = lines[i], *arg;
		T.Line = i + 1;
		if (!line[0] || (line[0] == '/' && line[1] == '/')) continue;
		if (line[0] != '@')
		{
			fprintf(stderr, "%s:%d: unexpected line outside of a block: %s\n", fileName, i + 1, line);
			return 0;
		}
		if ((arg = command(line, "@test")))
		{
			startTest(arg);
			continue;
		}
		if (!T.TestName)
		{
			fprintf(stderr, "%s:%d: command before @test\n", fileName, i + 1);
			return 0;
		}
		if (isBlockCommand(line))
		{
			/* The block lasts till the next @-line; "@@" at the start of a line stands for "@" */
			int start = i + 1, n;
			char **block;
			j = start;
			while (j < count && !(lines[j][0] == '@' && lines[j][1] != '@')) ++j;
			n = j - start;
			block = xmalloc((n + 1) * sizeof(char *));
			for (n = 0; start + n < j; ++n)
				block[n] = lines[start + n][0] == '@' ? lines[start + n] + 1 : lines[start + n];
			/* Trailing empty lines and // comments belong to the formatting of the file, not to the block */
			while (n > 0 && (!block[n - 1][0] || (block[n - 1][0] == '/' && block[n - 1][1] == '/'))) --n;
			runCommand(line, block, n);
			free(block);
			i = j - 1;
		}
		else
			runCommand(line, 0, 0);
	}
	finishTest();
	for (i = 0; i < count; ++i) free(lines[i]);
	free(lines);
	return 1;
}

int main(int argc, char **argv)
{
	int i, isOk = 1;
	if (argc < 2)
	{
		fprintf(stderr, "usage: %s FILE.qspt...\n", argv[0]);
		return 2;
	}
	QSPInit();
	for (i = 1; i < argc; ++i)
		if (!runFile(argv[i])) isOk = 0;
	resetEngine();
	QSPTerminate();
	printf("%d passed, %d failed, %d skipped\n", T.Passed, T.Failed, T.Skipped);
	return (isOk && !T.Failed) ? 0 : 1;
}