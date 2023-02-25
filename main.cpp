#include <tuple>
#include <queue>
#include <cstdio>
#include <string>
#include <cstdlib>
#include <cstdint>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <string_view>

#include <ui_main.h>

#include <Qt>
#include <QtCore>
#include <QtWidgets>
#include <QtNetwork>

using namespace std::literals;
namespace fs = std::filesystem;

QIcon* edit_icon;
QIcon* delete_icon;
QIcon* cancel_icon;
QIcon* confirm_icon;

class UserEntry;
std::vector<UserEntry*> gg;

bool is_valid_mac(const QString& mac_str)
{
    return mac_str.size() == 17;
}

class UserEntry : public QWidget
{
    bool editMode;
    bool cancelMode;
    QString oldmac, oldname;
    QLineEdit mac, name;
    
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
    UserEntry(QString mac_str, QString name_str, QWidget* parent=nullptr)
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
            std::erase(gg,this);
            this->deleteLater();
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
            cancelMode = !(is_valid_mac(mac.text()) and name.text().length());
            update_edit_btn_icon();
        });
        
        QObject::connect(&name, &QLineEdit::textEdited, [&]()
        {
            cancelMode = !(is_valid_mac(mac.text()) and name.text().length());
            update_edit_btn_icon();
        });
    }
};

class UserList_t
{
    //
} UserList;

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    //app.setStyle("windows");
    
    Ui::MainWindow ui;
    QMainWindow window;
    ui.setupUi(&window);
    
    QVBoxLayout user_list{ui.scroll_area_contents};
    user_list.setAlignment(Qt::AlignTop);
    user_list.setSpacing(0);
    user_list.setContentsMargins(0,0,0,0);;
    
    edit_icon    = new QIcon{"edit.png"};
    cancel_icon  = new QIcon{"cancel.png"};
    delete_icon  = new QIcon{"delete.png"};
    confirm_icon = new QIcon{"confirm.png"};
    
    auto user_list_add = [&user_list](UserEntry* entry)
    {
        gg.push_back(entry);
        user_list.addWidget(entry);
    };
    
    auto user_list_clear = []()
    {
        for (UserEntry* ptr : gg)
            ptr->deleteLater();
        
        gg.clear();
    };
    
    auto update_add_user_btn = [&]()
    {
        ui.add_user_btn->setEnabled(is_valid_mac(ui.mac_input->text()) and ui.name_input->text().length());
    };
    
    QObject::connect(ui.mac_input, &QLineEdit::textEdited, update_add_user_btn);
    QObject::connect(ui.name_input, &QLineEdit::textEdited, update_add_user_btn);
    
    QObject::connect(ui.add_user_btn, &QPushButton::clicked, [&]()
    {
        //send to network and if successful
        {
            user_list_add(new UserEntry{ui.mac_input->text(), ui.name_input->text()});
            ui.mac_input->clear(); ui.name_input->clear();
        }
        /*else
        {
            //
        }*/
    });
    
    QObject::connect(ui.refresh_btn, &QPushButton::clicked, [&]()
    {
        user_list_clear();
        user_list_add(new UserEntry{"Affrrrw;d:Sddffff", "Billa Singh"});
        user_list_add(new UserEntry{"AAggggg:Bddddd:FF", "Billa Singh"});
        user_list_add(new UserEntry{"AcccAaaaaaD:EE:FF", "Billa Singh"});
        user_list_add(new UserEntry{"AA:BB:CC:DD:EE:FF", "Billa Singh"});
        user_list_add(new UserEntry{"xxxxxxxxxxD:EE:FF", "Billa Singh"});
    });
    
    window.show();
    return app.exec();
}
