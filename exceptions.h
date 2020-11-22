#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <exception>
#include <string>

class LoaderException : public std::exception {
public:
    LoaderException(const std::string& str)
    : m_error(str)
    {}

    virtual ~LoaderException() = default;

    const char* error() const { return m_error.c_str(); }

protected:
    std::string m_error{};
};

class FailedOpen final : public LoaderException {
public:
    FailedOpen(const std::string& str)
    : LoaderException(str) {}
};

class BadFormat final : public LoaderException {
public:
    BadFormat(const std::string& str)
    : LoaderException(str) {}
};

class AllocFail final : public LoaderException {
public:
    AllocFail(const std::string& str)
    : LoaderException(str) {}
};

class ReadFail final : public LoaderException {
public:
    ReadFail(const std::string& str)
    : LoaderException(str) {}
};

#endif
