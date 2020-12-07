/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#ifndef READER_H
#define READER_H

#include <QThread>
#include <QSerialPort>

class Reader : public QThread
{
    Q_OBJECT

    QSerialPort *serialPort;
    QString portName;
    qint32 baudRate;
    uint8_t *rbuf;
    uint32_t rlen;
    const uint8_t *wbuf;
    uint32_t wlen;
    uint32_t readOffset;
    uint32_t bytesRead;
    uint32_t bytesReadNotified;
    bool isSkipBB;
    bool isReadLess;

    int serialPortCreate();
    void serialPortDestroy();
    int write(const uint8_t *data, uint32_t len);
    int readStart();
    int read(uint8_t *pbuf, uint32_t len);
    int handleError(uint8_t *pbuf, uint32_t len);
    int handleProgress(uint8_t *pbuf, uint32_t len);
    int handleBadBlock(uint8_t *pbuf, uint32_t len, bool isSkipped);
    int handleStatus(uint8_t *pbuf, uint32_t len);
    int handleData(uint8_t *pbuf, uint32_t len);
    int handlePacket(uint8_t *pbuf, uint32_t len);
    int handlePackets(uint8_t *pbuf, uint32_t len);
    int readData();
    void run() override;
    void logErr(const QString& msg);
    void logInfo(const QString& msg);

public:
    void init(const QString &portName, qint32 baudRate, uint8_t *rbuf,
        uint32_t rlen, const uint8_t *wbuf, uint32_t wlen, bool isSkipBB,
        bool isReadLess);
signals:
    void result(int ret);
    void progress(unsigned int progress);
    void log(QtMsgType msgType, QString msg);
};

#endif // READER_H
