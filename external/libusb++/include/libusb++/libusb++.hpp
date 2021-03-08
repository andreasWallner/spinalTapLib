#pragma once

#include "libusb++/descriptors.hpp"
#include "libusb++/error.hpp"
#include "libusb++/helper.hpp"
#include "libusb++/logging.hpp"

#include <chrono>
#include <gsl/span>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <fmt/core.h>

#define WIN32_LEAN_AND_MEAN
#define NOCOMM
#define NOMINMAX
#include <libusb-1.0/libusb.h>
#undef NOMINMAX
#undef NOCOMM
#undef WIN32_LEAN_AND_MEAN

namespace usb {

enum class type : uint8_t {
  standard = LIBUSB_REQUEST_TYPE_STANDARD,
  clazz = LIBUSB_REQUEST_TYPE_CLASS,
  vendor = LIBUSB_REQUEST_TYPE_VENDOR,
  reserved = LIBUSB_REQUEST_TYPE_RESERVED
};

enum class recipient : uint8_t {
  device = LIBUSB_RECIPIENT_DEVICE,
  interface = LIBUSB_RECIPIENT_INTERFACE,
  endpoint = LIBUSB_RECIPIENT_ENDPOINT,
  other = LIBUSB_RECIPIENT_OTHER
};

enum class log_cb_mode {
  global = LIBUSB_LOG_CB_GLOBAL,
  context = LIBUSB_LOG_CB_CONTEXT
};

enum class log_level {
  none = LIBUSB_LOG_LEVEL_NONE,
  error = LIBUSB_LOG_LEVEL_ERROR,
  warning = LIBUSB_LOG_LEVEL_WARNING,
  info = LIBUSB_LOG_LEVEL_INFO,
  debug = LIBUSB_LOG_LEVEL_DEBUG
};

class context {
public:
  context();
  ~context();
  context(const context &) = delete;
  context &operator=(const context &) = delete;

  // TODO: void set_log_cb(mode, );
  void set_log_level(log_level level);
  void set_usbdk(bool usbdk);
  libusb_context *get();

  static context &default_context();

private:
  explicit context(libusb_context *ctx);
  libusb_context *context_;
};

// TODO: get speed
class device {
  using native_handle_t = uintptr_t;

public:
  device(libusb_device *dev) : dev_(libusb_ref_device(dev)) {}
  device(const device &other) : dev_(libusb_ref_device(other.dev_)) {}
  device &operator=(const device &other) {
    if (this == &other)
      return *this;
    libusb_unref_device(dev_);
    dev_ = libusb_ref_device(other.dev_);
  }
  // device(device &&other) { std::swap(dev_, other.dev_); }
  ~device() { libusb_unref_device(dev_); };

  uint8_t bus_number() const noexcept { return libusb_get_bus_number(dev_); }
  uint8_t port_number() const noexcept { return libusb_get_port_number(dev_); }
  std::vector<uint8_t> port_numbers() const {
    // USB3 spec only allows for depth of 7
    std::vector<uint8_t> result{7, 0};
    const auto filled{
        libusb_get_port_numbers(dev_, result.data(), result.size())};
    if (filled < 0)
      throw usb_error(static_cast<errors>(filled));
    result.resize(filled);
    return result;
  }
  // get parent -> move to DeviceList?

  uint8_t device_address() const noexcept {
    return libusb_get_device_address(dev_);
  }
  libusb_speed device_speed() const noexcept {
    return static_cast<libusb_speed>(libusb_get_device_speed(dev_));
  }
  int max_packet_size(unsigned char endpoint) const noexcept {
    return libusb_get_max_packet_size(dev_, endpoint);
  }
  int max_iso_packet_size(unsigned char endpoint) const noexcept {
    return libusb_get_max_iso_packet_size(dev_, endpoint);
  }

  struct libusb_device_descriptor device_descriptor() const {
    struct libusb_device_descriptor descriptor;
    const int status{libusb_get_device_descriptor(dev_, &descriptor)};
    if (status < 0)
      throw usb_error(static_cast<errors>(status));
    return descriptor;
  }

  struct config_descriptor configuration(uint8_t config) const {
    struct libusb_config_descriptor *configuration;
    const int status{
        libusb_get_config_descriptor(dev_, config, &configuration)};
    if (status < 0)
      throw usb_error(static_cast<errors>(status));
    scope_exit descriptor_release{
        [&]() { libusb_free_config_descriptor(configuration); }};

