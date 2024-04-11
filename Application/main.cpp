#include "AstuteCore.hpp"

#include <Lib/popl.hpp>

#include "AstuteApplication.hpp"

auto
main(int argc, char** argv) -> int
{
  using namespace Engine::Core;
  using namespace Engine::Graphics;
  // Create a parser object
  popl::OptionParser parser("CLI Parser for Configuration struct");

  // Add options for the struct members
  auto headless_opt =
    parser.add<popl::Switch>("m", "headless", "Run in headless [m]ode");
  auto help_opt =
    parser.add<popl::Switch>("h", "help", "Produce [h]elp message");
  Extent size{
    1600,
    900,
  };
  parser.add<popl::Value<u32>>(
    "d", "depth", "Window [d]epth (height)", 900, &size.height);
  parser.add<popl::Value<u32>>(
    "b", "breadth", "Window [b]readth (width)", 1600, &size.width);
  auto fullscreen_opt =
    parser.add<popl::Switch>("f", "fullscreen", "Begin in [f]ullscreen mode");

  // Parse the command-line arguments
  try {
    parser.parse(argc, argv);
    // print auto-generated help message
    if (help_opt->count() == 1)
      warn("{}", parser.help());
    else if (help_opt->count() == 2)
      warn("{}", parser.help(popl::Attribute::advanced));
    else if (help_opt->count() > 2)
      warn("{}", parser.help(popl::Attribute::expert));
  } catch (const popl::invalid_option& ex) {
    error("Parse error: {}", ex.what());
    return 1;
  }

  Application::Configuration config{
    .headless = headless_opt->value_or(false),
    .size = size,
    .fullscreen = fullscreen_opt->value_or(false),
  };

  AstuteApplication application{ config };
  return application.run();
}
