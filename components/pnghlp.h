#include <libpng18/png.h>
#include <stdint.h>
#include <windows.h>
typedef struct {
	const uint8_t *data;
	size_t size;
	size_t pos;
} PngBuffer;
void PngReadBufCB(png_structp png, png_bytep out, png_size_t length);
HDC LoadPNGImage(char *path);
HDC LoadPNGImageFromBytes(char *bytes, int buflen);
HDC LoadPNGImageFromResource(HMODULE hModule, int res);
HBITMAP LoadPNGBitmap(char *path);