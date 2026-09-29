#include "../../declarations.h"

#ifdef _JVM_BINDING

#include "../../errors.h"
#include "../../game.h"

void qspOpenQuestFromFILE(FILE *f, const QSP_BOOL isAddLocs)
{
    int dataSize;
    char *buf = qspReadFileData(f, 3, &dataSize);
    if (!buf)
    {
        qspSetError(QSP_ERR_CANTLOADFILE);
        return;
    }
    qspOpenQuestFromData(buf, dataSize + 3, isAddLocs);
    free(buf);
}

void qspOpenGameStatusFromFILE(FILE *f)
{
    int dataSize;
    char *buf = qspReadFileData(f, (int)sizeof(QSP_CHAR), &dataSize);
    if (!buf)
    {
        qspSetError(QSP_ERR_CANTLOADFILE);
        return;
    }
    ((QSP_CHAR *) buf)[dataSize / sizeof(QSP_CHAR)] = 0;
    qspOpenGameStatusFromString((QSP_CHAR *) buf);
    free(buf);
}

void qspSaveGameStatusToFILE(FILE *f)
{
    int len;
    QSP_CHAR *buf;
    if ((len = qspSaveGameStatusToString(&buf)))
    {
        fwrite(buf, sizeof(QSP_CHAR), len, f);
        free(buf);
    }
}

#endif
