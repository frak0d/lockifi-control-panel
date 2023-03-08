#include <tuple>
#include <queue>
#include <cstdio>
#include <string>
#include <cstdlib>
#include <cstdint>
#include <thread>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <string_view>

#include <ui_main.h>
#include "http_api.hpp"
#include "UserList.hpp"

#include <Qt>
#include <QtCore>
#include <QtWidgets>

using namespace std::literals;
namespace fs = std::filesystem;

bool is_valid_mac(const QString& mac)
{
    return mac.length() == 17;
}

bool is_valid_name(const QString& name)
{
    return name.size();
}

QString lock_ip;

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setStyle("breeze");
    
    Ui::MainWindow ui;
    QMainWindow window;
    ui.setupUi(&window);
    
    UserList user_list{ui.scroll_area_contents};
    
    edit_icon    = new QIcon{"edit.png"};
    cancel_icon  = new QIcon{"cancel.png"};
    delete_icon  = new QIcon{"delete.png"};
    confirm_icon = new QIcon{"confirm.png"};
    
    auto show_dialog = [](const QString& text)
    {
        //show dialog box
        std::puts(text.toStdString().c_str());
    };
    
    ///////////////////////////////////////////////////////////////////////////
    
    auto update_add_user_btn = [&]()
    {
        ui.add_user_btn->setEnabled(is_valid_mac(ui.mac_input->text()) and is_valid_name(ui.name_input->text()));
    };
    
    QObject::connect(ui.mac_input, &QLineEdit::textEdited, update_add_user_btn);
    QObject::connect(ui.name_input, &QLineEdit::textEdited, update_add_user_btn);
    
    QObject::connect(ui.add_user_btn, &QPushButton::clicked, [&]()
    {
        if (lockifi::add_user(lock_ip, ui.mac_input->text(), ui.name_input->text()))
        {
            user_list.add(new UserEntry{ui.mac_input->text(), ui.name_input->text()});
            ui.mac_input->clear(); ui.name_input->clear();
        }
        else
        {
            show_dialog("error adding user");
        }
    });
    
    ///////////////////////////////////////////////////////////////////////////
    
    QObject::connect(ui.refresh_btn, &QPushButton::clicked, [&]()
    {
        user_list.clear();
        try
        {
            for (const auto& [mac, name] : lockifi::user_list(lock_ip))
            {
                user_list.add(mac, name);
            }
        }
        catch (std::exception& e)
        {
            show_dialog(e.what());
        }
    });
    
    QObject::connect(ui.reset_btn, &QPushButton::clicked, [&]()
    {
        int error_count = 0;
        const int error_threshold = 2*user_list.entries.size();
        
        while (user_list.entries.size())
        {
            if (lockifi::remove_user(lock_ip, user_list.entries.back()->getMac()))
                user_list.entries.pop_back();
            else
                ++error_count;
            
            if (error_count > error_threshold)
            {
                show_dialog("Failed to Reset User List Properly");
                break;
            }
        }
    });
    
    QObject::connect(ui.save_csv_btn, &QPushButton::clicked, [&]()
    {
        auto fname = QFileDialog::getSaveFileName(nullptr,{},{},"CSV File(*.csv)");
        if (!fname.isNull())
        {
            //proceed
        }
    });
    
    QObject::connect(ui.load_csv_btn, &QPushButton::clicked, [&]()
    {
        auto fname = QFileDialog::getOpenFileName(nullptr,{},{},"CSV File(*.csv)");
        if (!fname.isNull())
        {
            //proceed
        }
    });
    
    ///////////////////////////////////////////////////////////////////////////
    
    QObject::connect(ui.get_log_btn, &QPushButton::clicked, [&]()
    {
        auto logdata = lockifi::access_logs(lock_ip);
        //TODO: parse log data and populate gui
    });
    
    QObject::connect(ui.save_log_btn, &QPushButton::clicked, [&]()
    {
        auto fname = QFileDialog::getSaveFileName(nullptr,{},{},"Text File(*.txt)");
        if (!fname.isNull())
        {
            try
            {
                auto logdata = lockifi::access_logs(lock_ip);
                std::ofstream file{fname.toStdString(), std::ios::binary};
                if (!file.bad()) file.write(logdata.data(), logdata.size());
            }
            catch (...)
            {
                //show error
            }
        }
    });
    
    QObject::connect(ui.save_log_btn, &QPushButton::clicked, [&]()
    {
        //
    });

    ///////////////////////////////////////////////////////////////////////////
    
    window.show();
    return app.exec();
}
