#ifndef DIALOG_RENDERPREVIEW_H
#define DIALOG_RENDERPREVIEW_H

#include <QDialog>

namespace Ui {
    class DialogRenderPreview;
}

class DialogRenderPreview : public QDialog
{
    Q_OBJECT

public:
    explicit DialogRenderPreview(QWidget *parent = 0);
    ~DialogRenderPreview();

private:
    Ui::DialogRenderPreview *ui;
};

#endif // DIALOG_RENDERPREVIEW_H
