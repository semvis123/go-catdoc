package gocatdoc

import (
	"fmt"
	"io"
	"io/fs"
	"time"
)

// This file includes a minimal fake FS, it is used to convert the io.ReadSeeker to a fs.FS for Wazero

type fakeFileInfo struct {
	isDir bool
}

func (fi fakeFileInfo) Name() string {
	return ""
}

func (fi fakeFileInfo) Size() int64 {
	return 0
}

func (fi fakeFileInfo) Mode() fs.FileMode {
	if fi.isDir {
		return fs.ModeDir | 0555
	}
	return 0444
}

func (fi fakeFileInfo) ModTime() time.Time {
	return time.Now()
}

func (fi fakeFileInfo) IsDir() bool {
	return fi.isDir
}

func (fi fakeFileInfo) Sys() any {
	return nil
}

type fakeFS struct {
	reader io.ReadSeeker
	file   bool
}

func newFakeFS(reader io.ReadSeeker) (fakeFS, error) {
	_, err := reader.Seek(0, io.SeekStart)
	return fakeFS{reader, false}, err
}

func (s fakeFS) Open(filename string) (fs.File, error) {
	if filename == "." {
		return s, nil
	}

	s.file = true
	return s, nil
}

func (f fakeFS) Read(p []byte) (int, error) {
	return f.reader.Read(p)
}

func (f fakeFS) Seek(offset int64, whence int) (int64, error) {
	return f.reader.Seek(offset, whence)
}

func (f fakeFS) Stat() (fs.FileInfo, error) {
	fi := fakeFileInfo{
		isDir: !f.file,
	}
	return fi, nil
}

func (f fakeFS) Close() error {
	return fmt.Errorf("not implemented")
}
