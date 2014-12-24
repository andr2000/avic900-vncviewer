#include <android/log.h>
#include <errno.h>
#include <dlfcn.h>
#include <stdlib.h>

#include "brightness.h"

extern "C"
{
	#define MODULE_NAME "brightness"

	#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,  MODULE_NAME, __VA_ARGS__))
	#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__))
	#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__))
}

#define ANDROID_LIBHW_PATH "libhardware.so"
#define LIGHTS_HARDWARE_MODULE_ID "lights"
#define LIGHT_ID_BACKLIGHT "backlight"
#define LIGHT_INDEX_BACKLIGHT 0

struct hw_module_t;
struct hw_module_methods_t;

/**
 * Every hardware module must have a data structure named HAL_MODULE_INFO_SYM
 * and the fields of this data structure must begin with hw_module_t
 * followed by module specific information.
 */
typedef struct hw_module_t {
    /** tag must be initialized to HARDWARE_MODULE_TAG */
    uint32_t tag;

    /**
     * The API version of the implemented module. The module owner is
     * responsible for updating the version when a module interface has
     * changed.
     *
     * The derived modules such as gralloc and audio own and manage this field.
     * The module user must interpret the version field to decide whether or
     * not to inter-operate with the supplied module implementation.
     * For example, SurfaceFlinger is responsible for making sure that
     * it knows how to manage different versions of the gralloc-module API,
     * and AudioFlinger must know how to do the same for audio-module API.
     *
     * The module API version should include a major and a minor component.
     * For example, version 1.0 could be represented as 0x0100. This format
     * implies that versions 0x0100-0x01ff are all API-compatible.
     *
     * In the future, libhardware will expose a hw_get_module_version()
     * (or equivalent) function that will take minimum/maximum supported
     * versions as arguments and would be able to reject modules with
     * versions outside of the supplied range.
     */
    uint16_t module_api_version;
#define version_major module_api_version
    /**
     * version_major/version_minor defines are supplied here for temporary
     * source code compatibility. They will be removed in the next version.
     * ALL clients must convert to the new version format.
     */

    /**
     * The API version of the HAL module interface. This is meant to
     * version the hw_module_t, hw_module_methods_t, and hw_device_t
     * structures and definitions.
     *
     * The HAL interface owns this field. Module users/implementations
     * must NOT rely on this value for version information.
     *
     * Presently, 0 is the only valid value.
     */
    uint16_t hal_api_version;
#define version_minor hal_api_version

    /** Identifier of module */
    const char *id;

    /** Name of this module */
    const char *name;

    /** Author/owner/implementor of the module */
    const char *author;

    /** Modules methods */
    struct hw_module_methods_t* methods;

    /** module's dso */
    void* dso;

#ifdef __LP64__
    uint64_t reserved[32-7];
#else
    /** padding to 128 bytes, reserved for future use */
    uint32_t reserved[32-7];
#endif

} hw_module_t;

typedef struct hw_module_methods_t {
    /** Open a specific device */
    int (*open)(const struct hw_module_t* module, const char* id,
            struct hw_device_t** device);

} hw_module_methods_t;

typedef struct hw_device_t {
    /** tag must be initialized to HARDWARE_DEVICE_TAG */
    uint32_t tag;

    /**
     * Version of the module-specific device API. This value is used by
     * the derived-module user to manage different device implementations.
     *
     * The module user is responsible for checking the module_api_version
     * and device version fields to ensure that the user is capable of
     * communicating with the specific module implementation.
     *
     * One module can support multiple devices with different versions. This
     * can be useful when a device interface changes in an incompatible way
     * but it is still necessary to support older implementations at the same
     * time. One such example is the Camera 2.0 API.
     *
     * This field is interpreted by the module user and is ignored by the
     * HAL interface itself.
     */
    uint32_t version;

    /** reference to the module this device belongs to */
    struct hw_module_t* module;

    /** padding reserved for future use */
#ifdef __LP64__
    uint64_t reserved[12];
#else
    uint32_t reserved[12];
#endif

    /** Close this device */
    int (*close)(struct hw_device_t* device);

} hw_device_t;

