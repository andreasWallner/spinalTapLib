/*%
   ZTEX Core API for C with examples
   Copyright (C) 2009-2017 ZTEX GmbH.
   http://www.ztex.de

   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this file,
   You can obtain one at http://mozilla.org/MPL/2.0/.

   Alternatively, the contents of this file may be used under the terms
   of the GNU General Public License Version 3, as described below:

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 3 as
   published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see http://www.gnu.org/licenses/.
%*/

/** @file
memfifo example for C.
This example demonstrates the usage of the C API and the high speed interface of
the default firmware.

It performs certain tests (device and data integrity, read-write tests,
performance) using the memory FIFO as described on <a
href="http://wiki.ztex.de/doku.php?id=en:ztex_boards:ztex_fpga_boards:memfifo:memfifo">memfifo
example</a>.

The correct bitstream is detected and found automatically if the binary is
executed from the directory tree of the SDK. Otherwise it can be specified with
parameter '-s'.

Full list of options can be obtained with '-h'
@include memfifo.c
@cond memfifo
*/

#include "clipp.h"
#include "fmt/core.h"
#include "random.hpp"

#include <chrono>
#include <fcntl.h>
#include <libusb.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <vector>

#include "ztex.h"
#include <iostream>
#pragma comment(lib, "legacy_stdio_definitions.lib")

#define BULK_BUF_SIZE 4 * 1024 * 1024

// must be smaller then FIFO size, 64K on USB-FPGA Module 2.01
#define RW_SIZE 65000

static char *prog_name = NULL; // name of the program

static int readWriteTest(libusb_device_handle *handle, ztex_device_info *info);
static int readBandwidthTest(libusb_device_handle *handle,
                             ztex_device_info *info);
static int readValueTest(libusb_device_handle *handle, ztex_device_info *info);
static int loopbackTest(libusb_device_handle *handle, ztex_device_info *info);

