#include "pnghlp.h"
#include <libloaderapi.h>
#include <string.h>
#include <windows.h>
void PngReadBufCB(png_structp png, png_bytep out, png_size_t length) {
	PngBuffer *buf = png_get_io_ptr(png);
	memcpy(out, buf->data + buf->pos, length);
	buf->pos += length;
}
HDC LoadPNGImage(char *path) {
	HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
						   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (f == INVALID_HANDLE_VALUE) {
		MessageBoxA(NULL, "Error accessing the path!", "PNG Helper", 0);
		return NULL;
	}
	LARGE_INTEGER size;
	GetFileSizeEx(f, &size);

	size_t file_size = (size_t)size.QuadPart;

	uint8_t *buf = malloc(file_size);

	DWORD read = 0;

	ReadFile(f, buf, file_size, &read, NULL);

	png_structp png =
		png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info = png_create_info_struct(png);
	PngBuffer pngbuf = {buf, file_size, 0};

	png_set_read_fn(png, &pngbuf, PngReadBufCB);
	png_read_info(png, info);

	BITMAPINFO bmi = {0};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = png_get_image_width(png, info);
	bmi.bmiHeader.biHeight = -png_get_image_height(png, info); // top-down
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB; // THIS IS RAW - gordon ramsey
	if (setjmp(png_jmpbuf(png))) {
		png_destroy_read_struct(&png, &info, NULL);
		// err
		MessageBoxA(NULL, "zzzzzzzz", "zzzzzzzzz", 0);
		return NULL;
	}
	png_set_expand(png);
	png_set_strip_16(png);
	png_set_filler(png, 0xFF,
				   PNG_FILLER_AFTER); // ensures RGBA or RGBX cause winblows
	png_read_update_info(png, info);
	int width = png_get_image_width(png, info);
	int height = png_get_image_height(png, info);
	png_bytep *rows = malloc(height * sizeof(png_bytep));
	for (int y = 0; y < height; y++)
		rows[y] = malloc(png_get_rowbytes(png, info));
	// flat BGRA buffer for GDI
	uint8_t *fbuf = malloc(width * height * 4);
	png_read_image(png, rows);

	for (int y = 0; y < height; y++) {
		png_bytep src = rows[y];
		uint8_t *dst = fbuf + y * width * 4;

		for (int x = 0; x < width; x++) {
			dst[x * 4 + 0] = src[x * 4 + 2]; // B
			dst[x * 4 + 1] = src[x * 4 + 1]; // G
			dst[x * 4 + 2] = src[x * 4 + 0]; // R
			dst[x * 4 + 3] = src[x * 4 + 3]; // A
		}
	}
	void *gdibuf = NULL;
	HBITMAP bmp =
		CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &gdibuf, NULL, 0);
	// GDI allocates its own dumbahh buffer so we just memcpy to it
	memcpy(gdibuf, fbuf, width * height * 4);
	free(fbuf);
	HDC memdc = CreateCompatibleDC(NULL);
	SelectObject(memdc, bmp);
	CloseHandle(f);
	return memdc;
}
HBITMAP LoadPNGBitmap(char *path) {
	HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
						   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (f == INVALID_HANDLE_VALUE) {
		MessageBoxA(NULL, "Error accessing the path!", "PNG Helper", 0);
		return NULL;
	}
	LARGE_INTEGER size;
	GetFileSizeEx(f, &size);

	size_t file_size = (size_t)size.QuadPart;

	uint8_t *buf = malloc(file_size);

	DWORD read = 0;

	ReadFile(f, buf, file_size, &read, NULL);

	png_structp png =
		png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info = png_create_info_struct(png);
	PngBuffer pngbuf = {buf, file_size, 0};

	png_set_read_fn(png, &pngbuf, PngReadBufCB);
	png_read_info(png, info);

	BITMAPINFO bmi = {0};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = png_get_image_width(png, info);
	bmi.bmiHeader.biHeight = -png_get_image_height(png, info); // top-down
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB; // THIS IS RAW - gordon ramsey
	if (setjmp(png_jmpbuf(png))) {
		png_destroy_read_struct(&png, &info, NULL);
		// err
		MessageBoxA(NULL, "zzzzzzzz", "zzzzzzzzz", 0);
		return NULL;
	}
	png_set_expand(png);
	png_set_strip_16(png);
	png_set_filler(png, 0xFF,
				   PNG_FILLER_AFTER); // ensures RGBA or RGBX cause winblows
	png_read_update_info(png, info);
	int width = png_get_image_width(png, info);
	int height = png_get_image_height(png, info);
	png_bytep *rows = malloc(height * sizeof(png_bytep));
	for (int y = 0; y < height; y++)
		rows[y] = malloc(png_get_rowbytes(png, info));
	// flat BGRA buffer for GDI
	uint8_t *fbuf = malloc(width * height * 4);
	png_read_image(png, rows);

	for (int y = 0; y < height; y++) {
		png_bytep src = rows[y];
		uint8_t *dst = fbuf + y * width * 4;

		for (int x = 0; x < width; x++) {
			dst[x * 4 + 0] = src[x * 4 + 2]; // B
			dst[x * 4 + 1] = src[x * 4 + 1]; // G
			dst[x * 4 + 2] = src[x * 4 + 0]; // R
			dst[x * 4 + 3] = src[x * 4 + 3]; // A
		}
	}
	void *gdibuf = NULL;
	HBITMAP bmp =
		CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &gdibuf, NULL, 0);
	// GDI allocates its own dumbahh buffer so we just memcpy to it
	memcpy(gdibuf, fbuf, width * height * 4);
	free(fbuf);
	return bmp;
}
HDC LoadPNGImageFromBytes(char *bytes, int buflen) {
	png_structp png =
		png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info = png_create_info_struct(png);
	PngBuffer pngbuf = {bytes, buflen, 0};

	png_set_read_fn(png, &pngbuf, PngReadBufCB);
	png_read_info(png, info);

	BITMAPINFO bmi = {0};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = png_get_image_width(png, info);
	bmi.bmiHeader.biHeight = -png_get_image_height(png, info); // top-down
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB; // THIS IS RAW - gordon ramsey
	if (setjmp(png_jmpbuf(png))) {
		png_destroy_read_struct(&png, &info, NULL);
		// err
		MessageBoxA(NULL, "zzzzzzzz", "zzzzzzzzz", 0);
		return NULL;
	}
	png_set_expand(png);
	png_set_strip_16(png);
	png_set_filler(png, 0xFF,
				   PNG_FILLER_AFTER); // ensures RGBA or RGBX cause winblows
	png_read_update_info(png, info);
	int width = png_get_image_width(png, info);
	int height = png_get_image_height(png, info);
	png_bytep *rows = malloc(height * sizeof(png_bytep));
	for (int y = 0; y < height; y++)
		rows[y] = malloc(png_get_rowbytes(png, info));
	// flat BGRA buffer for GDI
	uint8_t *fbuf = malloc(width * height * 4);
	png_read_image(png, rows);

	for (int y = 0; y < height; y++) {
		png_bytep src = rows[y];
		uint8_t *dst = fbuf + y * width * 4;

		for (int x = 0; x < width; x++) {
			dst[x * 4 + 0] = src[x * 4 + 2]; // B
			dst[x * 4 + 1] = src[x * 4 + 1]; // G
			dst[x * 4 + 2] = src[x * 4 + 0]; // R
			dst[x * 4 + 3] = src[x * 4 + 3]; // A
		}
	}
	void *gdibuf = NULL;
	HBITMAP bmp =
		CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &gdibuf, NULL, 0);
	// GDI allocates its own dumbahh buffer so we just memcpy to it
	memcpy(gdibuf, fbuf, width * height * 4);
	free(fbuf);
	HDC memdc = CreateCompatibleDC(NULL);
	SelectObject(memdc, bmp);
	return memdc;
}

