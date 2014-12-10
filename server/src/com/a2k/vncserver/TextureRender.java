package com.a2k.vncserver;

import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import android.graphics.Bitmap;
import android.graphics.SurfaceTexture;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.Matrix;
import android.util.Log;

class TextureRender
{
	private static final String TAG = "TextureRender";

	private static final int FLOAT_SIZE_BYTES = 4;
	private static final int TRIANGLE_VERTICES_DATA_STRIDE_BYTES = 5 * FLOAT_SIZE_BYTES;
	private static final int TRIANGLE_VERTICES_DATA_POS_OFFSET = 0;
	private static final int TRIANGLE_VERTICES_DATA_UV_OFFSET = 3;
	private final float[] m_TriangleVerticesData =
	{
		 /* X      Y  Z,   U    V */
		-1.0f, -1.0f, 0, 0.f, 0.f,
		 1.0f, -1.0f, 0, 1.f, 0.f,
		-1.0f,  1.0f, 0, 0.f, 1.f,
		 1.0f,  1.0f, 0, 1.f, 1.f,
	};

	private FloatBuffer m_TriangleVertices;

	private static final String VERTEX_SHADER =
		"uniform mat4 uMVPMatrix;\n" +
		"uniform mat4 uSTMatrix;\n" +
		"attribute vec4 aPosition;\n" +
		"attribute vec4 aTextureCoord;\n" +
		"varying vec2 vTextureCoord;\n" +
		"void main() {\n" +
		"  gl_Position = uMVPMatrix * aPosition;\n" +
		"  vTextureCoord = (uSTMatrix * aTextureCoord).xy;\n" +
		"}\n";

	private static final String FRAGMENT_SHADER =
		"#extension GL_OES_EGL_image_external : require\n" +
		"precision mediump float;\n" +      // highp here doesn't seem to matter
		"varying vec2 vTextureCoord;\n" +
		"uniform samplerExternalOES sTexture;\n" +
		"void main() {\n" +
		"  gl_FragColor = texture2D(sTexture, vTextureCoord);\n" +
		"}\n";

	private float[] m_MVPMatrix = new float[16];
	private float[] m_STMatrix = new float[16];

	private int m_Program;
	private int m_TextureID = -12345;
	private int m_uMVPMatrixHandle;
	private int m_uSTMatrixHandle;
	private int m_aPositionHandle;
	private int m_aTextureHandle;

	public TextureRender()
	{
		m_TriangleVertices = ByteBuffer.allocateDirect(
			m_TriangleVerticesData.length * FLOAT_SIZE_BYTES)
			.order(ByteOrder.nativeOrder()).asFloatBuffer();
		m_TriangleVertices.put(m_TriangleVerticesData).position(0);

		Matrix.setIdentityM(m_STMatrix, 0);
	}

	public int getTextureId()
	{
		return m_TextureID;
	}

	public void drawFrame(SurfaceTexture st)
	{
		checkGlError("onDrawFrame start");
		st.getTransformMatrix(m_STMatrix);

		GLES20.glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
		GLES20.glClear(GLES20.GL_DEPTH_BUFFER_BIT | GLES20.GL_COLOR_BUFFER_BIT);

		GLES20.glUseProgram(m_Program);
		checkGlError("glUseProgram");

		GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
		GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, m_TextureID);

		m_TriangleVertices.position(TRIANGLE_VERTICES_DATA_POS_OFFSET);
		GLES20.glVertexAttribPointer(m_aPositionHandle, 3, GLES20.GL_FLOAT, false,
			TRIANGLE_VERTICES_DATA_STRIDE_BYTES, m_TriangleVertices);
		checkGlError("glVertexAttribPointer maPosition");
		GLES20.glEnableVertexAttribArray(m_aPositionHandle);
		checkGlError("glEnableVertexAttribArray maPositionHandle");

		m_TriangleVertices.position(TRIANGLE_VERTICES_DATA_UV_OFFSET);
		GLES20.glVertexAttribPointer(m_aTextureHandle, 2, GLES20.GL_FLOAT, false,
			TRIANGLE_VERTICES_DATA_STRIDE_BYTES, m_TriangleVertices);
		checkGlError("glVertexAttribPointer maTextureHandle");
		GLES20.glEnableVertexAttribArray(m_aTextureHandle);
		checkGlError("glEnableVertexAttribArray maTextureHandle");

		Matrix.setIdentityM(m_MVPMatrix, 0);
		GLES20.glUniformMatrix4fv(m_uMVPMatrixHandle, 1, false, m_MVPMatrix, 0);
		GLES20.glUniformMatrix4fv(m_uSTMatrixHandle, 1, false, m_STMatrix, 0);

		GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
		checkGlError("glDrawArrays");
		GLES20.glFinish();
	}

	public void surfaceCreated()
	{
		m_Program = createProgram(VERTEX_SHADER, FRAGMENT_SHADER);
		if (m_Program == 0)
		{
			throw new RuntimeException("failed creating program");
		}
		m_aPositionHandle = GLES20.glGetAttribLocation(m_Program, "aPosition");
		checkGlError("glGetAttribLocation aPosition");
		if (m_aPositionHandle == -1)
		{
			throw new RuntimeException("Could not get attrib location for aPosition");
		}
		m_aTextureHandle = GLES20.glGetAttribLocation(m_Program, "aTextureCoord");
		checkGlError("glGetAttribLocation aTextureCoord");
		if (m_aTextureHandle == -1)
		{
			throw new RuntimeException("Could not get attrib location for aTextureCoord");
		}

		m_uMVPMatrixHandle = GLES20.glGetUniformLocation(m_Program, "uMVPMatrix");
		checkGlError("glGetUniformLocation uMVPMatrix");
		if (m_uMVPMatrixHandle == -1)
		{
			throw new RuntimeException("Could not get attrib location for uMVPMatrix");
		}

		m_uSTMatrixHandle = GLES20.glGetUniformLocation(m_Program, "uSTMatrix");
		checkGlError("glGetUniformLocation uSTMatrix");
		if (m_uSTMatrixHandle == -1)
		{
			throw new RuntimeException("Could not get attrib location for uSTMatrix");
		}

		int[] textures = new int[1];
		GLES20.glGenTextures(1, textures, 0);

		m_TextureID = textures[0];
		GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, m_TextureID);
		checkGlError("glBindTexture mTextureID");

		GLES20.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);
		GLES20.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
		GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
		GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
		checkGlError("glTexParameter");
	}

	public void changeFragmentShader(String fragmentShader)
	{
		GLES20.glDeleteProgram(m_Program);
		m_Program = createProgram(VERTEX_SHADER, fragmentShader);
		if (m_Program == 0)
		{
			throw new RuntimeException("failed creating program");
		}
	}

	private int loadShader(int shaderType, String source)
	{
		int shader = GLES20.glCreateShader(shaderType);
		checkGlError("glCreateShader type=" + shaderType);
		GLES20.glShaderSource(shader, source);
		GLES20.glCompileShader(shader);
		int[] compiled = new int[1];
		GLES20.glGetShaderiv(shader, GLES20.GL_COMPILE_STATUS, compiled, 0);
		if (compiled[0] == 0)
		{
			Log.e(TAG, "Could not compile shader " + shaderType + ":");
			Log.e(TAG, " " + GLES20.glGetShaderInfoLog(shader));
			GLES20.glDeleteShader(shader);
			shader = 0;
		}
		return shader;
	}

	private int createProgram(String vertexSource, String fragmentSource)
	{
		int vertexShader = loadShader(GLES20.GL_VERTEX_SHADER, vertexSource);
		if (vertexShader == 0)
		{
			return 0;
		}
		int pixelShader = loadShader(GLES20.GL_FRAGMENT_SHADER, fragmentSource);
		if (pixelShader == 0)
		{
			return 0;
		}

		int program = GLES20.glCreateProgram();
		checkGlError("glCreateProgram");
		if (program == 0)
		{
			Log.e(TAG, "Could not create program");
		}
		GLES20.glAttachShader(program, vertexShader);
		checkGlError("glAttachShader");
		GLES20.glAttachShader(program, pixelShader);
		checkGlError("glAttachShader");
		GLES20.glLinkProgram(program);
		int[] linkStatus = new int[1];
		GLES20.glGetProgramiv(program, GLES20.GL_LINK_STATUS, linkStatus, 0);
		if (linkStatus[0] != GLES20.GL_TRUE)
		{
			Log.e(TAG, "Could not link program: ");
			Log.e(TAG, GLES20.glGetProgramInfoLog(program));
			GLES20.glDeleteProgram(program);
			program = 0;
		}
		return program;
	}

	public void checkGlError(String op)
	{
		int error;
		while ((error = GLES20.glGetError()) != GLES20.GL_NO_ERROR)
		{
			Log.e(TAG, op + ": glError " + error);
			throw new RuntimeException(op + ": glError " + error);
		}
	}

	public static void saveFrame(String filename, int width, int height)
	{
		// glReadPixels gives us a ByteBuffer filled with what is essentially big-endian RGBA
		// data (i.e. a byte of red, followed by a byte of green...).  We need an int[] filled
		// with native-order ARGB data to feed to Bitmap.
		//
		// If we implement this as a series of buf.get() calls, we can spend 2.5 seconds just
		// copying data around for a 720p frame.  It's better to do a bulk get() and then
		// rearrange the data in memory.  (For comparison, the PNG compress takes about 500ms
		// for a trivial frame.)
		//
		// So... we set the ByteBuffer to little-endian, which should turn the bulk IntBuffer
		// get() into a straight memcpy on most Android devices.  Our ints will hold ABGR data.
		// Swapping B and R gives us ARGB.  We need about 30ms for the bulk get(), and another
		// 270ms for the color swap.
		//
		// Making this even more interesting is the upside-down nature of GL, which means we
		// may want to flip the image vertically here.

		ByteBuffer buf = ByteBuffer.allocateDirect(width * height * 4);
		buf.order(ByteOrder.LITTLE_ENDIAN);
		GLES20.glReadPixels(0, 0, width, height, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, buf);
		buf.rewind();

		int pixelCount = width * height;
		int[] colors = new int[pixelCount];
		buf.asIntBuffer().get(colors);
		for (int i = 0; i < pixelCount; i++)
		{
			int c = colors[i];
			colors[i] = (c & 0xff00ff00) | ((c & 0x00ff0000) >> 16) | ((c & 0x000000ff) << 16);
		}

		FileOutputStream fos = null;
		try
		{
			fos = new FileOutputStream(filename);
			Bitmap bmp = Bitmap.createBitmap(colors, width, height, Bitmap.Config.ARGB_8888);
			bmp.compress(Bitmap.CompressFormat.PNG, 90, fos);
			bmp.recycle();
		}
		catch (IOException ioe)
		{
			throw new RuntimeException("Failed to write file " + filename, ioe);
		}
		finally
		{
			try
			{
				if (fos != null)
				{
					fos.close();
				}
			}
			catch (IOException ioe2)
			{
				throw new RuntimeException("Failed to close file " + filename, ioe2);
			}
		}
		Log.d(TAG, "Saved " + width + "x" + height + " frame as '" + filename + "'");
	}
}
