#include "qtboy.h"
#include "ui_qtboy.h"

QtBoy::QtBoy(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QtBoy)
{
    ui->setupUi(this);
}

QtBoy::~QtBoy()
{
    delete ui;
}

void QtBoy::start()
{
    ui->widget->blank();
}

long QtBoy::getTime()
{

}

void QtBoy::drawScreen(const unsigned char *image)
{

}

uint8_t QtBoy::getInput()
{

}
