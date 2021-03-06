// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SKY_ENGINE_CORE_PAINTING_CANVASIMAGE_H_
#define SKY_ENGINE_CORE_PAINTING_CANVASIMAGE_H_

#include "sky/engine/platform/weborigin/KURL.h"
#include "sky/engine/tonic/dart_wrappable.h"
#include "sky/engine/wtf/PassRefPtr.h"
#include "sky/engine/wtf/text/AtomicString.h"
#include "third_party/skia/include/core/SkBitmap.h"

namespace blink {

class CanvasImage final : public RefCounted<CanvasImage>,
                          public DartWrappable {
  DEFINE_WRAPPERTYPEINFO();
 public:
  ~CanvasImage() override;
  static PassRefPtr<CanvasImage> create() { return adoptRef(new CanvasImage); }

  int width() const;
  int height() const;

  const SkBitmap& bitmap() const { return bitmap_; }
  void setBitmap(const SkBitmap& bitmap) { bitmap_ = bitmap; }

 private:
  CanvasImage();

  SkBitmap bitmap_;
};

}  // namespace blink

#endif  // SKY_ENGINE_CORE_PAINTING_CANVASIMAGE_H_
