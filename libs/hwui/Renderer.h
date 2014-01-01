/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_HWUI_RENDERER_H
#define ANDROID_HWUI_RENDERER_H

#include <SkRegion.h>

#include <utils/String8.h>

#include "AssetAtlas.h"
#include "SkPaint.h"

namespace android {

class Functor;
class Res_png_9patch;

namespace uirenderer {

class DisplayList;
class Layer;
class Matrix4;
class SkiaColorFilter;
class SkiaShader;
class Patch;

enum DrawOpMode {
    kDrawOpMode_Immediate,
    kDrawOpMode_Defer,
    kDrawOpMode_Flush
};

/**
 * Hwui's abstract version of Canvas.
 *
 * Provides methods for frame state operations, as well as the SkCanvas style transform/clip state,
 * and varied drawing operations.
 *
 * Should at some point interact with native SkCanvas.
 */
class ANDROID_API Renderer {
public:
    virtual ~Renderer() {}

    /**
     * Sets the name of this renderer. The name is optional and empty by default, for debugging
     * purposes only. If the pointer is null the name is set to the empty string.
     */
    void setName(const char * name) {
        if (name) {
            mName.setTo(name);
        } else {
            mName.clear();
        }
    }

    /**
     * Returns the name of this renderer as UTF8 string.
     * The returned pointer is never null.
     */
    const char* getName() const {
        return mName.string();
    }

    /**
     * Indicates whether this renderer is recording drawing commands for later playback.
     * If this method returns true, the drawing commands are deferred.
     */
    virtual bool isRecording() const {
        return false;
    }

    /**
     * Safely retrieves the mode from the specified xfermode. If the specified
     * xfermode is null, the mode is assumed to be SkXfermode::kSrcOver_Mode.
     */
    static inline SkXfermode::Mode getXfermode(SkXfermode* mode) {
        SkXfermode::Mode resultMode;
        if (!SkXfermode::AsMode(mode, &resultMode)) {
            resultMode = SkXfermode::kSrcOver_Mode;
        }
        return resultMode;
    }

// ----------------------------------------------------------------------------
// Frame state operations
// ----------------------------------------------------------------------------
    /**
     * Sets the dimension of the underlying drawing surface. This method must
     * be called at least once every time the drawing surface changes size.
     *
     * @param width The width in pixels of the underlysing surface
     * @param height The height in pixels of the underlysing surface
     */
    virtual void setViewport(int width, int height) = 0;

    /**
     * Prepares the renderer to draw a frame. This method must be invoked
     * at the beginning of each frame. When this method is invoked, the
     * entire drawing surface is assumed to be redrawn.
     *
     * @param opaque If true, the target surface is considered opaque
     *               and will not be cleared. If false, the target surface
     *               will be cleared
     */
    virtual status_t prepare(bool opaque) = 0;

    /**
     * Prepares the renderer to draw a frame. This method must be invoked
     * at the beginning of each frame. Only the specified rectangle of the
     * frame is assumed to be dirty. A clip will automatically be set to
     * the specified rectangle.
     *
     * @param left The left coordinate of the dirty rectangle
     * @param top The top coordinate of the dirty rectangle
     * @param right The right coordinate of the dirty rectangle
     * @param bottom The bottom coordinate of the dirty rectangle
     * @param opaque If true, the target surface is considered opaque
     *               and will not be cleared. If false, the target surface
     *               will be cleared in the specified dirty rectangle
     */
    virtual status_t prepareDirty(float left, float top, float right, float bottom,
            bool opaque) = 0;

    /**
     * Indicates the end of a frame. This method must be invoked whenever
     * the caller is done rendering a frame.
     */
    virtual void finish() = 0;

    /**
     * This method must be invoked before handing control over to a draw functor.
     * See callDrawGLFunction() for instance.
     *
     * This command must not be recorded inside display lists.
     */
    virtual void interrupt() = 0;

    /**
     * This method must be invoked after getting control back from a draw functor.
     *
     * This command must not be recorded inside display lists.
     */
    virtual void resume() = 0;

// ----------------------------------------------------------------------------
// Canvas state operations
// ----------------------------------------------------------------------------
    // Save (layer)
    virtual int getSaveCount() const = 0;
    virtual int save(int flags) = 0;
    virtual void restore() = 0;
    virtual void restoreToCount(int saveCount) = 0;

    int saveLayer(float left, float top, float right, float bottom,
            const SkPaint* paint, int flags) {
        SkXfermode::Mode mode = SkXfermode::kSrcOver_Mode;
        int alpha = 255;
        if (paint) {
            mode = getXfermode(paint->getXfermode());
            alpha = paint->getAlpha();
        }
        return saveLayer(left, top, right, bottom, alpha, mode, flags);
    }
    int saveLayerAlpha(float left, float top, float right, float bottom,
            int alpha, int flags) {
        return saveLayer(left, top, right, bottom, alpha, SkXfermode::kSrcOver_Mode, flags);
    }
    virtual int saveLayer(float left, float top, float right, float bottom,
            int alpha, SkXfermode::Mode mode, int flags) = 0;

