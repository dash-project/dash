
#include <dash/dart/if/dart_config.h>
#include <dash/dart/if/dart_types.h>

dart_config_t dart_config_ = { 1 };

void dart_config(
  dart_config_t ** config_out)
{
  *config_out = &dart_config_;
}

