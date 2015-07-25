#!/usr/bin/env dart

import 'dart:io';
import 'generate_widgets_superfile.dart' as generator;

const RELATIVE_ACTUAL_WIDGETS_FILE_PATH = 'sky/sdk/lib/widgets/widgets.dart';
const RELATIVE_GENERATED_WIDGETS_FILE_PATH = 'sky/build/widgets.dart';
const CONTENTS_MISMATCH_ERROR_MSG = 'widgets.dart does not contain all widgets.  Did you add a new widget and forget to update it?';

main() async {
  var projectRootPath = _getProjectRootPath();
  var actualWidgetsFilePath = '$projectRootPath/$RELATIVE_ACTUAL_WIDGETS_FILE_PATH';
  var generatedWidgetsFilePath = '$projectRootPath/$RELATIVE_GENERATED_WIDGETS_FILE_PATH';

  await generator.generateSuperFile(generatedWidgetsFilePath);

  String actualContents = await _getFileContents(actualWidgetsFilePath);
  String generatedContents = await _getFileContents(generatedWidgetsFilePath);

  if (actualContents != generatedContents) {
    throw new Exception(CONTENTS_MISMATCH_ERROR_MSG);
  }
}

_getProjectRootPath() {
  return new File(Platform.script.path).parent.parent.parent.path;
}

_getFileContents(filePath) async {
  File file = new File(filePath);
  return await file.readAsString();
}