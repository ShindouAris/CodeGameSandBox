#ifndef CUSTOMEXEPTION_HPP
#define CUSTOMEXEPTION_HPP


#include <exception>
#include <string>

class ProblemNotAvailable : public std::exception {
  private:
    std::string message;

  public:
    explicit ProblemNotAvailable(const std::string& msg) : message(msg) {}

    const char* what() const noexcept override {
      return message.c_str();
    }
};

#endif
