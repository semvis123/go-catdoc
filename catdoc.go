package gocatdoc

import (
	"bytes"
	"context"
	"fmt"
	"io"
	"io/fs"
	"strings"
	"sync"

	"embed"

	"github.com/tetratelabs/wazero"
	"github.com/tetratelabs/wazero/api"
	"github.com/tetratelabs/wazero/imports/emscripten"
	"github.com/tetratelabs/wazero/imports/wasi_snapshot_preview1"
	"github.com/tetratelabs/wazero/sys"
)

//go:embed catdoc.wasm
var binary []byte

//go:embed charsets/*
var charsets embed.FS

var runtimeConfig wazero.RuntimeConfig
var r wazero.Runtime
var compiledModule wazero.CompiledModule
var ctx context.Context
var initLock = &sync.Mutex{}

func getWASMModuleWithFS(file fs.FS, stdout, stderr io.Writer) (api.Module, error) {
	cMod, run, err := getCompiledWASMModule()
	if err != nil {
		return nil, err
	}
	mod, err := run.InstantiateModule(ctx, cMod, wazero.NewModuleConfig().
		WithStartFunctions("_initialize").
		WithFSConfig(
			wazero.NewFSConfig().
				WithFSMount(file, "/input_file/").
				WithFSMount(charsets, "/")).
		WithStdout(stdout).WithStderr(stderr))
	return mod, err
}

func getCompiledWASMModule() (wazero.CompiledModule, wazero.Runtime, error) {
	initLock.Lock()
	defer initLock.Unlock()
	if r == nil {
		ctx = context.Background()

		if runtimeConfig == nil {
			cache := wazero.NewCompilationCache()
			runtimeConfig = wazero.NewRuntimeConfig().WithCompilationCache(cache)
		}

		r = wazero.NewRuntimeWithConfig(ctx, runtimeConfig)
		wasi_snapshot_preview1.MustInstantiate(ctx, r)

		if compiledModule == nil {
			module, err := r.CompileModule(ctx, binary)
			compiledModule = module
			if err != nil {
				return nil, nil, fmt.Errorf("failed to compile module: %w", err)
			}
		}
		_, err := emscripten.InstantiateForModule(ctx, r, compiledModule)
		if err != nil {
			return nil, nil, fmt.Errorf("failed to instantiate module (emscripten): %w", err)
		}
	}
	return compiledModule, r, nil
}

func GetAuthorFromFile(file io.ReadSeeker) (string, error) {
	return callWASMFuncWithFile("get_author", file)
}

func GetLastAuthorFromFile(file io.ReadSeeker) (string, error) {
	return callWASMFuncWithFile("get_last_author", file)
}

func GetTextFromFile(file io.ReadSeeker) (string, error) {
	return callWASMFuncWithFile("get_text", file)
}

func GetTitleFromFile(file io.ReadSeeker) (string, error) {
	return callWASMFuncWithFile("get_title", file)
}

func GetSubjectFromFile(file io.ReadSeeker) (string, error) {
	return callWASMFuncWithFile("get_subject", file)
}

func GetKeywordsFromFile(file io.ReadSeeker) (string, error) {
	return callWASMFuncWithFile("get_keywords", file)
}

func GetCommentsFromFile(file io.ReadSeeker) (string, error) {
	return callWASMFuncWithFile("get_comments", file)
}

func GetAnnotationAuthorsFromFile(file io.ReadSeeker) ([]string, error) {
	r, err := callWASMFuncWithFile("get_annotation_authors", file)
	if err != nil {
		return nil, err
	}
	return strings.Split(r, "\n"), nil
}

func GetVersion() (string, error) {
	return callWASMFunc("get_version", nil)
}

func callWASMFuncWithFile(funcName string, file io.ReadSeeker) (string, error) {
	fileFS, err := newFakeFS(file)
	if err != nil {
		return "", err
	}

	return callWASMFunc(funcName, fileFS)
}

func callWASMFunc(funcName string, fs fs.FS) (string, error) {
	outBuf := &bytes.Buffer{}
	errBuf := &bytes.Buffer{}
	mod, err := getWASMModuleWithFS(fs, outBuf, errBuf)
	if err != nil {
		return "", fmt.Errorf("could not get wasm module: %w", err)
	}
	_, err = mod.ExportedFunction(funcName).Call(ctx)
	if err != nil {
		if exitError, ok := err.(*sys.ExitError); ok && exitError.ExitCode() != 0 {
			return "", fmt.Errorf("%s %w", outBuf.String(), exitError)
		}
	}

	outStr := outBuf.String()
	errStr := errBuf.String()
	outStr = strings.TrimRight(outStr, "\n")
	errStr = strings.TrimRight(errStr, "\n")
	err = nil
	if errStr != "" {
		err = fmt.Errorf(errStr)
	}
	return outStr, err
}
