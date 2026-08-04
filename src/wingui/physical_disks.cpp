#include "PhysicalDisks.hpp"

#include <windows.h>
#include <winioctl.h>

#include <initguid.h>
#include <ntddstor.h> // GUID_DEVINTERFACE_DISK
#include <setupapi.h>

#include <cstring>
#include <vector>

namespace physical_disks {
namespace {

std::string trimmed(const char *data, size_t max_len) {
  std::string s(data, strnlen(data, max_len));
  size_t start = s.find_first_not_of(' ');
  if (start == std::string::npos) {
    return std::string();
  }
  size_t end = s.find_last_not_of(' ');
  return s.substr(start, end - start + 1);
}

std::string query_friendly_name(HANDLE device) {
  STORAGE_PROPERTY_QUERY query{};
  query.PropertyId = StorageDeviceProperty;
  query.QueryType = PropertyStandardQuery;

  BYTE buffer[1024]{};
  DWORD bytes_returned = 0;
  if (!DeviceIoControl(device, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), buffer, sizeof(buffer), &bytes_returned, nullptr)) {
    return std::string();
  }

  auto *descriptor = (STORAGE_DEVICE_DESCRIPTOR *)buffer;

  auto field_at = [&](DWORD offset) -> std::string {
    if (offset == 0 || offset >= sizeof(buffer)) {
      return std::string();
    }
    return trimmed((char *)buffer + offset, sizeof(buffer) - offset);
  };

  std::string vendor = field_at(descriptor->VendorIdOffset);
  std::string product = field_at(descriptor->ProductIdOffset);

  if (!vendor.empty() && !product.empty()) {
    return vendor + " " + product;
  }
  return vendor.empty() ? product : vendor;
}

// The device interface path SetupDi hands back isn't the \\.\PhysicalDriveN
// form the rest of the app (and libapfs's fopen-based BlockReader) expects
// -- recover the actual drive number via IOCTL_STORAGE_GET_DEVICE_NUMBER and
// rebuild that familiar path from it.
std::string physical_drive_path(HANDLE device) {
  STORAGE_DEVICE_NUMBER number{};
  DWORD bytes_returned = 0;
  if (!DeviceIoControl(device, IOCTL_STORAGE_GET_DEVICE_NUMBER, nullptr, 0, &number, sizeof(number), &bytes_returned, nullptr)) {
    return std::string();
  }
  return "\\\\.\\PhysicalDrive" + std::to_string(number.DeviceNumber);
}

} // namespace

std::vector<DriveInfo> enumerate() {
  std::vector<DriveInfo> drives;

  // Ask Windows directly for every disk device interface that's currently
  // present -- SetupDi's device-interface enumeration is the standard way
  // to do this without assuming a count or that drives are numbered
  // contiguously from 0.
  HDEVINFO dev_info =
    SetupDiGetClassDevsW(&GUID_DEVINTERFACE_DISK, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
  if (dev_info == INVALID_HANDLE_VALUE) {
    return drives;
  }

  SP_DEVICE_INTERFACE_DATA iface_data{};
  iface_data.cbSize = sizeof(iface_data);

  for (DWORD index = 0; SetupDiEnumDeviceInterfaces(dev_info, nullptr, &GUID_DEVINTERFACE_DISK, index, &iface_data); ++index) {
    DWORD required_size = 0;
    SetupDiGetDeviceInterfaceDetailW(dev_info, &iface_data, nullptr, 0, &required_size, nullptr);
    if (required_size == 0) {
      continue;
    }

    std::vector<BYTE> buffer(required_size);
    auto *detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)buffer.data();
    detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

    if (!SetupDiGetDeviceInterfaceDetailW(dev_info, &iface_data, detail, required_size, nullptr, nullptr)) {
      continue;
    }

    HANDLE device = CreateFileW(detail->DevicePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (device == INVALID_HANDLE_VALUE) {
      continue; // e.g. access denied, in use elsewhere -- skip but keep enumerating
    }

    const std::string path = physical_drive_path(device);
    if (path.empty()) {
      CloseHandle(device);
      continue;
    }

    DriveInfo info;
    info.path = path;
    info.friendly_name = query_friendly_name(device);
    if (info.friendly_name.empty()) {
      info.friendly_name = path.substr(path.find_last_of('\\') + 1);
    }

    CloseHandle(device);
    drives.push_back(std::move(info));
  }

  SetupDiDestroyDeviceInfoList(dev_info);

  return drives;
}

} // namespace physical_disks
