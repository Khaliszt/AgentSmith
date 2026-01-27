// C:\FarfadetsCorp\AgentSmith\include\core\result.h

#pragma once

#include <variant>
#include <string>
#include <optional>

namespace smith::core {

struct Error {
    enum class Code {
        None = 0,
        InvalidArgument,
        NotFound,
        AlreadyExists,
        PermissionDenied,
        Timeout,
        NetworkError,
        ParseError,
        IoError,
        SystemError,
        Unknown
    };

    Code code = Code::Unknown;
    std::string message;
    std::string details;
    std::string source;

    Error() = default;
    Error(Code c, std::string msg) : code(c), message(std::move(msg)) {}

    bool IsOk() const { return code == Code::None; }
    operator bool() const { return !IsOk(); }

    std::string ToString() const {
        std::string result = message;
        if (!details.empty()) result += " (" + details + ")";
        if (!source.empty()) result += " at " + source;
        return result;
    }
};

template<typename T, typename E = Error>
class Result {
public:
    Result(const T& value) : m_data(value) {}
    Result(T&& value) : m_data(std::move(value)) {}
    Result(const E& error) : m_data(error) {}
    Result(E&& error) : m_data(std::move(error)) {}

    bool IsOk() const { return std::holds_alternative<T>(m_data); }
    bool IsError() const { return std::holds_alternative<E>(m_data); }
    operator bool() const { return IsOk(); }

    T& Value() { return std::get<T>(m_data); }
    const T& Value() const { return std::get<T>(m_data); }
    T ValueOr(const T& defaultValue) const { return IsOk() ? Value() : defaultValue; }

    E& GetError() { return std::get<E>(m_data); }
    const E& GetError() const { return std::get<E>(m_data); }

    template<typename F>
    auto Map(F&& f) -> Result<decltype(f(std::declval<T>())), E> {
        if (IsOk()) return f(Value());
        return GetError();
    }

private:
    std::variant<T, E> m_data;
};

template<typename E>
class Result<void, E> {
public:
    Result() : m_error(std::nullopt) {}
    Result(const E& error) : m_error(error) {}

    bool IsOk() const { return !m_error.has_value(); }
    bool IsError() const { return m_error.has_value(); }
    operator bool() const { return IsOk(); }

    E& GetError() { return *m_error; }
    const E& GetError() const { return *m_error; }

private:
    std::optional<E> m_error;
};

template<typename T>
Result<T> Ok(T&& value) { return Result<T>(std::forward<T>(value)); }

inline Result<void> Ok() { return Result<void>(); }

template<typename T = void>
Result<T> Err(Error::Code code, const std::string& message) {
    return Result<T>(Error(code, message));
}

} // namespace smith::core
