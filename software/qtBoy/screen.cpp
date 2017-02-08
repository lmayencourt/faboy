#include "screen.h"
#include <QPainter>

Screen::Screen(QWidget *parent) : QWidget(parent)
{
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

void Screen::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

//    painter.setPen();
//    painter.setBrush();

    painter.drawLine(0,0,50,50);
}