struct light_state_t {
    /**
     * The color of the LED in ARGB.
     *
     * Do your best here.
     *   - If your light can only do red or green, if they ask for blue,
     *     you should do green.
     *   - If you can only do a brightness ramp, then use this formula:
     *      unsigned char brightness = ((77*((color>>16)&0x00ff))
     *              + (150*((color>>8)&0x00ff)) + (29*(color&0x00ff))) >> 8;
     *   - If you can only do on or off, 0 is off, anything else is on.
     *
     * The high byte should be ignored.  Callers will set it to 0xff (which
     * would correspond to 255 alpha).
     */
    unsigned int color;

    /**
     * See the LIGHT_FLASH_* constants
     */
    int flashMode;
    int flashOnMS;
    int flashOffMS;

    /**
     * Policy used by the framework to manage the light's brightness.
     * Currently the values are BRIGHTNESS_MODE_USER and BRIGHTNESS_MODE_SENSOR.
     */
    int brightnessMode;
};

struct light_device_t {
    struct hw_device_t common;

    /**
     * Set the provided lights to the provided values.
     *
     * Returns: 0 on succes, error code on failure.
     */
    int (*set_light)(struct light_device_t* dev,
            struct light_state_t const* state);
};

static class HWFunctions
{
public:
	constexpr HWFunctions() :
		fHWGetModule(nullptr),
		m_Initialized(false)
	{
	}

	typedef int (*pfnHWGetModule)(const char *id, const struct hw_module_t **module);
	pfnHWGetModule fHWGetModule;

	bool ensureInitialized()
	{
		if (m_Initialized)
		{
			return true;
		}

		void *handle = dlopen(ANDROID_LIBHW_PATH, RTLD_LAZY);
		if (!handle)
		{
			LOGE("Couldn't load %s", ANDROID_LIBHW_PATH);
			return false;
		}
		fHWGetModule = (pfnHWGetModule)dlsym(handle, "hw_get_module");
		if (!fHWGetModule)
		{
			LOGE("Failed to lookup hw_get_module function");
			return false;
		}
		m_Initialized = true;
		return true;
	}

private:
	bool m_Initialized;
} sHWFunctions;

Brightness::~Brightness()
{
	if (m_LightDevice)
	{
		hw_device_t *device = reinterpret_cast<hw_device_t *>(m_LightDevice);
		device->close(device);
	}
}

light_device_t *Brightness::getLightDevice(hw_module_t *module, char const *name)
{
	int err;
	hw_device_t *device;
	err = module->methods->open(module, name, &device);
	if (err == 0)
	{
		return reinterpret_cast<light_device_t *>(device);
	}
	LOGE("Failed open light device, error code %d", err);
	return nullptr;
}

bool Brightness::initialize()
{
	if (!sHWFunctions.ensureInitialized())
	{
		return false;
	}
	int err;
	hw_module_t *module;
	err = sHWFunctions.fHWGetModule(LIGHTS_HARDWARE_MODULE_ID, (hw_module_t const **)&module);
	if (err == 0)
	{
		LOGD("Opened HW module with id \"%s\", name \"%s\"", module->id, module->name);
		m_LightDevice = getLightDevice(module, LIGHT_ID_BACKLIGHT);
	}
	return m_LightDevice != nullptr;
}

bool Brightness::setBrightness(int value)
{
	if (m_LightDevice == nullptr)
	{
		return false;
	}
	light_state_t state = { 0 };
	state.color = value & 0xff;
	int err = m_LightDevice->set_light(m_LightDevice, &state);
	if (err < 0)
	{
		LOGE("Failed to set brightness level: %s", strerror(errno));
	}
	return err == 0;
}
