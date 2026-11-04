package main

// #cgo LDFLAGS: -lm -O3
// #include <stdlib.h>
// #include <stdio.h>
// #include "vid.h"
import "C"
import (
	"fmt"
	"unsafe"
)

type Image struct {
	width    int
	height   int
	channels int
	data     []uint8
	pingus   []int64
}

func main() {
	cPath := C.CString("nsfw.png")
	defer C.free(unsafe.Pointer(cPath))

	image := C.loadImage(cPath)

	width := int(image.width)
	height := int(image.height)
	channels := int(image.channels)
	data := C.GoBytes(unsafe.Pointer(image.data), C.int(width*height*channels))

	goImage := Image{
		width:    width,
		height:   height,
		channels: channels,
		data:     data,
	}

	fmt.Println(goImage)
}
