#include "Threading.h"
#include "Logger.h"
#include <windows.h>
#include <sstream>

namespace Utils {

// WorkerThread implementation
WorkerThread::WorkerThread() = default;

WorkerThread::~WorkerThread() {
    Stop();
}

void WorkerThread::Start() {
    if (m_running) return;
    
    m_shouldStop = false;
    m_running = true;
    m_thread = std::thread(&WorkerThread::WorkerLoop, this);
}

void WorkerThread::Stop() {
    if (!m_running) return;
    
    m_shouldStop = true;
    
    // Wake up the worker thread
    PostTask([]() {});
    
    if (m_thread.joinable()) {
        m_thread.join();
    }
    
    m_running = false;
}

void WorkerThread::PostTask(Task task) {
    if (task) {
        m_taskQueue.Push(task);
    }
}

void WorkerThread::WorkerLoop() {
    Threading::SetCurrentThreadName("WorkerThread");
    
    while (!m_shouldStop) {
        Task task;
        if (m_taskQueue.WaitAndPop(task, std::chrono::milliseconds(100))) {
            try {
                task();
            }
            catch (const std::exception& e) {
                Logger::Error(std::string("Exception in worker thread task: ") + e.what());
            }
        }
    }
}

// Timer implementation
Timer::Timer() = default;

Timer::~Timer() {
    Stop();
}

void Timer::Start(std::chrono::milliseconds interval, std::function<void()> callback) {
    if (m_running) Stop();
    
    m_interval = interval;
    m_callback = callback;
    m_shouldStop = false;
    m_running = true;
    
    m_thread = std::thread(&Timer::TimerLoop, this);
}

void Timer::Stop() {
    if (!m_running) return;
    
    m_shouldStop = true;
    
    if (m_thread.joinable()) {
        m_thread.join();
    }
    
    m_running = false;
}

void Timer::TimerLoop() {
    Threading::SetCurrentThreadName("Timer");
    
    while (!m_shouldStop) {
        std::this_thread::sleep_for(m_interval);
        
        if (m_shouldStop) break;
        
        try {
            if (m_callback) {
                m_callback();
            }
        }
        catch (const std::exception& e) {
            Logger::Error(std::string("Exception in timer callback: ") + e.what());
        }
    }
}

// Threading utility functions
void Threading::SetCurrentThreadName(const std::string& name) {
    std::wstring wideName(name.begin(), name.end());
    SetThreadDescription(GetCurrentThread(), wideName.c_str());
}

std::string Threading::GetCurrentThreadId() {
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    return oss.str();
}

void Threading::Sleep(std::chrono::milliseconds duration) {
    std::this_thread::sleep_for(duration);
}

} // namespace Utils 
