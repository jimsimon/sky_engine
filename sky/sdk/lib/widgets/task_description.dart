// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
part of sky.widgets;

class TaskDescription extends Component {

  TaskDescription({ this.label, this.child });

  final Widget child;
  final String label;

  Widget build() {
    activity.updateTaskDescription(label, Theme.of(this).primaryColor);
    return child;
  }

}