HDC LoadPNGImageFromResource(HMODULE hModule, int res) {
	HRSRC resf = FindResource(hModule, MAKEINTRESOURCE(res), RT_RCDATA);
	DWORD len = SizeofResource(hModule, resf);
	HGLOBAL resl = LoadResource(hModule, resf);
	void* resbuf = LockResource(resl);
	png_structp png =
		png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info = png_create_info_struct(png);
	PngBuffer pngbuf = {resbuf, len, 0};

	png_set_read_fn(png, &pngbuf, PngReadBufCB);
	png_read_info(png, info);

	BITMAPINFO bmi = {0};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = png_get_image_width(png, info);
	bmi.bmiHeader.biHeight = -png_get_image_height(png, info); // top-down
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB; // THIS IS RAW - gordon ramsey
	if (setjmp(png_jmpbuf(png))) {
		png_destroy_read_struct(&png, &info, NULL);
		// err
		MessageBoxA(NULL, "zzzzzzzz", "zzzzzzzzz", 0);
		return NULL;
	}
	png_set_expand(png);
	png_set_strip_16(png);
	png_set_filler(png, 0xFF,
				   PNG_FILLER_AFTER); // ensures RGBA or RGBX cause winblows
	png_read_update_info(png, info);
	int width = png_get_image_width(png, info);
	int height = png_get_image_height(png, info);
	png_bytep *rows = malloc(height * sizeof(png_bytep));
	for (int y = 0; y < height; y++)
		rows[y] = malloc(png_get_rowbytes(png, info));
	// flat BGRA buffer for GDI
	uint8_t *fbuf = malloc(width * height * 4);
	png_read_image(png, rows);

	for (int y = 0; y < height; y++) {
		png_bytep src = rows[y];
		uint8_t *dst = fbuf + y * width * 4;

		for (int x = 0; x < width; x++) {
			dst[x * 4 + 0] = src[x * 4 + 2]; // B
			dst[x * 4 + 1] = src[x * 4 + 1]; // G
			dst[x * 4 + 2] = src[x * 4 + 0]; // R
			dst[x * 4 + 3] = src[x * 4 + 3]; // A
		}
	}
	void *gdibuf = NULL;
	HBITMAP bmp =
		CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &gdibuf, NULL, 0);
	// GDI allocates its own dumbahh buffer so we just memcpy to it
	memcpy(gdibuf, fbuf, width * height * 4);
	free(fbuf);
	HDC memdc = CreateCompatibleDC(NULL);
	SelectObject(memdc, bmp);
	return memdc;
}
