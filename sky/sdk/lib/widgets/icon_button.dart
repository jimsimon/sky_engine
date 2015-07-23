// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
part of sky.widgets;

class IconButton extends Component {

  IconButton({ Key key, this.icon, this.onPressed, this.color }) : super(key: key);

  final String icon;
  final Function onPressed;
  final Color color;

  Widget build() {
    Widget child = new Icon(type: icon, size: 24);
    if (color != null) {
      child = new ColorFilter(
        color: color,
        transferMode: sky.TransferMode.srcATop,
        child: child
      );
    }
    return new Listener(
      child: new Padding(
        child: child,
        padding: const EdgeDims.all(8.0)),
      onGestureTap: (_) {
        if (onPressed != null)
          onPressed();
      }
    );
  }

}
