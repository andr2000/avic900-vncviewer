#include <android/log.h>
#include <dlfcn.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <stdlib.h>

#include "AndroidGraphicBuffer.h"

#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "AndroidGraphicBuffer" , ## args)

#define EGL_NATIVE_BUFFER_ANDROID 0x3140
#define EGL_IMAGE_PRESERVED_KHR   0x30D2

typedef void *EGLContext;
typedef void *EGLDisplay;
typedef uint32_t EGLenum;
typedef int32_t EGLint;
typedef uint32_t EGLBoolean;

#define EGL_TRUE 1
#define EGL_FALSE 0
#define EGL_NONE 0x3038
#define EGL_NO_CONTEXT (EGLContext)0
#define EGL_DEFAULT_DISPLAY  (void*)0

#define ANDROID_LIBUI_PATH "libui.so"
#define ANDROID_GLES_PATH "libGLESv2.so"
#define ANDROID_EGL_PATH "libEGL.so"

// Really I have no idea, but this should be big enough
#define GRAPHIC_BUFFER_SIZE 1024

enum
{
	/* buffer is never read in software */
	GRALLOC_USAGE_SW_READ_NEVER   = 0x00000000,
	/* buffer is rarely read in software */
	GRALLOC_USAGE_SW_READ_RARELY  = 0x00000002,
	/* buffer is often read in software */
	GRALLOC_USAGE_SW_READ_OFTEN   = 0x00000003,
	/* mask for the software read values */
	GRALLOC_USAGE_SW_READ_MASK    = 0x0000000F,

	/* buffer is never written in software */
	GRALLOC_USAGE_SW_WRITE_NEVER  = 0x00000000,
	/* buffer is never written in software */
	GRALLOC_USAGE_SW_WRITE_RARELY = 0x00000020,
	/* buffer is never written in software */
	GRALLOC_USAGE_SW_WRITE_OFTEN  = 0x00000030,
	/* mask for the software write values */
	GRALLOC_USAGE_SW_WRITE_MASK   = 0x000000F0,

	/* buffer will be used as an OpenGL ES texture */
	GRALLOC_USAGE_HW_TEXTURE      = 0x00000100,
	/* buffer will be used as an OpenGL ES render target */
	GRALLOC_USAGE_HW_RENDER       = 0x00000200,
	/* buffer will be used by the 2D hardware blitter */
	GRALLOC_USAGE_HW_2D           = 0x00000400,
	/* buffer will be used with the framebuffer device */
	GRALLOC_USAGE_HW_FB           = 0x00001000,
	/* mask for the software usage bit-mask */
	GRALLOC_USAGE_HW_MASK         = 0x00001F00,
};

enum
{
	HAL_PIXEL_FORMAT_RGBA_8888          = 1,
	HAL_PIXEL_FORMAT_RGBX_8888          = 2,
	HAL_PIXEL_FORMAT_RGB_888            = 3,
	HAL_PIXEL_FORMAT_RGB_565            = 4,
	HAL_PIXEL_FORMAT_BGRA_8888          = 5,
	HAL_PIXEL_FORMAT_RGBA_5551          = 6,
	HAL_PIXEL_FORMAT_RGBA_4444          = 7,
};

static bool gTryRealloc = true;

struct ARect
{
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;
};

static class GLFunctions
{
public:
	constexpr GLFunctions() : fGetDisplay(nullptr),
		fEGLGetError(nullptr),
		fCreateImageKHR(nullptr),
		fDestroyImageKHR(nullptr),
		fImageTargetTexture2DOES(nullptr),
		fBindTexture(nullptr),
		fGLGetError(nullptr),
		fGraphicBufferCtor(nullptr),
		fGraphicBufferDtor(nullptr),
		fGraphicBufferLock(nullptr),
		fGraphicBufferLockRect(nullptr),
		fGraphicBufferUnlock(nullptr),
		fGraphicBufferGetNativeBuffer(nullptr),
		fGraphicBufferReallocate(nullptr),
		m_Initialized(false)
	{
	}

	typedef EGLDisplay (* pfnGetDisplay)(void *display_id);
	pfnGetDisplay fGetDisplay;
	typedef EGLint (* pfnEGLGetError)(void);
	pfnEGLGetError fEGLGetError;

