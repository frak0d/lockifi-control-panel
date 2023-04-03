#pragma once

#include <QtCore>
#include <QtWidgets>
#include "http_api.hpp"

QIcon* edit_icon;
QIcon* delete_icon;
QIcon* cancel_icon;
QIcon* confirm_icon;

extern QString lock_ip;

bool is_valid_mac(const QString& mac);
bool is_valid_name(const QString& mac);

class UserEntry : public QWidget
{
    bool editMode{0};
    bool cancelMode{1};
    uint8_t oldlevel{3};
    QLineEdit mac, name;
    QString oldmac, oldname;
    
    QHBoxLayout layout{this};
    QComboBox level{this};
    QPushButton del_btn{*delete_icon, "", this};
    QPushButton edit_btn{*edit_icon, "", this};
    
    void update_cancel_mode()
    {
        cancelMode = !(is_valid_mac(mac.text()) and is_valid_name(name.text()))
                   or (mac.text() == oldmac and name.text() == oldname and level.currentIndex() == oldlevel);        
    }
    
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
    
    void backup_fields()
    {
        oldmac = mac.text();
        oldname = name.text();
        oldlevel = level.currentIndex();
    }
    
    void restore_fields()
    {
        mac.setText(oldmac);
        name.setText(oldname);
        level.setCurrentIndex(oldlevel);
    }
    
public:
    UserEntry(QString mac_str, QString name_str, uint8_t usr_lvl, QWidget* parent=nullptr);
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
    
    void add(QString mac, QString name, uint8_t level)
    {
        add(new UserEntry{mac, name, level, this});
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

UserEntry::UserEntry(QString mac_str, QString name_str, uint8_t usr_lvl, QWidget* parent)
         : QWidget{parent}, mac{mac_str, this}, name{name_str, this}, oldmac{mac_str}, oldname{name_str}
{
    mac.setFont(QFont{"Monospace"});
    mac.setAlignment(Qt::AlignCenter);
    mac.setFrame(false); name.setFrame(false);
    mac.setMaxLength(17); name.setMaxLength(35);
    mac.setReadOnly(true); name.setReadOnly(true);
    mac.setMaximumHeight(35); name.setMaximumHeight(35);
    
    del_btn.setFlat(true); edit_btn.setFlat(true);
    del_btn.setMaximumHeight(35); edit_btn.setMaximumHeight(35);
    
    level.setFrame(false);
    level.setEnabled(false);
    level.setMaximumHeight(35);
    level.addItems({"   Admin","   Third","   Half","   Full","   Ultra"}); // Ultra = Full + Admin
    level.setCurrentIndex(usr_lvl);                                         // Admin Can NOT Unlock
    
    layout.setSpacing(2);
    layout.setContentsMargins(4,1,4,1);
    layout.addWidget(&mac, 4); layout.addWidget(&name, 4);
    layout.addWidget(&level);layout.addWidget(&del_btn); layout.addWidget(&edit_btn);
    
    QObject::connect(&del_btn, &QPushButton::clicked, [&]
    {
        try
        {
            lockifi::remove_user(lock_ip, mac.text());
            
            if (parentWidget())
            {
                auto user_list = dynamic_cast<UserList*>(parentWidget());
                if (user_list) user_list->remove(this);
            }
            else this->~UserEntry();
        }
        catch (std::exception& e) {QMessageBox::warning(nullptr, "Delete Error", e.what());}
    });
    
    QObject::connect(&mac, &QLineEdit::textEdited, [&]
    {
        update_cancel_mode();
        update_edit_btn_icon();
    });
    
    QObject::connect(&name, &QLineEdit::textEdited, [&]
    {
        update_cancel_mode();
        update_edit_btn_icon();
    });
    
    QObject::connect(&level, &QComboBox::currentTextChanged, [&]
    {
        update_cancel_mode();
        update_edit_btn_icon();
    });
    
    QObject::connect(&edit_btn, &QPushButton::clicked, [&]
    {
        if (editMode)
        {
            if (cancelMode)
            {
                restore_fields();
            }
            else //confirm mode
            {
                try {lockifi::add_user(lock_ip, mac.text(), name.text(), level.currentIndex());}
                catch(std::exception& e)
                {
                    restore_fields();
                    QMessageBox::warning(nullptr, "Edit Error", e.what());
                }
            }
        }
        else backup_fields();
        
        editMode = !editMode;
        level.setEnabled(editMode);
        mac.setFrame(editMode); name.setFrame(editMode);
        mac.setReadOnly(!editMode); name.setReadOnly(!editMode);
        update_edit_btn_icon();
    });
}
