#ifndef _STELRENDERER_HPP_
#define _STELRENDERER_HPP_

#include <QColor>
#include <QImage>
#include <QPainter>
#include <QSize>

#include "StelIndexBuffer.hpp"
#include "StelVertexBuffer.hpp"
#include "StelViewportEffect.hpp"
#include "StelTexture.hpp"
#include "StelTextureBackend.hpp"
#include "StelTextureParams.hpp"

//! Pixel blending modes.
//!
//! Used for example for transparency, mixing colors in background 
//! with colors drawn in front of it.
enum BlendMode
{
	//! No blending, new color overrides previous color.
	BlendMode_None,
	//! Colors of each channel are added up, clamping at maximum value 
	//! (255 for 8bit channels, 1.0 for floating-point colors)
	BlendMode_Add,
	//! Use alpha value of the front color for blending.
	//! (0 is fully transparent, 255 or 1.0 fully opague)
	BlendMode_Alpha
};


//Notes:
//
//enable/disablePainting are temporary and will be removed
//    once painting is done through StelRenderer.

//! Provides access to scene rendering calls so the Renderer can control it.
//!
//! Renderer implementations might decide to only draw parts of the scene 
//! each frame to increase FPS. This allows them to do so.
class StelRenderClient
{
public:
	//! Partially draw the scene.
	//!
	//! @return false if the last part of the scene was drawn 
	//!         (i.e. we're done drawing), true otherwise.
	virtual bool drawPartial() = 0;

	//! Get QPainter used for Qt-style painting to the viewport.
	//!
	//! (Note that this painter might not necessarily end up being used - 
	//! e.g. if a GL backend uses FBOs, it creates a painter to draw to the
	//! FBOs.)
	virtual QPainter* getPainter() = 0;

	//! Get viewport effect to apply when drawing the viewport
	//!
	//! This can return NULL, in which case no viewport effect is used.
	virtual StelViewportEffect* getViewportEffect() = 0;
};

//! Handles all graphics related functionality.
//! 
//! @note This is an interface. It should have only functions, no data members,
//!       as it might be used in multiple inheritance.
class StelRenderer
{
public:
	//! Initialize the renderer. 
	//! 
	//! Must be called before any other methods.
	//!
	//! @return true on success, false if there was an error.
	virtual bool init() = 0;
	
	//! Take a screenshot and return it.
	virtual QImage screenshot() = 0;
	
	//! Enable painting.
	//!
	//! Painting must be disabled when calling this.
	//!
	//! This method is temporary and will be removed once painting is handled through Renderer.
	virtual void enablePainting() = 0;
	
	//! Disable painting.
	//!
	//! Painting must be enabled when calling this.
	//!
	//! This method is temporary and will be removed once painting is handled through Renderer.
	virtual void disablePainting() = 0;
	
	//! Must be called once at startup and on every GL viewport resize, specifying new size.
	virtual void viewportHasBeenResized(const QSize size) = 0;

	//! Create an empty index buffer and return a pointer to it.
	virtual class StelIndexBuffer* createIndexBuffer(const IndexType type) = 0;
	
	//! Create an empty vertex buffer and return a pointer to it.
	//!
	//! The vertex buffer must be deleted by the user once it is not used
	//! (and before the Renderer is destroyed).
	//!
	//! @tparam V Vertex type. See the example in StelVertexBuffer documentation
	//!           on how to define a vertex type.
	//! @param primitiveType Graphics primitive type stored in the buffer.
	//!
	//! @return New vertex buffer storing vertices of type V.
	template<class V>
	StelVertexBuffer<V>* createVertexBuffer(const PrimitiveType primitiveType)
	{
		return new StelVertexBuffer<V>(createVertexBufferBackend(primitiveType, V::attributes));
	}
	
	//! Draw contents of a vertex buffer.
	//!
	//! @param vertexBuffer Vertex buffer to draw.
	//! @param indexBuffer  Index buffer specifying which vertices from the buffer to draw.
	//!                     If NULL, indexing will not be used and vertices will be drawn
	//!                     directly in order they are in the buffer.
	//! @param projector    Projector to project vertices' positions before drawing.
	//!                     Also determines viewport to draw in.
	//!                     If NULL, no projection will be done and the vertices will be drawn
	//!                     directly.
	//! @param dontProject  Disable vertex position projection.
	//!                     (Projection matrix and viewport information of the 
	//!                     projector are still used)
	//!
	//!                     This is a hack to support StelSkyDrawer, which already 
	//!                     projects the star positions before drawing them.
	//!                     Avoid using this if possible. StelSkyDrawer might be 
	//!                     refactored in future to remove the need for dontProject,
	//!                     in which case it should be removed.
	template<class V>
	void drawVertexBuffer(StelVertexBuffer<V>* vertexBuffer, 
	                      class StelIndexBuffer* indexBuffer = NULL,
	                      StelProjectorP projector = StelProjectorP(NULL),
	                      bool dontProject = false)
	{
		drawVertexBufferBackend(vertexBuffer->backend, indexBuffer, projector, dontProject);
	}

	//! Draw a rectangle to the screen.
	//!
	//! The rectangle will be colored by the global color
	//! (which can be specified by setGlobalColor()).
	//!
	//! Optionally, the rectangle can be textured by the currently bound texture
	//! (on by default).
	//!
	//! Default implementation uses other Renderer functions to draw the rectangle,
	//! but can be overridden if a more optimized implementation is needed.
	//!
	//! @param x        X position of the top left corner on the screen in pixels.
	//! @param y        Y position of the top left corner on the screen in pixels. 
	//! @param width    Width in pixels.
	//! @param height   Height in pixels.
	//! @param textured Draw textured or jus plain color rectangle?
	virtual void drawRect(const float x, const float y, 
	                      const float width, const float height, 
	                      const bool textured = true);
	
