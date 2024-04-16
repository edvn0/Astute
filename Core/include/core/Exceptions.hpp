#pragma once

#include <exception>
#include <format>
#include <stdexcept>
#include <string_view>

namespace Engine::Core {

class AstuteBaseException : public std::runtime_error
{
public:
  explicit AstuteBaseException(const std::string_view data)
    : std::runtime_error(std::format("Astute Exception: {}", data))
  {
  }
};

class NoDeviceFoundException : public AstuteBaseException
{
public:
  using AstuteBaseException::AstuteBaseException;
};

class CouldNotSelectPhysicalException : public AstuteBaseException
{
public:
  using AstuteBaseException::AstuteBaseException;
};

class CouldNotCreateDeviceException : public AstuteBaseException
{
public:
  using AstuteBaseException::AstuteBaseException;
};

class InvalidInitialisationException : public AstuteBaseException
{
public:
  using AstuteBaseException::AstuteBaseException;
};

class NotFoundInContainerException : public AstuteBaseException
{
public:
  using AstuteBaseException::AstuteBaseException;
};

class FileCouldNotBeOpened : public AstuteBaseException
{
public:
  using AstuteBaseException::AstuteBaseException;
};

} // namespace Engine::Core
