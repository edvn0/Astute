#include "AstuteApplication.hpp"

AstuteApplication::~AstuteApplication() = default;

AstuteApplication::AstuteApplication(Application::Configuration config)
  : Application(config){};

auto AstuteApplication::update(f64) -> void{};
auto AstuteApplication::interpolate(f64) -> void{};