	typedef EGLImageKHR (* pfnCreateImageKHR)(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list);
	pfnCreateImageKHR fCreateImageKHR;
	typedef EGLBoolean (* pfnDestroyImageKHR)(EGLDisplay dpy, EGLImageKHR image);
	pfnDestroyImageKHR fDestroyImageKHR;

	typedef void (* pfnImageTargetTexture2DOES)(GLenum target, EGLImageKHR image);
	pfnImageTargetTexture2DOES fImageTargetTexture2DOES;

	typedef void (* pfnBindTexture)(GLenum target, GLuint texture);
	pfnBindTexture fBindTexture;

	typedef GLenum (* pfnGLGetError)();
	pfnGLGetError fGLGetError;

	typedef void (*pfnGraphicBufferCtor)(void*, uint32_t w, uint32_t h, uint32_t format, uint32_t usage);
	pfnGraphicBufferCtor fGraphicBufferCtor;

	typedef void (*pfnGraphicBufferDtor)(void*);
	pfnGraphicBufferDtor fGraphicBufferDtor;

	typedef int (*pfnGraphicBufferLock)(void*, uint32_t usage, unsigned char **addr);
	pfnGraphicBufferLock fGraphicBufferLock;

	typedef int (*pfnGraphicBufferLockRect)(void*, uint32_t usage, const ARect&, unsigned char **addr);
	pfnGraphicBufferLockRect fGraphicBufferLockRect;

	typedef int (*pfnGraphicBufferUnlock)(void*);
	pfnGraphicBufferUnlock fGraphicBufferUnlock;

	typedef void* (*pfnGraphicBufferGetNativeBuffer)(void*);
	pfnGraphicBufferGetNativeBuffer fGraphicBufferGetNativeBuffer;

	typedef int (*pfnGraphicBufferReallocate)(void*, uint32_t w, uint32_t h, uint32_t format);
	pfnGraphicBufferReallocate fGraphicBufferReallocate;

	bool ensureInitialized()
	{
		if (m_Initialized)
		{
			return true;
		}

		void *handle = dlopen(ANDROID_EGL_PATH, RTLD_LAZY);
		if (!handle)
		{
			LOG("Couldn't load EGL library");
			return false;
		}

		fGetDisplay = (pfnGetDisplay)dlsym(handle, "eglGetDisplay");
		fEGLGetError = (pfnEGLGetError)dlsym(handle, "eglGetError");
		fCreateImageKHR = (pfnCreateImageKHR)dlsym(handle, "eglCreateImageKHR");
		fDestroyImageKHR = (pfnDestroyImageKHR)dlsym(handle, "eglDestroyImageKHR");

		if (!fGetDisplay || !fEGLGetError || !fCreateImageKHR || !fDestroyImageKHR)
		{
			LOG("Failed to find some EGL functions");
			return false;
		}

		handle = dlopen(ANDROID_GLES_PATH, RTLD_LAZY);
		if (!handle) {
			LOG("Couldn't load GL library");
			return false;
		}

		fImageTargetTexture2DOES = (pfnImageTargetTexture2DOES)dlsym(handle, "glEGLImageTargetTexture2DOES");
		fBindTexture = (pfnBindTexture)dlsym(handle, "glBindTexture");
		fGLGetError = (pfnGLGetError)dlsym(handle, "glGetError");

		if (!fImageTargetTexture2DOES || !fBindTexture || !fGLGetError)
		{
			LOG("Failed to find some GL functions");
			return false;
		}

		handle = dlopen(ANDROID_LIBUI_PATH, RTLD_LAZY);
		if (!handle)
		{
			LOG("Couldn't load libui.so");
			return false;
		}

		fGraphicBufferCtor = (pfnGraphicBufferCtor)dlsym(handle, "_ZN7android13GraphicBufferC1Ejjij");
		fGraphicBufferDtor = (pfnGraphicBufferDtor)dlsym(handle, "_ZN7android13GraphicBufferD1Ev");
		fGraphicBufferLock = (pfnGraphicBufferLock)dlsym(handle, "_ZN7android13GraphicBuffer4lockEjPPv");
		fGraphicBufferLockRect = (pfnGraphicBufferLockRect)dlsym(handle, "_ZN7android13GraphicBuffer4lockEjRKNS_4RectEPPv");
		fGraphicBufferUnlock = (pfnGraphicBufferUnlock)dlsym(handle, "_ZN7android13GraphicBuffer6unlockEv");
		fGraphicBufferGetNativeBuffer = (pfnGraphicBufferGetNativeBuffer)dlsym(handle, "_ZNK7android13GraphicBuffer15getNativeBufferEv");
		fGraphicBufferReallocate = (pfnGraphicBufferReallocate)dlsym(handle, "_ZN7android13GraphicBuffer10reallocateEjjij");

		if (!fGraphicBufferCtor || !fGraphicBufferDtor || !fGraphicBufferLock ||
			!fGraphicBufferUnlock || !fGraphicBufferGetNativeBuffer)
		{
			LOG("Failed to lookup some GraphicBuffer functions");
			return false;
		}

		m_Initialized = true;
		return true;
	}

private:
	bool m_Initialized;
} sGLFunctions;

