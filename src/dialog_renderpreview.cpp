#include "dialog_renderpreview.h"
#include "ui_dialog_renderpreview.h"

DialogRenderPreview::DialogRenderPreview(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogRenderPreview)
{
    ui->setupUi(this);
}

DialogRenderPreview::~DialogRenderPreview()
{
    delete ui;
}
