/*
 * Copyright (C) 2018 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CustomPaintCanvas.h"

#if ENABLE(CSS_PAINTING_API)

#include "ImageBitmap.h"
#include "PaintRenderingContext2D.h"

namespace WebCore {

Ref<CustomPaintCanvas> CustomPaintCanvas::create(ScriptExecutionContext& context, unsigned width, unsigned height)
{
    return adoptRef(*new CustomPaintCanvas(context, width, height));
}

CustomPaintCanvas::CustomPaintCanvas(ScriptExecutionContext& context, unsigned width, unsigned height)
    : CanvasBase(&context)
    , m_size(width, height)
{
}

CustomPaintCanvas::~CustomPaintCanvas()
{
    notifyObserversCanvasDestroyed();

    m_context = nullptr; // Ensure this goes away before the ImageBuffer.
}

unsigned CustomPaintCanvas::width() const
{
    return m_size.width();
}

void CustomPaintCanvas::setWidth(unsigned newWidth)
{
    return m_size.setWidth(newWidth);
}

unsigned CustomPaintCanvas::height() const
{
    return m_size.height();
}

void CustomPaintCanvas::setHeight(unsigned newHeight)
{
    return m_size.setHeight(newHeight);
}

const IntSize& CustomPaintCanvas::size() const
{
    return m_size;
}

void CustomPaintCanvas::setSize(const IntSize& newSize)
{
    m_size = newSize;
}

ExceptionOr<RefPtr<PaintRenderingContext2D>> CustomPaintCanvas::getContext()
{
    if (m_context)
        return { RefPtr<PaintRenderingContext2D> { &downcast<PaintRenderingContext2D>(*m_context) } };

    m_context = PaintRenderingContext2D::create(*this);
    downcast<PaintRenderingContext2D>(*m_context).setUsesDisplayListDrawing(true);
    downcast<PaintRenderingContext2D>(*m_context).setTracksDisplayListReplay(false);

    return { RefPtr<PaintRenderingContext2D> { &downcast<PaintRenderingContext2D>(*m_context) } };
}

Image* CustomPaintCanvas::copiedImage() const
{
    ASSERT(!m_destinationGraphicsContext);
    if (!width() || !height())
        return nullptr;

    m_copiedBuffer = ImageBuffer::create(size(), Unaccelerated, 1, ColorSpaceSRGB, nullptr);
    if (!m_copiedBuffer)
        return nullptr;

    m_destinationGraphicsContext = &m_copiedBuffer->context();
    if (m_context)
        m_context->paintRenderingResultsToCanvas();
    m_destinationGraphicsContext = nullptr;

    m_copiedImage = m_copiedBuffer->copyImage(m_copiedBuffer->fastCopyImageMode(), PreserveResolution::Yes);
    return m_copiedImage.get();
}

GraphicsContext* CustomPaintCanvas::drawingContext() const
{
    return m_destinationGraphicsContext;
}

GraphicsContext* CustomPaintCanvas::existingDrawingContext() const
{
    return drawingContext();
}

void CustomPaintCanvas::makeRenderingResultsAvailable()
{
    if (m_context)
        m_context->paintRenderingResultsToCanvas();
}

}
#endif
