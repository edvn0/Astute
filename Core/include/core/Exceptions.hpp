#pragma once

#include <exception>
#include <format>
#include <stdexcept>
#include <string_view>

namespace Engine::Core {

class AstuteBaseException : public std::runtime_error
{
protected:
  explicit AstuteBaseException(const std::string_view data)
    : std::runtime_error(std::format("Astute Exception: {}", data))
  {
  }
};

class NoDeviceFoundException : public AstuteBaseException
{
public:
  explicit NoDeviceFoundException(const std::string_view data)
    : AstuteBaseException(data.data())
  {
  }
};

class CouldNotSelectPhysicalException : public AstuteBaseException
{
public:
  explicit CouldNotSelectPhysicalException(const std::string_view data)
    : AstuteBaseException(data.data())
  {
  }
};

class CouldNotCreateDeviceException : public AstuteBaseException
{
public:
  explicit CouldNotCreateDeviceException(const std::string_view data)
    : AstuteBaseException(data.data())
  {
  }
};

class InvalidInitialisationException : public AstuteBaseException
{
public:
  explicit InvalidInitialisationException(std::string_view reason)
    : AstuteBaseException(reason.data())
  {
  }
};

} // namespace Engine::Core
