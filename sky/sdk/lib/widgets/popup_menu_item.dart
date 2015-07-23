// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
part of sky.widgets;

const double kMenuItemHeight = 48.0;
const double kBaselineOffsetFromBottom = 20.0;

class PopupMenuItem extends Component {
  PopupMenuItem({
    Key key,
    this.onPressed,
    this.child
  }) : super(key: key);

  final Widget child;
  final Function onPressed;

  TextStyle get textStyle => Theme.of(this).text.subhead;

  Widget build() {
    return new Listener(
      onGestureTap: (_) {
        if (onPressed != null)
          onPressed();
      },
      child: new InkWell(
        child: new Container(
          height: kMenuItemHeight,
          child: new DefaultTextStyle(
            style: textStyle,
            child: new Baseline(
              baseline: kMenuItemHeight - kBaselineOffsetFromBottom,
              child: child
            )
          )
        )
      )
    );
  }
}
