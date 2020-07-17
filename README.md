# Darwin-0.3

## Building packages
* Place the debs from the Release tarball in a directory e.g. /build/repo
* Place the source files in a directory e.g. /build/source/

```
usage: /usr/bin/darwin-buildpackage [ --cvs | --dir ] [ --target {all|headers|objs|local} ] <source> <repository> <dstdir>
darwin-buildpackage --dir --target headers /build/source/kernel-7 /build/repo /build/built
```

## Building from the manifest file
* Current working directory must be directory containing source files e.g. /build/source

```
usage: /usr/bin/darwin-buildall <srclist> <repository> <dstdir>
darwin-buildall Manifest /build/repo /build/built
```
