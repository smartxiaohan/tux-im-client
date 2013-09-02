#ifndef LOGIN_H
#define LOGIN_H

#include <QWidget>

namespace Ui {
    class Login;
}

class Login : public QWidget
{
    Q_OBJECT

public:
    explicit Login(QWidget *parent = 0);
    ~Login();

private:
    Ui::Login *ui;

private slots:
    void on_regButton_clicked();
};

#endif // WIDGET_H