int main(int argc, char **argv) {
  int id_vendor = 0x221A; // ZTEX vendor ID
  int id_product = 0x100; // default product ID for ZTEX firmware
  int status = 0;
  libusb_device **devs = NULL;
  int print_all = 0, print = 0, print_info = 0, reset_dev = 0;
  int busnum = -1, devnum = -1;
  std::string sn_string, product_string,
      bitstream_path = "../../examples/memfifo";
  libusb_device_handle *handle = NULL;
  ztex_device_info info;
  std::string bitstream_fn;
  char sbuf[8192];
  bool test_write = false, test_read_bandwidth = false, test_loopback = false,
       test_read_data = false;

  auto cli =
      (clipp::option("-p").set(print, 1).doc("Print matching USB devices"),
       clipp::option("-pa").set(print_all, 1).doc("Print all USB devices"),
       clipp::option("-i").set(print_info, 1).doc("Print device info"),
       clipp::option("-r")
           .set(reset_dev, 1)
           .doc("Reset device (default: reset configuration only)"),
       clipp::option("-fu").doc("Select device by USB IDs, default: "
                                "0x221A 0x100, 0:ignore ID") &
           clipp::value("vendor", id_vendor) &
           clipp::value("product", id_product),
       clipp::option("-fd").doc(
           "Select device by bus number and device address") &
           clipp::value("bus", busnum) & clipp::value("dev", devnum),
       clipp::option("-fs").doc("Select device by serial number string") &
           clipp::value("serialnumber", sn_string),
       clipp::option("-fp").doc("Select device by product string") &
           clipp::value("product_string", product_string),
       clipp::option("-s").doc("Additional search path for bitstream") &
           clipp::value(bitstream_path),
       clipp::required("-b").doc("Bitstream path") &
           clipp::value("path", bitstream_fn),
       clipp::option("-tw").set(test_write),
       clipp::option("-trb").set(test_read_bandwidth),
       clipp::option("-tlb").set(test_loopback),
       clipp::option("-trd").set(test_read_data));

  auto result = clipp::parse(argc, argv, cli);

  if (result.any_error()) {
    std::cout << "Usage:\n"
              << clipp::usage_lines(cli, "progname") << "\nOptions:\n"
              << clipp::documentation(cli) << '\n';
    exit(-1);
  }

  // INIT libusb,
  status = libusb_init(NULL);
  if (status < 0) {
    fprintf(stderr, "Error: Unable to init libusb: %s\n",
            libusb_error_name(status));
    return 1;
  }
  libusb_set_option(nullptr, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);

  // find all USB devices
  status = libusb_get_device_list(NULL, &devs);
  if (status < 0) {
    fprintf(stderr, "Error: Unable to get device list: %s\n",
            libusb_error_name(status));
    goto err;
  }

  // print bus info or find device
  int dev_idx = ztex_scan_bus(
      sbuf, sizeof(sbuf), devs, print_all ? -1 : print ? 1 : 0, id_vendor,
      id_product, busnum, devnum,
      sn_string.length() == 0 ? nullptr : sn_string.c_str(),
      product_string.length() == 0 ? nullptr : product_string.c_str());
  printf(sbuf);
  fflush(stdout);
  if (print || print_all) {
    status = 0;
    goto noerr;
  } else if (dev_idx < 0) {
    if (dev_idx == -1)
      fprintf(stderr, "Error: No device found\n");
    goto err;
  }

  // open device
  status = libusb_open(devs[dev_idx], &handle);
  if (status < 0) {
    fprintf(stderr, "Error: Unable to open device: %s\n",
            libusb_error_name(status));
    goto err;
  }
  libusb_free_device_list(devs, 1);
  devs = NULL;

  // reset configuration or device
  if (!reset_dev) {
    status = libusb_set_configuration(handle, -1);
    if (status < 0) {
      fprintf(stderr,
              "Warning: Unable to unconfigure device: %s, trying to reset it\n",
              libusb_error_name(status));
#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
      fprintf(stderr, "Due to limitations of Windows neither this nor device "
                      "reset works. This may cause further errors ...\n");
#endif
      reset_dev = 1;
    }
  }
  if (reset_dev) {
    status = libusb_reset_device(handle);
    if (status < 0) {
      fprintf(stderr, "Error: Unable to reset device: %s\n",
              libusb_error_name(status));
      goto err;
    }
  }
  status = libusb_set_configuration(handle, 1);
  if (status < 0)
    fprintf(stderr, "Warning: Unable to set configuration 1: %s\n",
            libusb_error_name(status));
  fflush(stderr);

  // get and print device info
  status = ztex_get_device_info(handle, &info);
  if (status < 0) {
    fprintf(stderr, "Error: Unable to get device info: %s\n",
            libusb_error_name(status));
    goto err;
  }
  if (print_info) {
    ztex_print_device_info(sbuf, sizeof(sbuf), &info);
    printf(sbuf);
    status = ztex_get_fpga_config(handle);
    if (status < 0) {
      fprintf(stderr, "Error: Unable to get FPGA configuration state: %s\n",
              libusb_error_name(status));
      goto err;
    }
    printf("FPGA: %s\n", status == 0 ? "unconfigured" : "configured");
    status = 0;
    goto noerr;
  }

  // read and upload bitstream
  FILE *fd = fopen(bitstream_fn.c_str(), "rb");
  if (fd == NULL) {
    fprintf(stderr, "Warning: Error opening file '%s'\n", bitstream_fn.c_str());
    goto nobitstream;
  }
  status = ztex_upload_bitstream(sbuf, sizeof(sbuf), handle, &info, fd, -1);
  fclose(fd);
  fprintf(stderr, sbuf);

nobitstream:
  fflush(stderr);
  // check config
  status = ztex_get_fpga_config(handle);
  if (status < 0) {
    fprintf(stderr, "Error: Unable to get FPGA configuration state: %s\n",
            libusb_error_name(status));
    goto err;
  } else if (status == 0) {
    fprintf(stderr, "Error: FPGA not configured\n");
    goto err;
  }

  // claim interface
  status = libusb_claim_interface(handle, 0);
  if (status < 0) {
    fprintf(stderr, "Error claiming interface 0: %s\n",
            libusb_error_name(status));
    goto err;
  }

#if 0
    status = readWriteTest(handle, &info);
    if (status < 0)
      goto err;
  status = readBandwidthTest(handle, &info);
  if (status < 0)
    goto err;
#endif
  if (test_loopback) {
    status = loopbackTest(handle, &info);
  } else if (test_read_data) {
    status = readValueTest(handle, &info);
  }

  // release resources
  status = 0;
  goto noerr;
err:
  status = 1;
noerr:
  if (handle) {
    libusb_release_interface(handle, 0);
    libusb_close(handle);
  }
  if (devs)
    libusb_free_device_list(devs, 1);
  libusb_exit(NULL);
  return status;
}