    std::vector<interface_descriptor> interfaces;
    for (uint8_t intfIdx = 0; intfIdx < configuration->bNumInterfaces;
         intfIdx++) {
      const libusb_interface_descriptor *interface =
          configuration->interface[intfIdx].altsetting; // TODO parse all
      std::vector<endpoint_descriptor> endpoints;
      for (uint8_t epIdx = 0; epIdx < interface->bNumEndpoints; epIdx++) {
        const libusb_endpoint_descriptor *endpoint =
            interface->endpoint + epIdx;
        endpoints.emplace_back(
            endpoint->bEndpointAddress, endpoint->bmAttributes,
            endpoint->wMaxPacketSize, endpoint->bInterval, endpoint->bRefresh,
            endpoint->bSynchAddress,
            std::vector<uint8_t>(endpoint->extra,
                                 endpoint->extra + endpoint->extra_length));
      }
      interfaces.emplace_back(
          interface->bInterfaceNumber, interface->bAlternateSetting,
          interface->bInterfaceClass, interface->bInterfaceSubClass,
          interface->bInterfaceProtocol, interface->iInterface,
          std::move(endpoints),
          std::vector<uint8_t>(interface->extra,
                               interface->extra + interface->extra_length));
    }

    return config_descriptor(
        configuration->bConfigurationValue, configuration->iConfiguration,
        configuration->bmAttributes, configuration->MaxPower,
        std::move(interfaces),
        std::vector<uint8_t>(configuration->extra,
                             configuration->extra +
                                 configuration->extra_length));
  }

  native_handle_t native_handle() const noexcept { return (uintptr_t)dev_; }

private:
  libusb_device *dev_;
};

class device_handle {
public:
  using native_handle_t = uintptr_t;

  device_handle(const device &d) {
    const auto status{
        libusb_open((libusb_device *)d.native_handle(), &handle_)};
    if (status != 0)
      throw usb_error(static_cast<errors>(status));
  }
  // TODO: not use helper and do own implementation for better error output
  // TODO: overload/default param with context
  device_handle(uint16_t vendor_id, uint16_t product_id, usb::context &ctx)
      : handle_(
            libusb_open_device_with_vid_pid(ctx.get(), vendor_id, product_id)) {
    if (handle_ == nullptr)
      throw usb_error(errors::OTHER);
  }
  device_handle(const device_handle &other) = delete;
  device_handle &operator=(const device_handle &other) = delete;
  device_handle(device_handle &&other) : handle_(0) {
    std::swap(handle_, other.handle_);
  }
  ~device_handle() {
    if (handle_ != 0)
      libusb_close(handle_);
  }

  int configuration() const {
    int config;
    const auto status{libusb_get_configuration(handle_, &config)};
    if (status != 0)
      throw usb_error(static_cast<errors>(status));
    return config;
  }

  void reset() {
    const auto status{libusb_reset_device(handle_)};
    if (status != 0)
      throw usb_error(static_cast<errors>(status));
  }

  void set_configuration(int configuration) {
    const auto status{libusb_set_configuration(handle_, configuration)};
    if (status != 0)
      throw usb_error(static_cast<errors>(status));
  }

  [[nodiscard]] std::string string_descriptor(uint8_t index,
                                              uint16_t langid) const {
    // max length of string descriptor is 255 (single byte length field)
    std::vector<uint8_t> buffer{255, 0};
    const auto status{libusb_get_string_descriptor(
        handle_, index, langid, buffer.data(), buffer.size())};
    if (status < 0)
      throw usb_error(static_cast<errors>(status));
    return std::string(buffer.data(), buffer.data() + status);
  }

  [[nodiscard]] std::string string_descriptor_ascii(uint8_t index) const {
    // max length of string descriptor is 255 (single byte length field)
    std::vector<uint8_t> buffer(255, 0);
    const auto status{libusb_get_string_descriptor_ascii(
        handle_, index, buffer.data(), buffer.size())};
    if (status < 0)
      throw usb_error(static_cast<errors>(status));
    return std::string(buffer.data(), buffer.data() + status);
  }

  [[nodiscard]] native_handle_t native_handle() const noexcept {
    return (native_handle_t)handle_;
  }

private:
  libusb_device_handle *handle_;
};

class interface {
public:
  using native_handle_type = uintptr_t;

  interface(const device_handle &dh, int interface_number)
      : handle_((libusb_device_handle *)dh.native_handle()),
        interface_number_(interface_number) {
    const auto status{libusb_claim_interface(handle_, interface_number)};
    if (status != 0)
      throw usb_error(static_cast<errors>(status));
  }
  interface(const interface &other) = delete;
  interface &operator=(const interface &other) = delete;
  interface(interface &&other)
      : handle_(0),
        interface_number_(0) { // TODO: implement swap overload, use that
    std::swap(handle_, other.handle_);
    std::swap(interface_number_, other.interface_number_);
  }
  ~interface() {
    if (handle_ != 0)
      libusb_release_interface(handle_, interface_number_);
  }

  // TODO explicit release?

