#pragma once

#include "libusb++/details/libusb.hpp"
#include <stdexcept>
#include <system_error>

namespace usb {
enum class errors {
  SUCCESS = LIBUSB_SUCCESS,
  IO_ERROR = LIBUSB_ERROR_IO,
  INVALID_PARAM = LIBUSB_ERROR_INVALID_PARAM,
  ACCESS_DENIED = LIBUSB_ERROR_ACCESS,
  NO_DEVICE = LIBUSB_ERROR_NO_DEVICE,
  NOT_FOUND = LIBUSB_ERROR_NOT_FOUND,
  BUSY = LIBUSB_ERROR_BUSY,
  TIMEOUT = LIBUSB_ERROR_TIMEOUT,
  BUFFER_OVERFLOW = LIBUSB_ERROR_OVERFLOW,
  BROKEN_PIPE = LIBUSB_ERROR_PIPE,
  INTERRUPTED = LIBUSB_ERROR_INTERRUPTED,
  NO_MEM = LIBUSB_ERROR_NO_MEM,
  NOT_SUPPORTED = LIBUSB_ERROR_NOT_SUPPORTED,
  OTHER = LIBUSB_ERROR_OTHER,
};

class error_category : public std::error_category {
public:
  const char *name() const noexcept override { return "usb error"; }
  
  std::string message(int value) const override {
    return libusb_error_name(static_cast<libusb_error>(value));
  }

  std::error_condition
  default_error_condition(int value) const noexcept override {
    switch (static_cast<errors>(value)) {
    case errors::IO_ERROR:
      return std ::errc::io_error;
    case errors::INVALID_PARAM:
      return std::errc::invalid_argument;
    case errors::ACCESS_DENIED:
      return std::errc::permission_denied;
    case errors::BUSY:
      return std::errc::device_or_resource_busy;
    case errors::TIMEOUT:
      return std::errc::timed_out;
    case errors::BROKEN_PIPE:
      return std::errc::broken_pipe;
    case errors::INTERRUPTED:
      return std::errc::interrupted;
    case errors::NO_MEM:
      return std::errc::not_enough_memory;
    case errors::NOT_SUPPORTED:
      return std::errc::operation_not_supported;
    default:
      break;
    }
    std::error_condition(value, *this);
  }
};

inline std::error_category const &get_error_category() {
  static error_category singleton;
  return singleton;
}

inline std::error_code make_error_code(errors e) {
  return std::error_code(static_cast<int>(e), get_error_category());
}
inline std::error_condition make_error_condition(errors e) {
  return std::error_condition(static_cast<int>(e), get_error_category());
}

class usb_error : public std::exception {
public:
  usb_error(errors error) : error_{error} {}

  virtual const char *what() const noexcept override {
    return libusb_error_name(static_cast<libusb_error>(error_));
  }

  [[nodiscard]] std::error_code error_code() const noexcept {
    return make_error_code(error_);
  }

private:
  const errors error_;
};
} // namespace usb
