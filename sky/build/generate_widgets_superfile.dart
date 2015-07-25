#!/usr/bin/env dart

import 'dart:io';

const RELATIVE_WIDGETS_FOLDER_PATH = 'sky/sdk/lib/widgets';
const WIDGETS_SUPERFILE_NAME = 'widgets.dart';
const WIDGETS_FILE_EXTENSION = '.dart';

main() async {
  var projectRootPath = _getProjectRootPath();
  var outputFilePath = '$projectRootPath/$RELATIVE_WIDGETS_FOLDER_PATH/$WIDGETS_SUPERFILE_NAME';

  await generateSuperFile(outputFilePath);
}

generateSuperFile(String outputFilePath) async {

  var projectRootPath = _getProjectRootPath();
  var widgetsFolderPath = '$projectRootPath/$RELATIVE_WIDGETS_FOLDER_PATH';

  var outputFile = new File(outputFilePath);
  if (await outputFile.exists()) {
    await outputFile.delete();
  }

  var dartFiles = await _collectWidgetFileNames(widgetsFolderPath);
  await _writeWidgetsSuperfile(outputFile, dartFiles);
}

_getProjectRootPath() {
  return new File(Platform.script.path).parent.parent.parent.path;
}

_collectWidgetFileNames(String widgetsFolderPath) async {
  return await new Directory(widgetsFolderPath)
  .list()
  .where((fse) => fse.path.endsWith(WIDGETS_FILE_EXTENSION) && !fse.path.endsWith(WIDGETS_SUPERFILE_NAME))
  .map((fse) => "export '${fse.uri.pathSegments.last}';")
  .toList();
}

_writeWidgetsSuperfile(File outputFile, dartFiles) async {
  var handle = outputFile.openWrite();
  handle.writeAll(dartFiles, "\n");
  await handle.close();
}

