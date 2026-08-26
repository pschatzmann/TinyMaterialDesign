#pragma once

/// Default pixel format used by the RGB565-suffixed convenience aliases
/// throughout this library. Every widget/theme class is still a template
/// over RGB_T, so RGB666/RGB888 work too - just less conveniently named.
#ifndef TINYMD_DEFAULT_RGB_T
#define TINYMD_DEFAULT_RGB_T tinygpu::RGB565
#endif
