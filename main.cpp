#include <tuple>
#include <queue>
#include <cstdio>
#include <string>
#include <cstdlib>
#include <cstdint>
#include <thread>
#include <fstream>
#include <sstream>
#include <ranges>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <string_view>

#include <Qt>
#include <QtCore>
#include <QtWidgets>
#include <QtNetwork>

#include <ui_main.h>

#include "validity.hpp"
#include "http_api.hpp"
#include "UserList.hpp"

using namespace std::literals;
namespace fs = std::filesystem;

QString lock_ip;

auto local_ip_prefixes() // returns list of local ip's with last part replaced with %1 eg. 192.168.43.%1
{
    QStringList prefixes;
    for (const QNetworkInterface& netInterface : QNetworkInterface::allInterfaces())
    {
        QNetworkInterface::InterfaceFlags flags = netInterface.flags();
        
        if ((flags & QNetworkInterface::IsRunning) && !(flags & QNetworkInterface::IsLoopBack))
        {
            for (const QNetworkAddressEntry& address : netInterface.addressEntries())
            {
                if (address.ip().protocol() == QAbstractSocket::IPv4Protocol)
                {
                    auto ip_str = address.ip().toString();
                    auto ip_end = ip_str.split('.').last();
                    
                    ip_str.truncate(ip_str.length()-ip_end.length());
                    ip_str.append("%1");
                    
                    prefixes += ip_str;
                }
            }
        }
    }
    return prefixes;
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setStyle("breeze");
    
    Ui::MainWindow ui;
    QMainWindow window;
    ui.setupUi(&window);
    
    curl_global_init(CURL_GLOBAL_ALL);
    std::atexit(curl_global_cleanup);    
    
    UserList user_list{ui.scroll_area_contents};
    ui.level_input->addItems({"   Admin","   Third","   Half","   Full","   Ultra"});
    ui.level_input->setCurrentIndex(3);;
    
    edit_icon    = new QIcon{"edit.png"};
    cancel_icon  = new QIcon{"cancel.png"};
    delete_icon  = new QIcon{"delete.png"};
    confirm_icon = new QIcon{"confirm.png"};
    
    auto show_dialog = [](const QString& text)
    {
        QMessageBox::warning(nullptr, "Error", text);
    };
    
    ///////////////////////////////////////////////////////////////////////////
    
    QObject::connect(ui.scan_btn, &QPushButton::clicked, [&]
    {
        QStringList online_locks;
        
        for (const QString& ip_prefix : local_ip_prefixes())
        {
            std::array<std::future<bool>, 256> results;
            
            for (uint i=0 ; i < results.size() ; ++i)
                results[i] = std::async(lockifi::ping, ip_prefix.arg(i));
            
            for (uint i=0 ; i < results.size() ; ++i)
                if (results[i].get()) online_locks.push_back(ip_prefix.arg(i));
        }
        
        ui.ip_combobox->blockSignals(true);
        ui.ip_combobox->clear();
        ui.ip_combobox->blockSignals(false);
        ui.ip_combobox->addItems(online_locks);
    });
    
    QObject::connect(ui.ip_combobox, QOverload<int>::of(&QComboBox::currentIndexChanged), [&](int index)
    {
        lock_ip = ui.ip_combobox->itemText(index);
        ui.refresh_btn->click();
    });
    
    ///////////////////////////////////////////////////////////////////////////
    
    auto update_add_user_btn = [&]
    {
        ui.add_user_btn->setEnabled(is_valid_mac(ui.mac_input->text()) and
                                    is_valid_name(ui.name_input->text()));
    };
    
    QObject::connect(ui.mac_input, &QLineEdit::textEdited, update_add_user_btn);
    QObject::connect(ui.name_input, &QLineEdit::textEdited, update_add_user_btn);
    
    QObject::connect(ui.add_user_btn, &QPushButton::clicked, [&]
    {
        try
        {
            lockifi::add_user(lock_ip, ui.mac_input->text(), ui.name_input->text(), ui.level_input->currentIndex());
            user_list.add(ui.mac_input->text(), ui.name_input->text(), ui.level_input->currentIndex());
            ui.mac_input->clear(); ui.name_input->clear();
        }
        catch (std::exception& e) {show_dialog(e.what());}
    });
    
    ///////////////////////////////////////////////////////////////////////////
    
    QObject::connect(ui.refresh_btn, &QPushButton::clicked, [&]
    {
        user_list.clear();
        try
        {
            for (const auto& [mac, name, level] : lockifi::user_list(lock_ip))
            {
                user_list.add(mac, name, level);
            }
        }
        catch (std::exception& e)
        {
            show_dialog(e.what());
        }
    });
    
    QObject::connect(ui.reset_btn, &QPushButton::clicked, [&]
    {
        int error_count = 0;
        const int error_threshold = 2*user_list.entries.size();
        
        while (user_list.entries.size())
        {
            try
            {
                lockifi::remove_user(lock_ip, user_list.entries.back()->getMac());
                user_list.entries.pop_back();
            }
            catch (...) {++error_count;}
            
            if (error_count > error_threshold)
            {
                show_dialog("Failed to Reset User List Properly");
                break;
            }
        }
    });
    
    QObject::connect(ui.save_csv_btn, &QPushButton::clicked, [&]
    {
        auto fname = QFileDialog::getSaveFileName(nullptr,{},{},"CSV File(*.csv)");
        if (!fname.isNull())
        {
            //proceed
        }
    });
    
    QObject::connect(ui.load_csv_btn, &QPushButton::clicked, [&]
    {
        auto fname = QFileDialog::getOpenFileName(nullptr,{},{},"CSV File(*.csv)");
        if (!fname.isNull())
        {
            //proceed
        }
    });
    
    ///////////////////////////////////////////////////////////////////////////
    
    QObject::connect(ui.get_log_btn, &QPushButton::clicked, [&]
    {
        auto logdata = lockifi::access_logs(lock_ip);
        //TODO: parse log data and populate gui
    });
    
    QObject::connect(ui.save_log_btn, &QPushButton::clicked, [&]
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
    
    QObject::connect(ui.save_log_btn, &QPushButton::clicked, [&]
    {
        //
    });

    ///////////////////////////////////////////////////////////////////////////
    
    window.show();
    ui.scan_btn->click();
    return app.exec();
}
