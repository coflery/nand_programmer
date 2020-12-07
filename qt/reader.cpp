/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#include "reader.h"
#include "cmd.h"
#include "err.h"
#include <QDebug>
#include <QTextBlock>
#include <QTextCursor>

#define READ_TIMEOUT 10000
#define BUF_SIZE 4096
#define NOTIFY_LIMIT 131072 // 128KB

Q_DECLARE_METATYPE(QtMsgType)

void Reader::init(const QString &portName, qint32 baudRate, uint8_t *rbuf,
    uint32_t rlen, const uint8_t *wbuf, uint32_t wlen, bool isSkipBB,
    bool isReadLess)
{
    this->portName = portName;
    this->baudRate = baudRate;
    this->rbuf = rbuf;
    this->rlen = rlen;
    this->wbuf = wbuf;
    this->wlen = wlen;
    this->isSkipBB = isSkipBB;
    this->isReadLess = isReadLess;
    readOffset = 0;
    bytesRead = 0;
    bytesReadNotified = 0;
}

int Reader::write(const uint8_t *data, uint32_t len)
{
    qint64 ret;

    ret = serialPort->write(reinterpret_cast<const char *>(data), len);
    if (ret < 0)
    {
        logErr(QString("Failed to write: %1").arg(serialPort->errorString()));
        return -1;
    }
    else if (ret < len)
    {
        logErr(QString("Data was partialy written, returned %1, expected %2")
            .arg(ret).arg(len));
        return -1;
    }

    return 0;
}

int Reader::readStart()
{
    return write(wbuf, wlen);
}

int Reader::read(uint8_t *pbuf, uint32_t len)
{
    qint64 ret;

    if (!serialPort->waitForReadyRead(READ_TIMEOUT))
    {
        logErr("Read data timeout");
        return -1;
    }

    ret = serialPort->read(reinterpret_cast<char *>(pbuf), len);
    if (ret < 0)
    {
        logErr("Failed to read data");
        return -1;
    }

    return static_cast<int>(ret);
}

int Reader::handleBadBlock(uint8_t *pbuf, uint32_t len, bool isSkipped)
{
    RespBadBlock *badBlock = reinterpret_cast<RespBadBlock *>(pbuf);
    size_t size = sizeof(RespBadBlock);
    QString message = isSkipped ? "Skipped bad block at 0x%1 size 0x%2" :
        "Bad block at 0x%1 size 0x%2";

    if (len < size)
        return 0;

    logInfo(message.arg(badBlock->addr, 8, 16, QLatin1Char('0'))
        .arg(badBlock->size, 8, 16, QLatin1Char('0')));

    if (rlen && isSkipBB && isReadLess)
    {
        if (bytesRead + badBlock->size > rlen)
            bytesRead = rlen;
        else
            bytesRead += badBlock->size;
    }

    return static_cast<int>(size);
}

int Reader::handleError(uint8_t *pbuf, uint32_t len)
{
    RespError *err = reinterpret_cast<RespError *>(pbuf);
    size_t size = sizeof(RespError);

    if (len < size)
        return 0;

    logErr(QString("Programmer sent error: (%1) %2").arg(err->errCode)
        .arg(errCode2str(-err->errCode)));

    return -1;
}

int Reader::handleProgress(uint8_t *pbuf, uint32_t len)
{
    RespProgress *resp = reinterpret_cast<RespProgress *>(pbuf);
    size_t size = sizeof(RespProgress);

    if (len < size)
        return 0;

    emit progress(resp->progress);

    return static_cast<int>(size);
}

int Reader::handleStatus(uint8_t *pbuf, uint32_t len)
{
    RespHeader *header = reinterpret_cast<RespHeader *>(pbuf);

    switch (header->info)
    {
    case STATUS_ERROR:
        return handleError(pbuf, len);
    case STATUS_BB:
        return handleBadBlock(pbuf, len, false);
    case STATUS_BB_SKIP:
        return handleBadBlock(pbuf, len, true);
    case STATUS_PROGRESS:
        return handleProgress(pbuf, len);
    case STATUS_OK:
        // Exit read loop
        if (!rlen)
            bytesRead = 1;
        break;
    default:
        logErr(QString("Wrong response header info %1").arg(header->info));
        return -1;
    }

    return 0;
}

int Reader::handleData(uint8_t *pbuf, uint32_t len)
{
    RespHeader *header = reinterpret_cast<RespHeader *>(pbuf);
    uint8_t dataSize = header->info;
    size_t headerSize = sizeof(RespHeader), packetSize = headerSize + dataSize;

    if (!dataSize || packetSize > BUF_SIZE)
    {
        logErr(QString("Wrong data length in response header: %1")
            .arg(dataSize));
        return -1;
    }

    if (len < packetSize)
        return 0;

    if (dataSize + readOffset > this->rlen)
    {
        logErr("Read buffer overflow");
        return -1;
    }

    memcpy(rbuf + readOffset, header->data, dataSize);
    readOffset += dataSize;
    bytesRead += dataSize;

    return static_cast<int>(packetSize);
}

int Reader::handlePacket(uint8_t *pbuf, uint32_t len)
{
    RespHeader *header = reinterpret_cast<RespHeader *>(pbuf);

    if (len < sizeof(RespHeader))
        return 0;

    switch (header->code)
    {
    case RESP_STATUS:
        return handleStatus(pbuf, len);
    case RESP_DATA:
        return handleData(pbuf, len);
    default:
        logErr(QString("Programmer returned wrong response code: %1")
            .arg(header->code));
        return -1;
    }
}

int Reader::handlePackets(uint8_t *pbuf, uint32_t len)
{
    int ret;
    uint32_t offset = 0;
    do
    {
        if ((ret = handlePacket(pbuf + offset, len - offset)) < 0)
            return -1;

        if (ret)
            offset += static_cast<uint32_t>(ret);
        else
        {
            memmove(pbuf, pbuf + offset, len - offset);
            break;
        }
    }
    while (offset < len);

    return static_cast<int>(len - offset);
}

int Reader::readData()
{
    uint8_t pbuf[BUF_SIZE];
    int len, offset = 0;

    do
    {
        if ((len = read(pbuf + offset,
            BUF_SIZE - static_cast<uint32_t>(offset))) < 0)
        {
            return -1;
        }
        len += offset;

        if ((offset = handlePackets(pbuf, static_cast<uint32_t>(len))) < 0)
            return -1;

        if (bytesRead >= bytesReadNotified + NOTIFY_LIMIT)
        {
            emit progress(bytesRead);
            bytesReadNotified = bytesRead;
        }
    }
    while (!bytesRead || (rlen && rlen != bytesRead));

    return 0;
}

int Reader::serialPortCreate()
{
    serialPort = new QSerialPort();

    serialPort->setPortName(portName);
    serialPort->setBaudRate(baudRate);

    if (!serialPort->open(QIODevice::ReadWrite))
    {
        logErr(QString("Failed to open serial port: %1")
            .arg(serialPort->errorString()));
        return -1;
    }

    return 0;
}

void Reader::serialPortDestroy()
{
    serialPort->close();
    delete serialPort;
}

void Reader::run()
{
    int ret = -1;

    /* Required for logger */
    qRegisterMetaType<QtMsgType>();

    if (serialPortCreate())
        goto Exit;

    if (readStart())
        goto Exit;

    if (readData())
        goto Exit;

    ret = 0;
Exit:
    serialPortDestroy();
    emit result(ret);
}

void Reader::logErr(const QString& msg)
{
    emit log(QtCriticalMsg, msg);
}

void Reader::logInfo(const QString& msg)
{
    emit log(QtInfoMsg, msg);
}

