#ifndef ANDROIDGRAPHICBBUFFER_H_
#define ANDROIDGRAPHICBBUFFER_H_

#include <EGL/eglext.h>
#include <stdint.h>

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

	struct rect {
		int x;
		int y;
		int width;
		int height;
	};

	AndroidGraphicBuffer(int width, int height, uint32_t format);
	virtual ~AndroidGraphicBuffer();

	int lock(unsigned char **bits);
	int lock(const rect &rect, unsigned char **bits);
	int unlock();
	bool reallocate(int aWidth, int aHeight, uint32_t aFormat);

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
	uint32_t m_Usage;
	uint32_t m_Format;

	bool ensureInitialized();
	bool ensureEGLImage();

	void destroyBuffer();
	bool ensureBufferCreated();

	void *m_Handle;
	EGLImageKHR m_EGLImage;
};

#endif /* ANDROIDGRAPHICBBUFFER_H_ */
