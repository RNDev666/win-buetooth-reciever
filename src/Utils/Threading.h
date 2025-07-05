#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <queue>
#include <future>

namespace Utils {

// Simple thread-safe queue
template<typename T>
class ThreadSafeQueue {
public:
    void Push(const T& item) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(item);
        m_condition.notify_one();
    }
    
    bool TryPop(T& item) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) return false;
        
        item = m_queue.front();
        m_queue.pop();
        return true;
    }
    
    bool WaitAndPop(T& item, std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_condition.wait_for(lock, timeout, [this] { return !m_queue.empty(); })) {
            item = m_queue.front();
            m_queue.pop();
            return true;
        }
        return false;
    }
    
    bool Empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }
    
    size_t Size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }
    
private:
    mutable std::mutex m_mutex;
    std::queue<T> m_queue;
    std::condition_variable m_condition;
};

// Worker thread for background tasks
class WorkerThread {
public:
    using Task = std::function<void()>;
    
    WorkerThread();
    ~WorkerThread();
    
    void Start();
    void Stop();
    void PostTask(Task task);
    
    // Async task with future return
    template<typename F, typename... Args>
    auto PostTaskAsync(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using ReturnType = std::invoke_result_t<F, Args...>;
        
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<ReturnType> result = task->get_future();
        PostTask([task]() { (*task)(); });
        
        return result;
    }
    
    bool IsRunning() const { return m_running; }
    
private:
    void WorkerLoop();
    
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_shouldStop{false};
    std::thread m_thread;
    ThreadSafeQueue<Task> m_taskQueue;
};

// Simple timer class
class Timer {
public:
    Timer();
    ~Timer();
    
    void Start(std::chrono::milliseconds interval, std::function<void()> callback);
    void Stop();
    
    bool IsRunning() const { return m_running; }
    
private:
    void TimerLoop();
    
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_shouldStop{false};
    std::thread m_thread;
    std::chrono::milliseconds m_interval{1000};
    std::function<void()> m_callback;
};

// Threading utilities
class Threading {
public:
    // Set thread name for debugging
    static void SetCurrentThreadName(const std::string& name);
    
    // Get current thread ID as string
    static std::string GetCurrentThreadId();
    
    // Sleep for specified duration
    static void Sleep(std::chrono::milliseconds duration);
};

} // namespace Utils 
