#ifndef IOPADS_H
#define IOPADS_H

#include <QWidget>

namespace Ui {
class IOPads;
}

class IOPads : public QWidget
{
    Q_OBJECT

public:
    explicit IOPads(QWidget *parent = 0);
    ~IOPads();

private:
    Ui::IOPads *ui;
};

#endif // IOPADS_H
