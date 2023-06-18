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
        catch (const std::exception& e) {show_dialog(e.what());}
    });
    
    ///////////////////////////////////////////////////////////////////////////
    
    QObject::connect(ui.refresh_btn, &QPushButton::clicked, [&]
    {
        user_list.clear();
        
        try
        {
            auto sorted_user_list =  lockifi::user_list(lock_ip);
            
            std::ranges::sort(sorted_user_list, [](const auto& user1, const auto& user2)
            {
                return std::get<1>(user1) < std::get<1>(user2); //alphabetically sort names
            });
            
            for (const auto& [mac, name, level] : sorted_user_list)
            {
                user_list.add(mac, name, level);
            }
        }
        catch (const std::exception& e)
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
        
        ui.refresh_btn->click();
    });
    
    ///////////////////////////////////////////////////////////////////////////
    
    QObject::connect(ui.save_csv_btn, &QPushButton::clicked, [&]
    {
        auto fname = QFileDialog::getSaveFileName(nullptr,{},{},"Lockifi Database(*.csv)");
        
        if (!fname.isNull()) try
        {
            std::ofstream db{fname.toStdString()};
            
            for (const UserEntry* entry : user_list.entries)
            {
                db << entry->getLevel() << ','
                   << entry->getMac().toStdString() << ','
                   << entry->getName().toStdString() << '\n';
            }
        }
        catch (const std::exception& e)
        {
            show_dialog(e.what());
        }
    });
    
    QObject::connect(ui.load_csv_btn, &QPushButton::clicked, [&]
    {
        auto fname = QFileDialog::getOpenFileName(nullptr,{},{},"Lockifi Database(*.csv)");
        
        if (!fname.isNull()) try
        {
            std::stringstream db;
            std::ifstream f{fname.toStdString()};
            
            db << f.rdbuf();
            
            std::string line,mac,name,level;
            
            auto trim = [](std::string& s, const char* t = " \t\n\r\f\v")
            {
                s.erase(0, s.find_first_not_of(t));
                s.erase(s.find_last_not_of(t) + 1);
            };
            
            int total_count{0}, error_count{0};
            
            while (std::getline(db, line)) try
            {
                trim(line);
                if (line.empty()) continue;
                
                std::stringstream ss;
                ss << line;
                
                std::getline(ss, level, ',');
                std::getline(ss, mac  , ',');
                std::getline(ss, name , ',');
                
                trim(level); trim(mac); trim(name); ++total_count;
                
                if (is_valid_mac(mac.c_str()) and is_valid_name(name.c_str())) try
                {
                    lockifi::add_user(lock_ip, mac.c_str(), name.c_str(), std::stoul(level));
                }
                catch(...){++error_count;}
            }
            catch (const std::exception& e)
            {
                std::cerr << e.what() << std::endl;
            }
            
            show_dialog(QString("processed %1 entries\n"
                                "and got %2 error cases").arg(total_count, error_count));
            
            ui.refresh_btn->click();
        }
        catch (const std::exception& e)
        {
            show_dialog(e.what());
        }
    });
    
    ///////////////////////////////////////////////////////////////////////////
    
    QObject::connect(ui.get_log_btn, &QPushButton::clicked, [&]
    {
        try
        {
            auto logdata = lockifi::access_logs(lock_ip);
            ui.log_view->setText(logdata.c_str());
        }
        catch (const std::exception& e)
        {
            show_dialog(e.what());
        }
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
            catch (const std::exception& e)
            {
                show_dialog(e.what());
            }
        }
    });
    
    ///////////////////////////////////////////////////////////////////////////
    
    window.show();
    ui.scan_btn->click();
    return app.exec();
}