	//! Bind a texture (following draw calls will use this texture on specified texture unit).
	//!
	//! @param  textureBackend Texture to bind.
	//! @param  textureUnit Texture unit to use. 
	//!                     If multitexturing is not supported, 
	//!                     binds to texture units other than 0 are ignored.
	virtual void bindTexture(class StelTextureBackend* textureBackend, const int textureUnit) = 0;

	//! Render a single frame.
	//!
	//! This might not render the entire frame - if rendering takes too long,
	//! the backend may (or may not) suspend the rendering to finish it next 
	//! time renderFrame is called.
	//!
	//! @param renderClient Allows the renderer to draw the scene in parts.
	//!
	//! @see StelRenderClient
	virtual void renderFrame(StelRenderClient& renderClient) = 0;
	
	//Both of these will be hidden behind a createTexture() and 
	//the new StelTexture's dtor

	//! Create a StelTextureBackend from specified file or URL.
	//!
	//! Note that the StelTextureBackend created here must be destroyed 
	//! by calling destroyTextureBackend of the same renderer.
	//! This allows things like texture caching to work.
	//!
	//! @param  filename    File name or URL of the image to load the texture from.
	//!                     If it's a file and it's not found, it's searched for in
	//!                     the textures/ directory. Some renderer backends might also 
	//!                     support compressed textures with custom file formats.
	//!                     These backend-specific files should
	//!                     not be used as filename - instead, if a compressed
	//!                     texture with the same file name but different extensions
	//!                     exists, it will be used.
	//!                     E.g. the GLES backend prefers a .pvr version of a texture
	//!                     if it exists.
	//! @param  params      Texture parameters, such as filtering, wrapping, etc.
	//! @param  loadingMode Texture loading mode to use. Normal immediately loads
	//!                     the texture, Asynchronous starts loading it in a background
	//!                     thread and LazyAsynchronous starts loading it when it's
	//!                     first needed.
	//!
	//! @return New texture backend on success, or NULL on failure.
	StelTextureBackend* createTextureBackend
		(const QString& filename, const StelTextureParams& params, 
		 const TextureLoadingMode loadingMode)
	{
		//This function tests preconditions and calls implementation.
		Q_ASSERT_X(!filename.endsWith(".pvr"), Q_FUNC_INFO,
		           "createTextureBackend() can't load a PVR texture directly, as PVR "
		           "support may not be implemented by all Renderer backends. Request "
		           "a non-PVR texture, and if a PVR version exists and the backend "
		           "supports it, it will be loaded.");
		Q_ASSERT_X(!filename.isEmpty(), Q_FUNC_INFO,
		           "Trying to load a texture with an empty filename or URL");
		Q_ASSERT_X(!(filename.startsWith("http://") && loadingMode == TextureLoadingMode_Normal),
		           Q_FUNC_INFO,
		           "When loading a texture from network, texture loading mode must be "
		           "Asynchronous or LazyAsynchronous");

		return createTextureBackend_(filename, params, loadingMode);
	}

	//! Get a texture of the viewport, with everything drawn to the viewport so far.
	//!
	//! @note Since some backends only support textures with power of two 
	//! dimensions, the returned texture might be larger than the viewport
	//! in which case only the part of the texture matching viewport size 
	//! (returned by getViewportSize) will be used.
	//!
	//! @return Viewport texture.
	StelTexture* getViewportTexture()
	{
		return new StelTexture(getViewportTextureBackend(), this);
	}

	//! Destroy a StelTextureBackend.
	virtual void destroyTextureBackend(StelTextureBackend* backend) = 0;

	//! Get size of the viewport in pixels.
	virtual QSize getViewportSize() const = 0;

	//! Set the global vertex color.
	//!
	//! Default color is white.
	//!
	//! This color is used when rendering vertex formats that have no vertex color attribute.
	//!
	//! Per-vertex color completely overrides this 
	//! (this is to keep behavior from before the GL refactor unchanged).
	virtual void setGlobalColor(const QColor& color) = 0;

	//! Set blend mode.
	//!
	//! Used to enable/disable transparency and similar effects.
	//!
	//! On startup, the blend mode is BlendMode_None.
	virtual void setBlendMode(const BlendMode blendMode) = 0;

protected:
	//! Create a vertex buffer backend. Used by createVertexBuffer.
	//!
	//! This allows each Renderer backend to create its own vertex buffer backend.
	//!
	//! @param primitiveType Graphics primitive type stored in the buffer.
	//! @param attributes    Descriptions of all attributes of the vertex type stored in the buffer.
	//!
	//! @return Pointer to a vertex buffer backend specific to the Renderer backend.
	virtual StelVertexBufferBackend* createVertexBufferBackend
		(const PrimitiveType primitiveType, const QVector<StelVertexAttribute>& attributes) = 0;

	//! Draw contents of a vertex buffer (backend). Used by drawVertexBuffer.
	//!
	//! @see drawVertexBuffer
	virtual void drawVertexBufferBackend(StelVertexBufferBackend* vertexBuffer, 
	                                     class StelIndexBuffer* indexBuffer,
	                                     StelProjectorP projector,
	                                     const bool dontProject) = 0;

	//! Implementation of createTextureBackend.
	//!
	//! @see createTextureBackend
	virtual class StelTextureBackend* createTextureBackend_
		(const QString& filename, const StelTextureParams& params, 
		 const TextureLoadingMode loadingMode) = 0;

	//! Implementation of getViewportTexture.
	//!
	//! @see getViewportTexture.
	virtual StelTextureBackend* getViewportTextureBackend() = 0;
};

#endif // _STELRENDERER_HPP_
