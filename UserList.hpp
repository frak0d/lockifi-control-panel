#pragma once

#include <QtCore>
#include <QtWidgets>

QIcon* edit_icon;
QIcon* delete_icon;
QIcon* cancel_icon;
QIcon* confirm_icon;

bool is_valid_mac(const QString& mac);
bool is_valid_name(const QString& mac);

class UserEntry : public QWidget
{
    bool editMode;
    bool cancelMode;
    QLineEdit mac, name;
    QString oldmac, oldname;
    
    QHBoxLayout layout{this};
    QPushButton del_btn{*delete_icon, "", this};
    QPushButton edit_btn{*edit_icon, "", this};
    
    void update_edit_btn_icon()
    {
        if (editMode)
            if (cancelMode)
                edit_btn.setIcon(*cancel_icon);
            else
                edit_btn.setIcon(*confirm_icon);
        else
            edit_btn.setIcon(*edit_icon);
    }
    
public:
    UserEntry(QString mac_str, QString name_str, QWidget* parent=nullptr);
    QString getMac()  const {return  mac.text();}
    QString getName() const {return name.text();}
};

class UserList : public QWidget
{
    QVBoxLayout scrollbox;
    
public:
    std::vector<UserEntry*> entries;
    
    UserList(QWidget* parent=nullptr) : QWidget{parent}, scrollbox{this}
    {
        setMinimumSize(parent->geometry().size());
        scrollbox.setAlignment(Qt::AlignTop);
        scrollbox.setSpacing(0);
        scrollbox.setContentsMargins(0,0,0,0);
    }
    
    void add(UserEntry* entry)
    {
        entries.push_back(entry);
        scrollbox.addWidget(entry);
    }
    
    void add(QString mac, QString name)
    {
        add(new UserEntry{mac, name, this});
    }
    
    void remove(UserEntry* entry)
    {
        std::erase(entries, entry);
        entry->deleteLater();
    }
    
    void clear() noexcept
    {
        for (UserEntry* entry : entries)
            entry->deleteLater();
        
        entries.clear();
    }
};

UserEntry::UserEntry(QString mac_str, QString name_str, QWidget* parent)
         : QWidget{parent}, mac{mac_str, this}, name{name_str, this}
{
    mac.setFrame(false); name.setFrame(false);
    mac.setMaxLength(17); name.setMaxLength(35);
    mac.setReadOnly(true); name.setReadOnly(true);
    mac.setMaximumHeight(35); name.setMaximumHeight(35);
    del_btn.setMaximumHeight(35); edit_btn.setMaximumHeight(35);
    del_btn.setFlat(true); edit_btn.setFlat(true);
    
    mac.setAlignment(Qt::AlignCenter);
    layout.setSpacing(2);
    layout.setContentsMargins(4,1,4,1);
    
    layout.addWidget(&mac, 3); layout.addWidget(&name, 5);
    layout.addWidget(&del_btn); layout.addWidget(&edit_btn);
    
    QObject::connect(&del_btn, &QPushButton::clicked, [&]()
    {
        if (parentWidget()) try
        {
            auto user_list = dynamic_cast<UserList*>(parentWidget());
            if (user_list) user_list->remove(this);
        }
        catch(...){}
    });
    
    QObject::connect(&edit_btn, &QPushButton::clicked, [&]()
    {
        if (editMode)
        {
            if (cancelMode)
            {
                mac.setText(oldmac);
                name.setText(oldname);
            }
            else //confirm mode
            {
                //send to network
            }
        }
        else
        {
            oldmac = mac.text();
            oldname = name.text();
        }
        
        editMode = !editMode;
        mac.setFrame(editMode); name.setFrame(editMode);
        mac.setReadOnly(!editMode); name.setReadOnly(!editMode);
        update_edit_btn_icon();
    });
    
    QObject::connect(&mac, &QLineEdit::textEdited, [&]()
    {
        cancelMode = !(is_valid_mac(mac.text()) and is_valid_name(name.text()))
                   or (mac.text() == oldmac and name.text() == oldname);
        update_edit_btn_icon();
    });
    
    QObject::connect(&name, &QLineEdit::textEdited, [&]()
    {
        cancelMode = !(is_valid_mac(mac.text()) and is_valid_name(name.text()))
                   or (mac.text() == oldmac and name.text() == oldname);
        update_edit_btn_icon();
    });
}
