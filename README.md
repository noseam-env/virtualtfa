# VirtualTFA

This library is an implementation of a virtual write and read in parts archive format TFA (Transfer File Archive). This
technology allows you to comfortably stream archives over the network without *temp files*, as well as read streams of archives without
buffering. It is needed to completely eliminate temporary files and accelerate writing and reading. The virtualization
essentially reads file data directly from the disk into the final buffer, but in between it includes its headers.

Example usage:

```c
// TODO
```

## TFA Specification

### Header

Size: `42 bytes`

| Field    | Size | Pos   | Description                                                                               |
|----------|------|-------|-------------------------------------------------------------------------------------------|
| magic    | 4    | 0-3   | magic field, value `tfa1`                                                                 |
| version  | 1    | 4     | tfa version, currently `0`                                                                |
| typeflag | 1    | 5     | currently unused, designed to specify the file type, for example symbolic link            |
| unused   | 4    | 6-9   | reserved bytes, it will probably be used for hash in the future                           |
| mode     | 4    | 10-13 | file permissions (Big-endian signed 32-bit integer)                                       |
| ctime    | 8    | 14-21 | file creation UNIX time (Big-endian unsigned 64-bit integer)                              |
| mtime    | 8    | 22-29 | file last modification UNIX time (Big-endian unsigned 64-bit integer)                     |
| namesize | 4    | 30-33 | size of file name in bytes (without null terminator) (Big-endian unsigned 32-bit integer) |
| filesize | 8    | 34-41 | size of file data (Big-endian unsigned 64-bit integer)                                    |

### Structure

| Header   | File Name       | File Data       | Header   | File Name       |     |
|----------|-----------------|-----------------|----------|-----------------|-----|
| 42 bytes | header.namesize | header.filesize | 42 bytes | header.namesize | ... |

## License

The library is licensed under the [MIT License](https://opensource.org/license/mit/):

Copyright (C) 2024 Michael Neonov <two.nelonn at gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

## Authors

- **Michael Neonov** ([email](mailto:two.nelonn@gmail.com), [github](https://github.com/Nelonn))
