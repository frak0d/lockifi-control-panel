#pragma once

#include <deque>
#include <chrono>
#include <thread>
#include <utility>

#include <QLabel>
#include <QString>

class StatusText
{
    QLabel* label;
    QString permatext;
    std::jthread worker;
    std::deque<std::pair<QString, uint>> queue;
    
    void update_routine(std::stop_token st)
    {
        int timeout{};
        bool perma_set{};
        
        while (true)
        {
            if (st.stop_requested()) return;
            
            if (timeout > 0)
            {
                timeout -= 100;
                
                // nice little animation
                auto txt = label->text();
                txt[0] = (timeout%500) ? ' ' : '>';
                txt[1] = (timeout%500) ? '>' : ' ';
                label->setText(txt);
            }
            else
            {
                if (!queue.empty())
                {
                    perma_set = false;
                    label->setText(queue.front().first);
                    timeout = queue.front().second;
                    queue.pop_front();
                }
                else if (not perma_set)
                {
                    perma_set = true;
                    label->setText(permatext);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
        }
    }
    
public:
    StatusText(QLabel* l) : label{l}, permatext{l->text()}, worker{&StatusText::update_routine, this} {}
    
    void set(QString text)
    {
        permatext = "   "+text;
    }
    
    void set(QString text, uint timeout_ms)
    {
        queue.emplace_back(">  "+text, timeout_ms);
    }
    
    ~StatusText()
    {
        worker.request_stop();
    }
};
