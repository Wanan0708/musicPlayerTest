#include "mvshow.h"
#include "ui_mvshow.h"

mvShow::mvShow(QWidget *parent) :
    QVideoWidget(parent),
    ui(new Ui::mvShow)
{
    ui->setupUi(this);
}

mvShow::~mvShow()
{
    delete ui;
}

void mvShow::closeEvent(QCloseEvent *event)
{
    emit stopPlsySig();
    event->accept();
//    this->destroy(true, true);
}
