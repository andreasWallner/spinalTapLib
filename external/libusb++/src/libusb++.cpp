#include "libusb++/libusb++.hpp"

namespace usb {

device_list_iterator::device_list_iterator(const device_list &list,
                                           ssize_t offset)
    : list_(list), offset_(offset) {}

device device_list_iterator::operator*() const { return list_[offset_]; }
void device_list_iterator::operator++() { offset_++; };

bool device_list_iterator::operator==(const device_list_iterator &other) const {
  return &list_ == &other.list_ && offset_ == other.offset_;
}
bool device_list_iterator::operator!=(const device_list_iterator &other) const {
  return !(*this == other);
}

namespace {
libusb_context *init() {
  libusb_context *ctx;
  const auto status = libusb_init(&ctx);
  if (status != LIBUSB_SUCCESS)
    throw usb_error(static_cast<errors>(status));
  return ctx;
}
} // namespace

context::context() : context(init()) {}
context::context(libusb_context *ctx) : context_(ctx) {}

context::~context() { libusb_exit(context_); }

void context::set_log_level(log_level level) {
  const auto status = libusb_set_option(context_, LIBUSB_OPTION_LOG_LEVEL,
                                        static_cast<int>(level));
  if (status != LIBUSB_SUCCESS)
    throw usb_error(static_cast<errors>(status));
}

void context::set_usbdk(bool usbdk) {
  const auto status = libusb_set_option(context_, LIBUSB_OPTION_USE_USBDK,
                                        static_cast<int>(usbdk));
  if (status != LIBUSB_SUCCESS)
    throw usb_error(static_cast<errors>(status));
}

// hide type behind typedef of native handle
libusb_context *context::get() { return context_; }

context &context::default_context() {
  static context ctx{nullptr};
  return ctx;
}

} // namespace usb