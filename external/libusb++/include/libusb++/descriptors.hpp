#pragma once

#include <cstdint>
#include <vector>

class endpoint_descriptor {
public:
  endpoint_descriptor(uint8_t bEndpointAddress, uint8_t bmAttributes,
                      uint16_t wMaxPacketSize, uint8_t bInterval,
                      uint8_t bRefresh, uint8_t bResynchAddress,
                      std::vector<uint8_t> &&extra)
      : bEndpointAddress(bEndpointAddress), bmAttributes(bmAttributes),
        wMaxPacketSize(wMaxPacketSize), bInterval(bInterval),
        bRefresh(bRefresh), bSynchAddress(bSynchAddress), extra(extra) {}

  const uint8_t bEndpointAddress;
  const uint8_t bmAttributes;
  const uint16_t wMaxPacketSize;
  const uint8_t bInterval;
  const uint8_t bRefresh;
  const uint8_t bSynchAddress;
  const std::vector<uint8_t> extra;
};
class interface_descriptor {
public:
  interface_descriptor(uint8_t bInterfaceNumber, uint8_t bAlternateSetting,
                       uint8_t bInterfaceClass, uint8_t bInterfaceSubClass,
                       uint8_t bInterfaceProtocol, uint8_t iInterface,
                       std::vector<endpoint_descriptor> &&endpoints,
                       std::vector<uint8_t> &&extra)
      : bInterfaceNumber(bInterfaceNumber),
        bAlternateSetting(bAlternateSetting), bInterfaceClass(bInterfaceClass),
        bInterfaceSubClass(bInterfaceSubClass),
        bInterfaceProtocol(bInterfaceProtocol), iInterface(iInterface),
        endpoints(endpoints), extra(extra) {}
  const uint8_t bInterfaceNumber;
  const uint8_t bAlternateSetting;
  const uint8_t bInterfaceClass;
  const uint8_t bInterfaceSubClass;
  const uint8_t bInterfaceProtocol;
  const uint8_t iInterface;
  const std::vector<endpoint_descriptor> endpoints;
  const std::vector<uint8_t> extra;
};
class config_descriptor {
public:
  config_descriptor(uint8_t bConfigurationValue, uint8_t iConfiguration,
                    uint8_t bmAttributes, uint8_t maxPower,
                    std::vector<interface_descriptor> &&interfaces,
                    std::vector<uint8_t> &&extra)
      : bConfigurationValue(bConfigurationValue),
        iConfiguration(iConfiguration), bmAttributes(bmAttributes),
        maxPower(maxPower), interfaces(interfaces), extra(extra) {}
  const uint8_t bConfigurationValue;
  const uint8_t iConfiguration;
  const uint8_t bmAttributes;
  const uint8_t maxPower;
  const std::vector<interface_descriptor> interfaces;
  const std::vector<uint8_t> extra;
};
