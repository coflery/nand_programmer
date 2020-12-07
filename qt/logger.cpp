/*  Copyright (C) 2020 NANDO authors
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#include "logger.h"
#include <QDebug>
#include <QScrollBar>

Logger *Logger::logger;
QTextEdit *Logger::logTextEdit;
int Logger::refCount;

void Logger::logHandler(QtMsgType type, const QMessageLogContext &context ,
    const QString &msg)
{
    const char *prefix = "";
    QString formatMsg;

    (void)context;

    switch (type) {
    case QtDebugMsg:
        prefix = "Debug: ";
        break;
    case QtInfoMsg:
        prefix = "Info: ";
        break;
    case QtWarningMsg:
        prefix = "Warning: ";
        break;
    case QtCriticalMsg:
        prefix = "Error: ";
        break;
    case QtFatalMsg:
        prefix = "Fatal error: ";
        break;
    }

    formatMsg = QString("%1%2\n").arg(prefix).arg(msg);

    if (type == QtFatalMsg || !logTextEdit)
    {
        QByteArray ba = formatMsg.toLatin1();
        fputs(ba.data(), stderr);
        if (type == QtFatalMsg)
            abort();
    }
    else
    {
        logTextEdit->moveCursor(QTextCursor::End);
        logTextEdit->insertPlainText(formatMsg);

        logTextEdit->verticalScrollBar()->
            setValue(logTextEdit->verticalScrollBar()->maximum());

    }
}

Logger::Logger()
{
    qInstallMessageHandler(logHandler);
}

Logger::~Logger()
{
    qInstallMessageHandler(nullptr);
}

Logger *Logger::getInstance()
{
    if (!logger)
        logger = new Logger();
    refCount++;

    return logger;
}

void Logger::putInstance()
{
    if (!refCount)
    {
        qWarning() << "Logger is released without being allocated";
        return;
    }

    if (!--refCount && logger)
    {
        delete logger;
        logger = nullptr;
        logTextEdit = nullptr;
    }
}

void Logger::setTextEdit(QTextEdit *textEdit)
{
    logTextEdit = textEdit;
}

