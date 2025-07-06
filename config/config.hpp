#ifndef CONFIG_HPP
#define CONFIG_HPP

#ifdef NDEBUG
constexpr bool enableValidationLayers = true;
#pragma message("NDEBUG is defined: Release build")
#else

constexpr bool enableValidationLayers = true;
#pragma message("NDEBUG NOT defined: Debug build")
#endif
#endif
