#include "libusb++/device_filter.hpp"

namespace usb {

void device_filter::bus(int expected) { bus_ = expected; }
void device_filter::device_address(int expected) { device_address_ = expected; }
void device_filter::vendor_id(uint16_t vid) { vendor_id_ = vid; }
void device_filter::product_id(uint16_t pid) { product_id_ = pid; }
void device_filter::interface_class(uint8_t expected) {
  interface_class_ = expected;
}
void device_filter::interface_subclass(uint8_t expected) {
  interface_subclass_ = expected;
}
void device_filter::interface_protocol(uint8_t expected) {
  interface_protocol_ = expected;
}

bool device_filter::test(const usb::device &device) const {
  auto dd = device.device_descriptor();
  bool bus_match = !bus_.has_value() || bus_.value() == device.bus_number();
  bool device_address_match =
      !device_address_.has_value() ||
      device_address_.value() == device.device_address();
  bool vendor_id_match =
      !vendor_id_.has_value() || vendor_id_.value() == dd.idVendor;
  bool product_id_match =
      !product_id_.has_value() || product_id_.value() == dd.idProduct;
  if (!(bus_match && device_address_match && vendor_id_match &&
        product_id_match))
    return false;

  auto config = device.configuration(0);
  auto any_interface_match = std::any_of(
      begin(config.interfaces), end(config.interfaces), [&](const auto &intf) {
        bool class_match = !interface_class_.has_value() ||
                           interface_class_.value() == intf.bInterfaceClass;
        bool subclass_match =
            !interface_subclass_.has_value() ||
            interface_subclass_.value() == intf.bInterfaceSubClass;
        bool protocol_match =
            !interface_protocol_.has_value() ||
            interface_protocol_.value() == intf.bInterfaceProtocol;
        return class_match && subclass_match && protocol_match;
      });
  return any_interface_match;
}

std::vector<device> device_filter::filter(const device_list &list) const {
  std::vector<usb::device> matches;
  std::copy_if(list.begin(), list.end(), std::back_inserter(matches),
               [&](const auto &dev) { return test(dev); });
  return matches;
}

} // namespace usb