static int readWriteTest(libusb_device_handle *handle, ztex_device_info *info) {
  std::vector<uint8_t> mbuf(BULK_BUF_SIZE, 0);
  int transferred;
  int status;

  // verify mode and prepare device if necessary
  if (ztex_default_gpio_ctl(handle, 0, 0)) {
    fprintf(stderr, "Warning: wrong initial mode, switching to mode 0\n");
    ztex_default_gpio_ctl(handle, 7, 0);
    ztex_default_reset(handle, 0);
    libusb_bulk_transfer(handle, info->default_in_ep, mbuf.data(), mbuf.size(),
                         &transferred, 250);
  } else {
    status = libusb_bulk_transfer(handle, info->default_in_ep, mbuf.data(),
                                  mbuf.size(), &transferred, 250);
    if ((status >= 0) && (transferred > 0))
      fprintf(stderr, "Warning: found %d bytes in EZ-USB FIFO\n", transferred);
  }
  fflush(stderr);

  // test 1: read-write test (mode 0)
  for (int i = 0; i < RW_SIZE; i += 2) {
    mbuf[i] = (i >> 1) & 127;
    mbuf[i + 1] = 128 | ((i >> 8) & 127);
  }
  TWO_TRIES(status,
            libusb_bulk_transfer(handle, info->default_out_ep, mbuf.data(),
                                 RW_SIZE, &transferred, 2000));
  if (status < 0) {
    fprintf(stderr, "Bulk write error: %s\n", libusb_error_name(status));
    return -1;
  }
  printf("Read-write test, short packet test: wrote %d Bytes\n", transferred);
  fflush(stdout);

  TWO_TRIES(status,
            libusb_bulk_transfer(handle, info->default_in_ep, mbuf.data(),
                                 mbuf.size(), &transferred, 4000));
  if (status < 0) {
    fprintf(stderr, "Bulk read error: %s\n", libusb_error_name(status));
    return -1;
  }
  {
    int i = mbuf[0] >> 7;
    int j = mbuf[i] | ((mbuf[i + 1] & 127) << 7);
    printf("Read-write test: read (%d=%d*512+%d) Bytes.  %d leading Bytes lost",
           transferred, transferred / 512, transferred & 511, j * 2);
    if (j)
      printf("(This may be platform specific)");
    int size = 0;
    for (i = i + 2; i + 1 < transferred; i += 2) {
      int k = mbuf[i] | ((mbuf[i + 1] & 127) << 7);
      if (k != ((j + 1) & 0x3fff))
        size += 1;
      j = k;
    }
    printf(". %d data errors.  %d Bytes remaining in FIFO due to memory "
           "transfer granularity\n",
           size, RW_SIZE - transferred);
    fflush(stdout);
  }
  return 0;
}

