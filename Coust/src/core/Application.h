#pragma once

#include <memory>

namespace coust {

class Application {
public:
    Application() noexcept;
    virtual ~Application() noexcept;

    void run() noexcept;

private:
};

std::unique_ptr<Application> create_application();

}  // namespace coust
