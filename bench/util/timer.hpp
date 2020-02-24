struct timer final
{
    timer() : start{ std::chrono::system_clock::now() } {}

    double elapsed()
    {
        const auto now = std::chrono::system_clock::now();
        return std::chrono::duration<double>(now - start).count();
    }

private:
    std::chrono::time_point<std::chrono::system_clock> start;
};