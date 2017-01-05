#ifndef SCREEN_H
#define SCREEN_H

#include <QWidget>

class screen : public QWidget
{
    Q_OBJECT
public:
    explicit screen(QWidget *parent = 0);

    void blank();
signals:

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

public slots:
};

#endif // SCREEN_H
