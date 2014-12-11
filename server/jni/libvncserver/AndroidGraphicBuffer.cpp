#include <android/log.h>
#include <dlfcn.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <stdlib.h>

#include "AndroidGraphicBuffer.h"

#define LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "AndroidGraphicBuffer" , ## args)

#define ANDROID_LIBUI_PATH "libui.so"

// Really I have no idea, but this should be big enough
#define GRAPHIC_BUFFER_SIZE 1024

static bool gTryRealloc = true;

static class GLFunctions
{
public:
	constexpr GLFunctions() :
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

		void *handle = dlopen(ANDROID_LIBUI_PATH, RTLD_LAZY);
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
	while (glGetError() != GL_NO_ERROR);
}

static bool ensureNoGLError(const char* name)
{
	bool result = true;
	GLuint error;

	while ((error = glGetError()) != GL_NO_ERROR)
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
	uint32_t usage, uint32_t format) :
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
			m_Format, m_Usage);
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

int AndroidGraphicBuffer::lock(uint32_t aUsage,
	unsigned char **bits)
{
	if (!ensureInitialized())
	{
		return true;
	}
	return sGLFunctions.fGraphicBufferLock(m_Handle, aUsage, bits);
}

int AndroidGraphicBuffer::lock(uint32_t aUsage,
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
	return sGLFunctions.fGraphicBufferLockRect(m_Handle, aUsage, rect, bits);
}

int AndroidGraphicBuffer::unlock()
{
	if (!ensureInitialized())
	{
		return false;
	}
	return sGLFunctions.fGraphicBufferUnlock(m_Handle);
}

bool AndroidGraphicBuffer::reallocate(int aWidth, int aHeight, uint32_t aFormat)
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
	if (!gTryRealloc || sGLFunctions.fGraphicBufferReallocate(m_Handle, aWidth, aHeight, aFormat) != 0)
	{
		destroyBuffer();
		ensureBufferCreated();
		gTryRealloc = false;
	}
	return true;
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

	m_EGLImage = eglCreateImageKHR(eglGetDisplay(EGL_DEFAULT_DISPLAY),
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
	glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, m_EGLImage);
	return ensureNoGLError("glEGLImageTargetTexture2DOES");
}
