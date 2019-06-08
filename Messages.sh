# call xgettext on all source files. If your sources have other filename
# extensions besides .cc, .cpp, and .h, just add them in the find call.
$XGETTEXT $(find . -name \*.cc -o -name \*.cpp -o -name \*.h -o -name \*.qml) -o $podir/plasma-camera.pot
