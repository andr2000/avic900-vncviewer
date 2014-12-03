#ifndef ANDROIDGRAPHICBBUFFER_H_
#define ANDROIDGRAPHICBBUFFER_H_

#include <stdint.h>

typedef void *EGLImageKHR;
typedef void *EGLClientBuffer;

/**
 * This class allows access to Android's direct texturing mechanism. Locking
 * the buffer gives you a pointer you can read/write to directly. It is fully
 * threadsafe, but you probably really want to use the AndroidDirectTexture
 * class which will handle double buffering.
 *
 * In order to use the buffer in OpenGL, just call Bind() and it will attach
 * to whatever texture is bound to GL_TEXTURE_2D.
 */
class AndroidGraphicBuffer
{
public:
	enum gfxUsage {
		UsageSoftwareRead = 1,
		UsageSoftwareWrite = 1 << 1,
		UsageTexture = 1 << 2,
		UsageTarget = 1 << 3,
		Usage2D = 1 << 4
	};

	enum gfxImageFormat {
		ARGB32,		/* ARGB data in native endianness, using premultiplied alpha */
		RGB24,		/* xRGB data in native endianness */
		A8,			/* Only an alpha channel */
		A1,			/* Packed transparency information (one byte refers to 8 pixels) */
		RGB16_565,	/* RGB_565 data in native endianness */
		Unknown
	};

	struct rect {
		int x;
		int y;
		int width;
		int height;
	};

	AndroidGraphicBuffer(int width, int height, gfxUsage usage, gfxImageFormat format);
	virtual ~AndroidGraphicBuffer();

	int lock(gfxUsage usage, unsigned char **bits);
	int lock(gfxUsage usage, const rect &rect, unsigned char **bits);
	int unlock();
	bool reallocate(int aWidth, int aHeight, gfxImageFormat aFormat);

	int getWidth()
	{
		return m_Width;
	}
	int getHeight()
	{
		return m_Height;
	}

	bool bind();

private:
	uint32_t m_Width;
	uint32_t m_Height;
	gfxUsage m_Usage;
	gfxImageFormat m_Format;

	bool ensureInitialized();
	bool ensureEGLImage();

	void destroyBuffer();
	bool ensureBufferCreated();

	uint32_t getAndroidUsage(gfxUsage aUsage);
	uint32_t getAndroidFormat(gfxImageFormat aFormat);

	void *m_Handle;
	void *m_EGLImage;
};

#endif /* ANDROIDGRAPHICBBUFFER_H_ */
