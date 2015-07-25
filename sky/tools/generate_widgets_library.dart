import 'dart:io';

main() async {
  var srcDirectory = Platform.script.path.replaceFirst('git /tools/generate_widgets_library.dart', '');
  final relativeWidgetPath = '$srcDirectory/sdk/lib/widgets';
  var directory = new Directory(relativeWidgetPath);
  print(directory.absolute.path);
  if (await directory.exists()) {
    var widgetsFile = new File(relativeWidgetPath + '/widgets.dart');
    if (await widgetsFile.exists()) {
      await widgetsFile.delete();
      await widgetsFile.create();
    }

    var dartFiles = await directory
    .list()
    .where((fse) => fse.path.endsWith(".dart"))
    .map((fse) => "export '${fse.uri.pathSegments.last}';")
    .toList();

    var handle = widgetsFile.openWrite();
    handle.writeAll(dartFiles, "\n");
    await handle.close();
  }
}
