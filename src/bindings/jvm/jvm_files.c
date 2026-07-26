#include "../../declarations.h"

#ifdef _JVM_BINDING

#include "../../errors.h"
#include "../../game.h"

void qspOpenQuestFromFILE(FILE *f, const QSP_BOOL isAddLocs)
{
    fseek(f, 0, SEEK_END);
    const int fileSize = ftell(f);

    char *buf = malloc(fileSize + 3);
    if (buf != NULL)
    {
        fseek(f, 0, SEEK_SET);
        fread(buf, 1, fileSize, f);

        buf[fileSize] = buf[fileSize + 1] = buf[fileSize + 2] = 0;

        qspOpenQuestFromData(buf, fileSize + 3, isAddLocs);
        free(buf);
    } else {
        qspSetError(QSP_ERR_FILENOTFOUND);
    }
}

void qspOpenGameStatusFromFILE(FILE *f)
{
    int fileLen;
    QSP_CHAR *buf;

    fseek(f, 0, SEEK_END);
    fileLen = ftell(f) / sizeof(QSP_CHAR);
    buf = (QSP_CHAR *)malloc((fileLen + 1) * sizeof(QSP_CHAR));

    fseek(f, 0, SEEK_SET);
    fread(buf, sizeof(QSP_CHAR), fileLen, f);
    buf[fileLen] = 0;

    qspOpenGameStatusFromString(buf);
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