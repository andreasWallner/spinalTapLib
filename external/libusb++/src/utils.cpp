#include "libusb++/utils.hpp"

namespace usb {
namespace utils {

[[nodiscard]] std::vector<device>
filtered_devices(uint16_t vendor_id, uint16_t product_id, context &ctx) {
  const usb::device_list list{ctx};
  std::vector<device> matches;
  std::copy_if(list.begin(), list.end(), std::back_inserter(matches),
               [&](auto dev) {
                 auto dd = dev.device_descriptor();
                 return dd.idProduct == product_id && dd.idVendor == vendor_id;
               });
  return matches;
}

void print_short(const usb::device &d) {
  auto dd = d.device_descriptor();
  fmt::print("Bus {:03x} device {:03x}: ID {:04x}:{:04x}\n", d.bus_number(),
             d.device_address(), dd.idVendor, dd.idProduct);
}

void print_recursive(const usb::device &d) {
  std::optional<usb::device_handle> handle;
  try {
    handle.emplace(d);
  } catch (const usb::usb_error &e) {
    fmt::print(
        "  Couldn't open device, some information will be missing ({})\n",
        e.what());
  }
  auto dd{d.device_descriptor()};
  fmt::print("Bus {:03x} device {:03x}: ID {:04x}:{:04x}\n", d.bus_number(),
             d.device_address(), dd.idVendor, dd.idProduct);
  print(dd, "  ", handle);
  for (uint8_t confIdx = 0; confIdx < dd.bNumConfigurations; confIdx++) {
    try {
      auto c{d.configuration(confIdx)};
      fmt::print("  Configuration {}:\n", confIdx);
      print(c, "    ", handle);

      for (uint8_t intfIdx = 0; intfIdx < c.interfaces.size(); intfIdx++) {
        auto &i{c.interfaces[intfIdx]};
        fmt::print("    Interface {}:\n", intfIdx);
        print(i, "      ", handle);

        for (uint8_t epIdx = 0; epIdx < i.endpoints.size(); epIdx++) {
          fmt::print("      Endpoint {}:\n", epIdx);
          print(i.endpoints[epIdx], "        ");
        }
      }
    } catch (const usb::usb_error &e) {
      fmt::print("  Could not read configuration ({}), some information is "
                 "missing ({})\n",
                 confIdx, e.what());
    }
  }
}

void print(const libusb_device_descriptor &dd, const std::string_view prefix,
           const std::optional<device_handle> &handle) {
  fmt::print("{}bLength                {: >6}\n", prefix, dd.bLength);
  fmt::print("{}bDescriptorType        {: >6}\n", prefix, dd.bDescriptorType);
  fmt::print("{}bcdUSB                 {: >6}\n", prefix,
             dd.bcdUSB); // TODO: version dot
  fmt::print("{}bDeviceClass           {: >6}\n", prefix,
             dd.bDeviceClass); // TODO: decode
  fmt::print("{}bDeviceSubClass        {: >6}\n", prefix, dd.bDeviceSubClass);
  fmt::print("{}bDeviceProtocol        {: >6}\n", prefix, dd.bDeviceProtocol);
  fmt::print("{}bMaxPacketSize0        {: >6}\n", prefix, dd.bMaxPacketSize0);
  fmt::print("{}idVendor               {:#06x}\n", prefix, dd.idVendor);
  fmt::print("{}idProduct              {:#06x}\n", prefix, dd.idProduct);
  fmt::print("{}bcdDevice              {: >6}\n", prefix, dd.bcdDevice);
  fmt::print("{}iManufacturer          {: >6}\n", prefix, dd.iManufacturer);
  fmt::print("{}iProduct               {: >6} {}\n", prefix, dd.iProduct,
             (handle && dd.iProduct != 0)
                 ? handle->string_descriptor_ascii(dd.iProduct)
                 : "");
  fmt::print("{}iSerialNumber          {: >6} {}\n", prefix, dd.iSerialNumber,
             (handle && dd.iSerialNumber != 0)
                 ? handle->string_descriptor_ascii(dd.iSerialNumber)
                 : "");
  fmt::print("{}bNumConfigurations     {: >6}\n", prefix,
             dd.bNumConfigurations);
}
void print(const config_descriptor &cd, const std::string_view prefix,
           const std::optional<device_handle> &handle) {
  fmt::print("{}bConfigurationValue  {: >6}\n", prefix, cd.bConfigurationValue);
  fmt::print("{}iConfiguration       {: >6} {}\n", prefix, cd.iConfiguration,
             (handle && cd.iConfiguration != 0)
                 ? handle->string_descriptor_ascii(cd.iConfiguration)
                 : "");
  fmt::print("{}bmAttributes         {: >6}\n", prefix, cd.bmAttributes);
  fmt::print("{}maxPower             {: >6}\n", prefix, cd.maxPower);
}

void print(const interface_descriptor &id, const std::string_view prefix,
           const std::optional<device_handle> &handle) {
  fmt::print("{}bInterfaceNumber   {: >6}\n", prefix, id.bInterfaceNumber);
  fmt::print("{}bAlternateSetting  {: >6}\n", prefix, id.bAlternateSetting);
  fmt::print("{}bInterfaceClass    {:#06x}\n", prefix, id.bInterfaceClass);
  fmt::print("{}bInterfaceSubClass {:#06x}\n", prefix, id.bInterfaceSubClass);
  fmt::print("{}bInterfaceProtocol {:#06x}\n", prefix, id.bInterfaceProtocol);
  fmt::print("{}iInterface         {: >6} {}\n", prefix, id.iInterface,
             (handle && id.iInterface != 0)
                 ? handle->string_descriptor_ascii(id.iInterface)
                 : "");
}

void print(const endpoint_descriptor &ed, const std::string_view prefix) {
  fmt::print("{}bEndpointAddress {:#06x}\n", prefix, ed.bEndpointAddress);
  fmt::print("{}bmAttributes     {:#06x}\n", prefix, ed.bmAttributes);
  fmt::print("{}wMaxPacketSize   {: >6}\n", prefix, ed.wMaxPacketSize);
}
} // namespace utils
} // namespace usb