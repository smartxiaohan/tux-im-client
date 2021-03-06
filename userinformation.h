#ifndef USERINFORMATION_H
#define USERINFORMATION_H

#include <QWidget>
#include "datatype.h"

namespace Ui {
class UserInformation;
}

class UserInformation : public QWidget
{
    Q_OBJECT
    
public:
    explicit UserInformation(FriendProfile profile, bool isFriend, QWidget *parent = 0);
    ~UserInformation();
    
private slots:
    void on_pushButtonMakeFriend_clicked();

private:
    Ui::UserInformation *ui;
    FriendProfile friendProfile;

signals:
    void newFriend();
};

#endif // USERINFORMATION_H
