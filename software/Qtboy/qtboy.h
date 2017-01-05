#ifndef QTBOY_H
#define QTBOY_H

#include <QWidget>
#include "abstractarduboy.h"
#include "screen.h"

namespace Ui {
class QtBoy;
}

class QtBoy : public QWidget, AbstractArduboy
{
    Q_OBJECT

public:
    explicit QtBoy(QWidget *parent = 0);
    ~QtBoy();

    void start();

    long getTime();

    void LCDDataMode(){};
    void LCDCommandMode(){};
    void drawScreen(const unsigned char *image);

    uint8_t getInput();

private:
    Ui::QtBoy *ui;
};

#endif // QTBOY_H
