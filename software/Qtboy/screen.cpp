#include "screen.h"

#include <qpen.h>
#include <qpainter.h>

screen::screen(QWidget *parent) : QWidget(parent)
{

    QPalette Pal(palette());

    // set black background
    Pal.setColor(QPalette::Background, Qt::black);
    this->setAutoFillBackground(true);
    this->setPalette(Pal);
    this->show();

}

void screen::blank()
{
    QPainter painter(this);
    painter.drawRect(0,0,width()-1, height()-1);
}

void screen::paintEvent(QPaintEvent *event)
{
    QPen pen;
    pen.setColor(Qt::white);

    QPainter painter(this);
    painter.setPen(pen);

    painter.drawLine(0,0,width()-1,height()-1);
}