    // Matrix
    virtual void getMatrix(SkMatrix* outMatrix) const = 0;
    virtual void translate(float dx, float dy, float dz = 0.0f) = 0;
    virtual void rotate(float degrees) = 0;
    virtual void scale(float sx, float sy) = 0;
    virtual void skew(float sx, float sy) = 0;

    virtual void setMatrix(SkMatrix* matrix) = 0;
    virtual void concatMatrix(SkMatrix* matrix) = 0;

    // clip
    virtual const Rect& getClipBounds() const = 0;
    virtual bool quickRejectConservative(float left, float top,
            float right, float bottom) const = 0;
    virtual bool clipRect(float left, float top, float right, float bottom, SkRegion::Op op) = 0;
    virtual bool clipPath(SkPath* path, SkRegion::Op op) = 0;
    virtual bool clipRegion(SkRegion* region, SkRegion::Op op) = 0;

    // Misc - should be implemented with SkPaint inspection
    virtual void resetShader() = 0;
    virtual void setupShader(SkiaShader* shader) = 0;

    virtual void resetColorFilter() = 0;
    virtual void setupColorFilter(SkiaColorFilter* filter) = 0;

    virtual void resetShadow() = 0;
    virtual void setupShadow(float radius, float dx, float dy, int color) = 0;

    virtual void resetPaintFilter() = 0;
    virtual void setupPaintFilter(int clearBits, int setBits) = 0;

// ----------------------------------------------------------------------------
// Canvas draw operations
// ----------------------------------------------------------------------------
    virtual status_t drawColor(int color, SkXfermode::Mode mode) = 0;

    // Bitmap-based
    virtual status_t drawBitmap(SkBitmap* bitmap, float left, float top, SkPaint* paint) = 0;
    virtual status_t drawBitmap(SkBitmap* bitmap, SkMatrix* matrix, SkPaint* paint) = 0;
    virtual status_t drawBitmap(SkBitmap* bitmap, float srcLeft, float srcTop,
            float srcRight, float srcBottom, float dstLeft, float dstTop,
            float dstRight, float dstBottom, SkPaint* paint) = 0;
    virtual status_t drawBitmapData(SkBitmap* bitmap, float left, float top, SkPaint* paint) = 0;
    virtual status_t drawBitmapMesh(SkBitmap* bitmap, int meshWidth, int meshHeight,
            float* vertices, int* colors, SkPaint* paint) = 0;
    virtual status_t drawPatch(SkBitmap* bitmap, Res_png_9patch* patch,
            float left, float top, float right, float bottom, SkPaint* paint) = 0;

    // Shapes
    virtual status_t drawRect(float left, float top, float right, float bottom, SkPaint* paint) = 0;
    virtual status_t drawRects(const float* rects, int count, SkPaint* paint) = 0;
    virtual status_t drawRoundRect(float left, float top, float right, float bottom,
            float rx, float ry, SkPaint* paint) = 0;
    virtual status_t drawCircle(float x, float y, float radius, SkPaint* paint) = 0;
    virtual status_t drawOval(float left, float top, float right, float bottom, SkPaint* paint) = 0;
    virtual status_t drawArc(float left, float top, float right, float bottom,
            float startAngle, float sweepAngle, bool useCenter, SkPaint* paint) = 0;
    virtual status_t drawPath(SkPath* path, SkPaint* paint) = 0;
    virtual status_t drawLines(float* points, int count, SkPaint* paint) = 0;
    virtual status_t drawPoints(float* points, int count, SkPaint* paint) = 0;

    // Text
    virtual status_t drawText(const char* text, int bytesCount, int count, float x, float y,
            const float* positions, SkPaint* paint, float totalAdvance, const Rect& bounds,
            DrawOpMode drawOpMode = kDrawOpMode_Immediate) = 0;
    virtual status_t drawTextOnPath(const char* text, int bytesCount, int count, SkPath* path,
            float hOffset, float vOffset, SkPaint* paint) = 0;
    virtual status_t drawPosText(const char* text, int bytesCount, int count,
            const float* positions, SkPaint* paint) = 0;

// ----------------------------------------------------------------------------
// Canvas draw operations - special
// ----------------------------------------------------------------------------
    virtual status_t drawLayer(Layer* layer, float x, float y) = 0;
    virtual status_t drawDisplayList(DisplayList* displayList, Rect& dirty,
            int32_t replayFlags) = 0;

    // TODO: rename for consistency
    virtual status_t callDrawGLFunction(Functor* functor, Rect& dirty) = 0;

private:
    // Optional name of the renderer
    String8 mName;
}; // class Renderer

}; // namespace uirenderer
}; // namespace android

#endif // ANDROID_HWUI_RENDERER_H
