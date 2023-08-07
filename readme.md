## Go-catdoc, get text and metadata from .doc files.
[![GoDoc](https://godoc.org/github.com/semvis123/go-catdoc?status.svg)](https://godoc.org/github.com/semvis123/go-catdoc)
[![Tests](https://github.com/semvis123/go-catdoc/actions/workflows/go.yml/badge.svg)](https://github.com/semvis123/go-catdoc/actions/workflows/go.yml)

Uses Wazero to run catdoc as webassembly in Go.
The catdoc source is slightly modified to support reading metadata in `.doc`.
The msdoc.hexpat file is a pattern file for imhex that can parse the `summaryinformation` ole object inside the `.doc` file.

To compile the webassembly binary, go to ./catdoc/src/ and run `make catdoc-wasm`.
To run the tests, do `go test ./...`

Usage:
```
f, err := os.Open("test.doc")
text, err := gocatdoc.GetTextFromFile(f)
```
