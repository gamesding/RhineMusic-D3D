#pragma once
// Rhine-owned C ABI. No vendor SDK declarations or binary layouts are exposed.
#include <stdint.h>
#if defined(RHINE_PAD_BRIDGE_BUILD)
#define RHINE_PAD_EXPORT __declspec(dllexport)
#else
#define RHINE_PAD_EXPORT
#endif
#ifdef __cplusplus
extern "C" {
#endif
enum { RHINE_PAD_API_VERSION = 1 };
typedef struct RhinePadSample {
    uint32_t byteSize;
    uint32_t connected;
    uint32_t generation; // 4 or 5; zero when disconnected.
    uint32_t buttons;    // Rhine input masks, shared with the XInput adapter.
    float leftX, leftY, rightX, rightY; // [-1, 1], positive Y points down.
    uint32_t touchPressed, touchCount, touchWidth;
    float touchAverageX; // [0, 1], valid when touchCount > 0.
    int32_t singleTouchId;
    float singleTouchX, singleTouchY;
    uint32_t diagnosticButtons;
    uint32_t diagnosticAxes[4];
} RhinePadSample;
RHINE_PAD_EXPORT uint32_t rhine_pad_api_version(void);
RHINE_PAD_EXPORT int32_t rhine_pad_initialize(void);
RHINE_PAD_EXPORT void rhine_pad_shutdown(void);
RHINE_PAD_EXPORT int32_t rhine_pad_open(uint32_t slot);
RHINE_PAD_EXPORT void rhine_pad_close(int32_t handle);
RHINE_PAD_EXPORT int32_t rhine_pad_read(int32_t handle, RhinePadSample* sample, uint32_t byteSize);
RHINE_PAD_EXPORT int32_t rhine_pad_vibrate(int32_t handle, uint8_t strongMotor, uint8_t weakMotor);
#ifdef __cplusplus
}
static_assert(sizeof(RhinePadSample) == 80, "Rhine Pad ABI must remain stable");
#endif