static void clearGLError()
{
	while (sGLFunctions.fGLGetError() != GL_NO_ERROR);
}

static bool ensureNoGLError(const char* name)
{
	bool result = true;
	GLuint error;

	while ((error = sGLFunctions.fGLGetError()) != GL_NO_ERROR)
	{
		LOG("GL error [%s]: %40x\n", name, error);
		result = false;
	}
	return result;
}

static __attribute__((noinline)) unsigned next_pow2(unsigned x)
{
	x -= 1;
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	return x + 1;
}
AndroidGraphicBuffer::AndroidGraphicBuffer(int width, int height,
	AndroidGraphicBuffer::gfxUsage usage,
	AndroidGraphicBuffer::gfxImageFormat format) :
	m_Width(next_pow2(width)), m_Height(next_pow2(height)), m_Usage(usage), m_Format(format),
	m_Handle(nullptr), m_EGLImage(nullptr)
{
	LOG("Size rounded to the next power of 2 (%dx%d)", m_Width, m_Height);
}

AndroidGraphicBuffer::~AndroidGraphicBuffer()
{
	destroyBuffer();
}

void AndroidGraphicBuffer::destroyBuffer()
{
	/**
	* XXX: eglDestroyImageKHR crashes sometimes due to refcount badness (I think)
	*
	* If you look at egl.cpp (https://github.com/android/platform_frameworks_base/blob/master/opengl/libagl/egl.cpp#L2002)
	* you can see that eglCreateImageKHR just refs the native buffer, and eglDestroyImageKHR
	* just unrefs it. Somehow the ref count gets messed up and things are already destroyed
	* by the time eglDestroyImageKHR gets called. For now, at least, just not calling
	* eglDestroyImageKHR should be fine since we do free the GraphicBuffer below.
	*
	* Bug 712716
	*/
#if 0
	if (m_EGLImage)
	{
		if (sGLFunctions.EnsureInitialized())
		{
			sGLFunctions.fDestroyImageKHR(sGLFunctions.fGetDisplay(EGL_DEFAULT_DISPLAY), m_EGLImage);
			m_EGLImage = nullptr;
		}
	}
#endif
	m_EGLImage = nullptr;

	if (m_Handle)
	{
		if (sGLFunctions.ensureInitialized())
		{
			sGLFunctions.fGraphicBufferDtor(m_Handle);
		}
		free(m_Handle);
		m_Handle = nullptr;
	}
}

bool AndroidGraphicBuffer::ensureBufferCreated()
{
	if (!m_Handle)
	{
		m_Handle = malloc(GRAPHIC_BUFFER_SIZE);
		sGLFunctions.fGraphicBufferCtor(m_Handle, m_Width, m_Height,
			getAndroidFormat(m_Format), getAndroidUsage(m_Usage));
	}
	return true;
}

bool AndroidGraphicBuffer::ensureInitialized()
{
	if (!sGLFunctions.ensureInitialized())
	{
		return false;
	}
	ensureBufferCreated();
	return true;
}