  void set_alt_setting(int alternate_setting) {
    const auto status{libusb_set_interface_alt_setting(
        handle_, interface_number_, alternate_setting)};
    if (status != 0)
      throw usb_error(static_cast<errors>(status));
  }

  int control_write(type type, recipient to, uint8_t request, uint16_t value,
                    uint16_t index, const gsl::span<uint8_t> data,
                    std::chrono::milliseconds timeout) {
    uint8_t bmRequestType = 0 | static_cast<uint8_t>(type);
    const auto status{libusb_control_transfer(
        handle_, bmRequestType, request, value, index, data.data(), data.size(),
        static_cast<unsigned int>(timeout.count()))};
    if (status < 0)
      throw usb_error(static_cast<errors>(status));
    return status;
  }

  int control_read(type type, recipient to, uint8_t request, uint16_t value,
                   uint16_t index, gsl::span<uint8_t> data,
                   std::chrono::milliseconds timeout) {
    uint8_t bmRequestType = 0x80 | static_cast<uint8_t>(type);
    const auto status{libusb_control_transfer(
        handle_, bmRequestType, request, value, index, data.data(), data.size(),
        static_cast<unsigned int>(timeout.count()))};
    if (status < 0)
      throw usb_error(static_cast<errors>(status));
    return status;
  }

  native_handle_type native_handle() const noexcept {
    return (native_handle_type)handle_;
  }

private:
  libusb_device_handle *handle_;
  int interface_number_;
};

// TODO: get max packet size
class endpoint {
public:
  endpoint(const interface &i, unsigned char ep)
      : handle_((libusb_device_handle *)i.native_handle()), endpoint_(ep) {}

protected:
  libusb_device_handle *handle_;
  unsigned char endpoint_;
};

class out_endpoint : public endpoint {
public:
  using endpoint::endpoint;
  int bulk_write(gsl::span<const uint8_t> data,
                 std::chrono::milliseconds timeout) {
    int transferred;
    // const_cast is fine since libusb does not alter data for out endpoints
    const auto status{libusb_bulk_transfer(
        handle_, endpoint_, const_cast<uint8_t *>(data.data()), data.size(),
        &transferred, static_cast<unsigned int>(timeout.count()))};
    if (status != 0)
      throw usb_error(static_cast<errors>(status));
    logging::logger->debug("TX: {:x}",
                           spdlog::to_hex(data.begin(), data.end()));
    return transferred;
  }

  void bulk_write_all(
      gsl::span<const uint8_t> data,
      std::chrono::milliseconds timeout = std::chrono::milliseconds(500)) {
    int remaining = data.size();
    do {
      const int written{bulk_write(data, timeout)};
      remaining -= written;
      data = data.subspan(written);
    } while (remaining);
  }
};

class in_endpoint : public endpoint {
public:
  in_endpoint(const interface &i, unsigned char ep) : endpoint(i, 0x80 | ep) {}
  int bulk_read(gsl::span<uint8_t> data, std::chrono::milliseconds timeout) {
    int transferred;
    const auto status{libusb_bulk_transfer(
        handle_, endpoint_, data.data(), data.size(), &transferred,
        static_cast<unsigned int>(timeout.count()))};
    if (status != 0)
      throw usb_error(static_cast<errors>(status));
    logging::logger->debug("RX: {:x}",
                           spdlog::to_hex(data.begin(), data.end()));
    return transferred;
  }

  void bulk_read_all(gsl::span<uint8_t> data,
                     std::chrono::milliseconds timeout) {
    int remaining = data.size();
    do {
      const int read{bulk_read(data, timeout)};
      remaining -= read;
      data = data.subspan(read);
    } while (remaining);
  }
};

class device_list;
class device_list_iterator {
public:
  device_list_iterator(const device_list &list, ssize_t offset);

  device operator*() const;
  void operator++();

  bool operator==(const device_list_iterator &other) const;
  bool operator!=(const device_list_iterator &other) const;

private:
  const device_list &list_;
  ssize_t offset_;
};

class device_list {
public:
  device_list(context &ctx) {
    listLen_ = libusb_get_device_list(ctx.get(), &list_);
    if (listLen_ < 0)
      throw usb_error(static_cast<errors>(listLen_));
  }
  device_list(const device_list &other) = delete;
  device_list &operator=(const device_list &other) = delete;
  ~device_list() { libusb_free_device_list(list_, 1); }

  device operator[](ssize_t offset) const { return device(list_[offset]); }

  device_list_iterator begin() const { return cbegin(); }
  device_list_iterator end() const { return cend(); }
  device_list_iterator cbegin() const { return device_list_iterator{*this, 0}; }
  device_list_iterator cend() const {
    // libusb_get_device_list will place a NULL entry
    // at the end of the array
    return device_list_iterator(*this, listLen_);
  }

private:
  libusb_device **list_;
  ssize_t listLen_;
};

} // namespace usb
