#pragma once

extern coust::memory::unique_ptr<coust::Application, coust::DefaultAlloc>
    coust::create_application();

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
#if defined(COUST_TEST)
    const auto [test_cmd_count, test_cmds] = coust::parse_test_cmds(argc, argv);
    const auto [should_exit, test_result] =
        coust::run_tests(test_cmd_count, test_cmds);
    if (should_exit)
        return test_result;
#endif
    auto app = coust::create_application();
    app->run();
    return 0;
}
