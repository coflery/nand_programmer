/*  Copyright (C) 2017 Bogdan Bogush <bogdan.s.bogush@gmail.com>
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 */

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class Programmer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    Programmer *prog;
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void log(QString logMsg);

private:
    Ui::MainWindow *ui;

    void insertBufferRow(quint8 *readBuf, quint32 size, quint32 rowNum,
        quint32 address);

public slots:
    void slotFileOpen();
    void slotProgConnect();
    void slotProgReadDeviceId();
    void slotProgErase();
    void slotProgRead();
    void slotProgWrite();
    void slotSelectChip(int selectedChipNum);
};

#endif // MAIN_WINDOW_H
