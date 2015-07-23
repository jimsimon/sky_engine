// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
part of sky.widgets;

class Theme extends Inherited {

  Theme({
    Key key,
    this.data,
    Widget child
  }) : super(key: key, child: child) {
    assert(child != null);
    assert(data != null);
  }

  final ThemeData data;

  static final ThemeData _kFallbackTheme = new ThemeData.fallback();

  static ThemeData of(Component component) {
    Theme theme = component.inheritedOfType(Theme);
    return theme == null ? _kFallbackTheme : theme.data;
  }

  bool syncShouldNotify(Theme old) => data != old.data;

}