int AndroidGraphicBuffer::lock(AndroidGraphicBuffer::gfxUsage aUsage,
	unsigned char **bits)
{
	if (!ensureInitialized())
	{
		return true;
	}
	return sGLFunctions.fGraphicBufferLock(m_Handle, getAndroidUsage(aUsage), bits);
}

int AndroidGraphicBuffer::lock(AndroidGraphicBuffer::gfxUsage aUsage,
	const AndroidGraphicBuffer::rect& aRect, unsigned char **bits)
{
	if (!ensureInitialized())
	{
		return false;
	}

	ARect rect;
	rect.left = aRect.x;
	rect.top = aRect.y;
	rect.right = aRect.x + aRect.width;
	rect.bottom = aRect.y + aRect.height;
	return sGLFunctions.fGraphicBufferLockRect(m_Handle, getAndroidUsage(aUsage), rect, bits);
}

int AndroidGraphicBuffer::unlock()
{
	if (!ensureInitialized())
	{
		return false;
	}
	return sGLFunctions.fGraphicBufferUnlock(m_Handle);
}

bool AndroidGraphicBuffer::reallocate(int aWidth, int aHeight,
	AndroidGraphicBuffer::gfxImageFormat aFormat)
{
	if (!ensureInitialized())
	{
		return false;
	}

	m_Width = aWidth;
	m_Height = aHeight;
	m_Format = aFormat;

	// Sometimes GraphicBuffer::reallocate just doesn't work. In those cases we'll just allocate a brand
	// new buffer. If reallocate fails once, never try it again.
	if (!gTryRealloc || sGLFunctions.fGraphicBufferReallocate(m_Handle, aWidth, aHeight, getAndroidFormat(aFormat)) != 0)
	{
		destroyBuffer();
		ensureBufferCreated();
		gTryRealloc = false;
	}
	return true;
}

uint32_t AndroidGraphicBuffer::getAndroidUsage(gfxUsage aUsage)
{
	uint32_t flags = 0;

	if (aUsage & UsageSoftwareRead)
	{
		flags |= GRALLOC_USAGE_SW_READ_OFTEN;
	}

	if (aUsage & UsageSoftwareWrite)
	{
		flags |= GRALLOC_USAGE_SW_WRITE_OFTEN;
	}

	if (aUsage & UsageTexture)
	{
		flags |= GRALLOC_USAGE_HW_TEXTURE;
	}

	if (aUsage & UsageTarget)
	{
		flags |= GRALLOC_USAGE_HW_RENDER;
	}

	if (aUsage & Usage2D)
	{
		flags |= GRALLOC_USAGE_HW_2D;
	}
	return flags;
}

uint32_t AndroidGraphicBuffer::getAndroidFormat(AndroidGraphicBuffer::gfxImageFormat aFormat)
{
	switch (aFormat)
	{
		case RGB24:
		{
			return HAL_PIXEL_FORMAT_RGBX_8888;
		}
		case RGB16_565:
		{
			return HAL_PIXEL_FORMAT_RGB_565;
		}
		case ARGB32:
		{
			return HAL_PIXEL_FORMAT_RGBA_8888;
		}
		default:
		{
			return 0;
		}
	}
}

bool AndroidGraphicBuffer::ensureEGLImage()
{
	if (m_EGLImage)
	{
		return true;
	}
	if (!ensureInitialized())
	{
		return false;
	}
	EGLint eglImgAttrs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE, EGL_NONE };
	void *nativeBuffer = sGLFunctions.fGraphicBufferGetNativeBuffer(m_Handle);

	m_EGLImage = sGLFunctions.fCreateImageKHR(sGLFunctions.fGetDisplay(EGL_DEFAULT_DISPLAY),
		EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, (EGLClientBuffer)nativeBuffer, eglImgAttrs);
	return m_EGLImage != nullptr;
}

bool AndroidGraphicBuffer::bind()
{
	if (!ensureInitialized())
	{
		return false;
	}
	if (!ensureEGLImage())
	{
		LOG("No valid EGLImage!");
		return false;
	}
	clearGLError();
	sGLFunctions.fImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, m_EGLImage);
	return ensureNoGLError("glEGLImageTargetTexture2DOES");
}