static int readBandwidthTest(libusb_device_handle *handle,
                             ztex_device_info *info) {
  std::vector<uint8_t> mbuf(BULK_BUF_SIZE, 0);
  int transferred;

  // test 2: read rate test using test data generator (mode 1)
  // reset application and set mode 1
  ztex_default_reset(handle, 0);
  int status = ztex_default_gpio_ctl(handle, 7, 1);
  if (status < 0) {
    fprintf(stderr, "Error setting GPIO's: %s\n", libusb_error_name(status));
    return -1;
  }

  // read data and measure time, first packets are ignored because EZ-USB buffer
  // may be filled
  printf("Measuring read rate using libusb_bulk_transfer ... \n");
  fflush(stdout);
  int size = 0;
  std::chrono::high_resolution_clock::time_point start;
  for (int i = 0; i < 55; i++) {
    if (i == 5)
      start = std::chrono::high_resolution_clock::now();
    TWO_TRIES(status,
              libusb_bulk_transfer(handle, info->default_in_ep, mbuf.data(),
                                   mbuf.size(), &transferred, 2000));
    if (status < 0) {
      fprintf(stderr, "Bulk read error: %s\n", libusb_error_name(status));
      return -1;
    }
    if ((i == 0) && ((mbuf[0] != 0) && (mbuf[1] != 239))) {
      fprintf(
          stderr,
          "Warning: Invalid start of data: %d %d, leading data may went lost\n",
          mbuf[0], mbuf[1]);
    }
    if (i >= 5)
      size += transferred;
  }
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<float, std::micro> usecs = end - start;
  printf("Read %.1f MB at %.1f MB/s\n", size / (1024.0 * 1024.0),
         size / usecs.count());
  fflush(stdout);
  return 0;
}

static int loopbackTest(libusb_device_handle *handle, ztex_device_info *info) {
  ztex_default_reset(handle, 0);
  int status = ztex_default_gpio_ctl(handle, 7, 0);
  if (status < 0) {
    fprintf(stderr, "Error setting GPIO's: %s\n", libusb_error_name(status));
    return -1;
  }

  std::vector<uint8_t> txbuf(1024, 0);
  xorshift16 xs;
  std::generate(begin(txbuf), end(txbuf), [&]() { return xs(); });

  int transferred;
  TWO_TRIES(status,
            libusb_bulk_transfer(handle, info->default_out_ep, txbuf.data(),
                                 txbuf.size(), &transferred, 4000));
  if (status < 0) {
    fprintf(stderr, "Bulk write error: %s\n", libusb_error_name(status));
    return -1;
  }

  fmt::print("sent {}\n", transferred);

  std::vector<uint8_t> rxbuf(BULK_BUF_SIZE, 0);
  int received;
  TWO_TRIES(status,
            libusb_bulk_transfer(handle, info->default_in_ep, rxbuf.data(),
                                 rxbuf.size(), &received, 4000));
  if (status < 0) {
    fprintf(stderr, "Bulk read error: %s\n", libusb_error_name(status));
    return -1;
  }

  fmt::print("received {}\n", received);

  bool error = false;
  for (size_t i = 0; i < min(received, transferred); i++) {
    if (txbuf[i] != rxbuf[i]) {
      fmt::print("{}: {:04x} != {:04x}\n", i, txbuf[i], rxbuf[i]);
      error = true;
    }
  }
  if (!error)
    fmt::print("DONE: OK\n");
}

static int readValueTest(libusb_device_handle *handle, ztex_device_info *info) {
  std::vector<uint8_t> mbuf(BULK_BUF_SIZE, 0);
  int transferred;

  ztex_default_reset(handle, 0);
  int status = ztex_default_gpio_ctl(handle, 7, 1);
  if (status < 0) {
    fprintf(stderr, "Error setting GPIO's: %s\n", libusb_error_name(status));
    return -1;
  }

  int size{0}; // seed so that first result is 1, matching HW
  int errors{0};
  xorshift16 xs{0xc181};
  while (size < 2000000) {
    TWO_TRIES(status,
              libusb_bulk_transfer(handle, info->default_in_ep, mbuf.data(),
                                   mbuf.size(), &transferred, 2000));
    if (status < 0) {
      fprintf(stderr, "Bulk read error: %s\n", libusb_error_name(status));
      return -1;
    }
    fmt::print("transferred: {}\n", transferred);
    if (transferred % 2)
      return -1;
    uint16_t *mbuf16 = reinterpret_cast<uint16_t *>(mbuf.data());
    while (transferred >= 2) {
      size++;
      uint16_t expected = xs();
      if (*mbuf16 != expected) {
        fmt::print("{}: 0x{:04x} != 0x{:04x}\n", size, *mbuf16, expected);
        if (errors++ > 20)
          return -1;
      } else if (size < 20) {
        fmt::print("{}: 0x{:04x} == 0x{:04x}\n", size, *mbuf16, expected);
      }
      mbuf16++;
      transferred -= 2;
    }
  }

  return 0;
}
