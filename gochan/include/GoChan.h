#ifndef GO_CHAN
#define GO_CHAN

#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>

template <typename T>
class GoChan {
private:
    std::queue<T> buffer;
    std::queue<std::shared_ptr<std::condition_variable>> sendQueue;
    std::queue<std::shared_ptr<std::condition_variable>> receiveQueue;
    mutable std::mutex mtx;
    std::condition_variable cv;
    const size_t capacity;
    bool closed = false;

public:
    explicit GoChan(size_t capacity = 0) : capacity(capacity) {}

    bool send(const T& data) {
        std::unique_lock<std::mutex> lock(mtx);
        if (closed)
            return false;

        if (!receiveQueue.empty()) {
            buffer.push(data);
            auto cv_recv = receiveQueue.front();
            receiveQueue.pop();
            cv_recv->notify_one();
            return true;
        }

        if (capacity == 0 || buffer.size() < capacity) {
            buffer.push(data);
            cv.notify_one();
            return true;
        }

        auto send_cv = std::make_shared<std::condition_variable>();
        sendQueue.push(send_cv);
        send_cv->wait(lock, [&]() { return !receiveQueue.empty() || buffer.size() < capacity || closed; });

        if (closed)
            return false;

        buffer.push(data);
        if (!receiveQueue.empty()) {
            auto cv_recv = receiveQueue.front();
            receiveQueue.pop();
            cv_recv->notify_one();
            return true;
        }

        cv.notify_one();
        return true;
    }

    std::optional<T> receive() {
        std::unique_lock<std::mutex> lock(mtx);

        auto sendFromQueue = [&]() -> std::optional<T> {
            auto cv_send = sendQueue.front();
            sendQueue.pop();
            cv_send->notify_one();
            cv.wait(lock, [&]() { return !buffer.empty() || closed; });
            if (closed && buffer.empty())
                return std::nullopt;
            T data = buffer.front();
            buffer.pop();
            return data;
        };

        if (!sendQueue.empty())
            return sendFromQueue();

        if (!buffer.empty()) {
            T data = buffer.front();
            buffer.pop();
            if (!sendQueue.empty()) {
                auto cv_send = sendQueue.front();
                sendQueue.pop();
                cv_send->notify_one();
            }
            return data;
        }

        if (closed)
            return std::nullopt;

        auto cv_recv = std::make_shared<std::condition_variable>();
        receiveQueue.push(cv_recv);
        cv_recv->wait(lock, [&]() { return closed || !sendQueue.empty() || !buffer.empty(); });

        if (closed && buffer.empty())
            return std::nullopt;

        if (!sendQueue.empty())
            return sendFromQueue();

        if (!buffer.empty()) {
            T data = buffer.front();
            buffer.pop();
            return data;
        }

        return std::nullopt;
    }

    void close() {
        std::unique_lock<std::mutex> lock(mtx);
        closed = true;
        while (!sendQueue.empty()) {
            sendQueue.front()->notify_one();
            sendQueue.pop();
        }
        while (!receiveQueue.empty()) {
            receiveQueue.front()->notify_one();
            receiveQueue.pop();
        }
        cv.notify_all();
    }

    [[nodiscard]] bool isClosed() const {
        std::unique_lock<std::mutex> lock(mtx);
        return closed;
    }

    ~GoChan() { close(); }
};

#endif  // GO_CHAN