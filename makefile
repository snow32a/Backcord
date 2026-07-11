CC      = i686-w64-mingw32-gcc
WINDRES = i686-w64-mingw32-windres

SRCS    = *.c */*.c
RES     = rc/res.o

Backcord: $(RES)
	$(WINDRES) rc/res.rc -o rc/res.o
	$(CC) $(SRCS) $(RES) \
		-D _WIN32_WINNT=0x401 -D WINVER=0x401 -Wl,-Bstatic -lpng -lz \
		-Wl,-Bdynamic -lgdi32 -lssl -lcrypto -lcrypt32 -lws2_32 \
		-luser32 -lcomctl32 -lmsvcrt -lcomdlg32 -lcjson -lntdll \
		-o Backcord.exe

clean:
	rm -f rc/res.o Backcord.exe