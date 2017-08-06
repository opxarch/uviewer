# GNU Makefile for third party libraries used by uView
#
# If libs libraries are supplied, they will be built as
# static libraries.
#
# Use 'git submodule init' and 'git submodule update' to check
# out the libs libraries from git.

FREETYPE_DIR := libs/freetype
HARFBUZZ_DIR := libs/harfbuzz
JBIG2DEC_DIR := libs/jbig2dec
JPEG_DIR := libs/jpeg
MUJS_DIR := libs/mujs
OPENJPEG_DIR := libs/openjpeg/libopenjpeg
ZLIB_DIR := libs/zlib

CURL_DIR := libs/curl
GLFW_DIR := libs/glfw

# --- MuJS ---

ifneq "$(wildcard $(MUJS_DIR)/README)" ""

MUJS_OUT := $(OUT)/mujs
MUJS_SRC := one.c

MUJS_OBJ := $(addprefix $(MUJS_OUT)/, $(MUJS_SRC:%.c=%.o))

$(MUJS_OUT)/one.o: $(wildcard $(MUJS_DIR)/js*.c $(MUJS_DIR)/utf*.c $(MUJS_DIR)/regex.c $(MUJS_DIR)/*.h)

$(MUJS_OUT):
	$(MKDIR_CMD)
$(MUJS_OUT)/%.o: $(MUJS_DIR)/%.c | $(MUJS_OUT)
	$(CC_CMD)

MUJS_CFLAGS := -I$(MUJS_DIR)

HAVE_MUJS := yes
endif

# --- FreeType 2 ---

ifneq "$(wildcard $(FREETYPE_DIR)/README)" ""

FREETYPE_OUT := $(OUT)/freetype
FREETYPE_SRC := \
	ftbase.c \
	ftbbox.c \
	ftbitmap.c \
	ftdebug.c \
	ftfntfmt.c \
	ftgasp.c \
	ftglyph.c \
	ftinit.c \
	ftstroke.c \
	ftsynth.c \
	ftsystem.c \
	fttype1.c \
	cff.c \
	psaux.c \
	pshinter.c \
	psnames.c \
	raster.c \
	sfnt.c \
	smooth.c \
	truetype.c \
	type1.c \
	type1cid.c \

FREETYPE_OBJ := $(addprefix $(FREETYPE_OUT)/, $(FREETYPE_SRC:%.c=%.o))

$(FREETYPE_OUT):
	$(MKDIR_CMD)

FT_CFLAGS := -DFT2_BUILD_LIBRARY -DDARWIN_NO_CARBON \
	'-DFT_CONFIG_MODULES_H="slimftmodules.h"' \
	'-DFT_CONFIG_OPTIONS_H="slimftoptions.h"'

$(FREETYPE_OUT)/%.o: $(FREETYPE_DIR)/src/base/%.c	| $(FREETYPE_OUT)
	$(CC_CMD) $(FT_CFLAGS)
$(FREETYPE_OUT)/%.o: $(FREETYPE_DIR)/src/cff/%.c	| $(FREETYPE_OUT)
	$(CC_CMD) $(FT_CFLAGS)
$(FREETYPE_OUT)/%.o: $(FREETYPE_DIR)/src/cid/%.c	| $(FREETYPE_OUT)
	$(CC_CMD) $(FT_CFLAGS)
$(FREETYPE_OUT)/%.o: $(FREETYPE_DIR)/src/psaux/%.c	| $(FREETYPE_OUT)
	$(CC_CMD) $(FT_CFLAGS)
$(FREETYPE_OUT)/%.o: $(FREETYPE_DIR)/src/pshinter/%.c	| $(FREETYPE_OUT)
	$(CC_CMD) $(FT_CFLAGS)
$(FREETYPE_OUT)/%.o: $(FREETYPE_DIR)/src/psnames/%.c	| $(FREETYPE_OUT)
	$(CC_CMD) $(FT_CFLAGS)
$(FREETYPE_OUT)/%.o: $(FREETYPE_DIR)/src/raster/%.c	| $(FREETYPE_OUT)
	$(CC_CMD) $(FT_CFLAGS)
$(FREETYPE_OUT)/%.o: $(FREETYPE_DIR)/src/smooth/%.c	| $(FREETYPE_OUT)
	$(CC_CMD) $(FT_CFLAGS)
$(FREETYPE_OUT)/%.o: $(FREETYPE_DIR)/src/sfnt/%.c	| $(FREETYPE_OUT)
	$(CC_CMD) $(FT_CFLAGS)
$(FREETYPE_OUT)/%.o: $(FREETYPE_DIR)/src/truetype/%.c	| $(FREETYPE_OUT)
	$(CC_CMD) $(FT_CFLAGS)
$(FREETYPE_OUT)/%.o: $(FREETYPE_DIR)/src/type1/%.c	| $(FREETYPE_OUT)
	$(CC_CMD) $(FT_CFLAGS)

FREETYPE_CFLAGS := -Iscripts/freetype -I$(FREETYPE_DIR)/include
else
FREETYPE_CFLAGS := $(SYS_FREETYPE_CFLAGS)
FREETYPE_LIBS := $(SYS_FREETYPE_LIBS)
endif


# --- HarfBuzz ---

ifneq "$(wildcard $(HARFBUZZ_DIR)/README)" ""

HARFBUZZ_OUT := $(OUT)/harfbuzz
HARFBUZZ_SRC := \
	hb-blob.cc \
	hb-buffer.cc \
	hb-buffer-serialize.cc \
	hb-common.cc \
	hb-face.cc \
	hb-fallback-shape.cc \
	hb-font.cc \
	hb-ft.cc \
	hb-ot-font.cc \
	hb-ot-layout.cc \
	hb-ot-map.cc \
	hb-ot-shape-complex-arabic.cc \
	hb-ot-shape-complex-default.cc \
	hb-ot-shape-complex-hangul.cc \
	hb-ot-shape-complex-hebrew.cc \
	hb-ot-shape-complex-indic-table.cc \
	hb-ot-shape-complex-indic.cc \
	hb-ot-shape-complex-myanmar.cc \
	hb-ot-shape-complex-thai.cc \
	hb-ot-shape-complex-tibetan.cc \
	hb-ot-shape-complex-use-table.cc \
	hb-ot-shape-complex-use.cc \
	hb-ot-shape-fallback.cc \
	hb-ot-shape-normalize.cc \
	hb-ot-shape.cc \
	hb-ot-tag.cc \
	hb-set.cc \
	hb-shape-plan.cc \
	hb-shape.cc \
	hb-shaper.cc \
	hb-ucdn.cc \
	hb-unicode.cc \
	hb-warning.cc

#	hb-coretext.cc
#	hb-directwrite.cc
#	hb-glib.cc
#	hb-gobject-structs.cc
#	hb-graphite2.cc
#	hb-icu.cc
#	hb-uniscribe.cc

HARFBUZZ_OBJ := $(addprefix $(HARFBUZZ_OUT)/, $(HARFBUZZ_SRC:%.cc=%.o))

$(HARFBUZZ_OUT):
	$(MKDIR_CMD)
$(HARFBUZZ_OUT)/%.o: $(HARFBUZZ_DIR)/src/%.cc | $(HARFBUZZ_OUT)
	$(CC_CMD) -I$(HARFBUZZ_DIR)/src/hb-ucdn -DHAVE_OT -DHAVE_UCDN -DHB_NO_MT $(FREETYPE_CFLAGS) \
		-Dhb_malloc_impl=hb_malloc -Dhb_calloc_impl=hb_calloc \
		-Dhb_free_impl=hb_free -Dhb_realloc_impl=hb_realloc \
		-fno-rtti -fno-exceptions -fvisibility-inlines-hidden --std=c++0x

HARFBUZZ_CFLAGS := -I$(HARFBUZZ_DIR)/src
else
HARFBUZZ_CFLAGS := $(SYS_HARFBUZZ_CFLAGS)
HARFBUZZ_LIBS := $(SYS_HARFBUZZ_LIBS)
endif

# --- JBIG2DEC ---

ifneq "$(wildcard $(JBIG2DEC_DIR)/README)" ""

JBIG2DEC_OUT := $(OUT)/jbig2dec
JBIG2DEC_SRC := \
	jbig2.c \
	jbig2_arith.c \
	jbig2_arith_iaid.c \
	jbig2_arith_int.c \
	jbig2_generic.c \
	jbig2_halftone.c \
	jbig2_huffman.c \
	jbig2_image.c \
	jbig2_metadata.c \
	jbig2_mmr.c \
	jbig2_page.c \
	jbig2_refinement.c \
	jbig2_segment.c \
	jbig2_symbol_dict.c \
	jbig2_text.c \

JBIG2DEC_OBJ := $(addprefix $(JBIG2DEC_OUT)/, $(JBIG2DEC_SRC:%.c=%.o))

$(JBIG2DEC_OUT):
	$(MKDIR_CMD)
$(JBIG2DEC_OUT)/%.o: $(JBIG2DEC_DIR)/%.c | $(JBIG2DEC_OUT)
	$(CC_CMD) -DHAVE_STDINT_H -DJBIG_EXTERNAL_MEMENTO_H=\"uview/memento.h\"

JBIG2DEC_CFLAGS := -I$(JBIG2DEC_DIR)
else
JBIG2DEC_CFLAGS := $(SYS_JBIG2DEC_CFLAGS)
JBIG2DEC_LIBS := $(SYS_JBIG2DEC_LIBS)
endif

# --- JPEG library from IJG ---

ifneq "$(wildcard $(JPEG_DIR)/README)" ""

JPEG_OUT := $(OUT)/jpeg
JPEG_SRC := \
	jaricom.c \
	jcomapi.c \
	jdapimin.c \
	jdapistd.c \
	jdarith.c \
	jdatadst.c \
	jdatasrc.c \
	jdcoefct.c \
	jdcolor.c \
	jddctmgr.c \
	jdhuff.c \
	jdinput.c \
	jdmainct.c \
	jdmarker.c \
	jdmaster.c \
	jdmerge.c \
	jdpostct.c \
	jdsample.c \
	jdtrans.c \
	jerror.c \
	jfdctflt.c \
	jfdctfst.c \
	jfdctint.c \
	jidctflt.c \
	jidctfst.c \
	jidctint.c \
	jmemmgr.c \
	jquant1.c \
	jquant2.c \
	jutils.c \

JPEG_OBJ := $(addprefix $(JPEG_OUT)/, $(JPEG_SRC:%.c=%.o))

$(JPEG_OUT):
	$(MKDIR_CMD)
$(JPEG_OUT)/%.o: $(JPEG_DIR)/%.c | $(JPEG_OUT)
	$(CC_CMD) -Dmain=xxxmain

JPEG_CFLAGS := -Iscripts/jpeg -I$(JPEG_DIR)
else
JPEG_CFLAGS := $(SYS_JPEG_CFLAGS) -DSHARE_JPEG
JPEG_LIBS := $(SYS_JPEG_LIBS)
endif

# --- OpenJPEG ---

ifneq "$(wildcard $(OPENJPEG_DIR)/README.md)" ""

OPENJPEG_OUT := $(OUT)/openjpeg
OPENJPEG_SRC := \
	bio.c \
	cidx_manager.c \
	cio.c \
	dwt.c \
	event.c \
	function_list.c \
	image.c \
	invert.c \
	j2k.c \
	jp2.c \
	mct.c \
	mqc.c \
	openjpeg.c \
	opj_clock.c \
	phix_manager.c \
	pi.c \
	ppix_manager.c \
	raw.c \
	t1.c \
	t2.c \
	tcd.c \
	tgt.c \
	thix_manager.c \
	tpix_manager.c \

OPENJPEG_OBJ := $(addprefix $(OPENJPEG_OUT)/, $(OPENJPEG_SRC:%.c=%.o))

$(OPENJPEG_OUT):
	$(MKDIR_CMD)
$(OPENJPEG_OUT)/%.o: $(OPENJPEG_DIR)/%.c | $(OPENJPEG_OUT)
	$(CC_CMD) -DOPJ_STATIC -DOPJ_HAVE_STDINT_H

OPENJPEG_CFLAGS += -Iscripts/openjpeg -I$(OPENJPEG_DIR)
else
OPENJPEG_CFLAGS := $(SYS_OPENJPEG_CFLAGS)
OPENJPEG_LIBS := $(SYS_OPENJPEG_LIBS)
endif

# --- ZLIB ---

ifneq "$(wildcard $(ZLIB_DIR)/README)" ""

ZLIB_OUT := $(OUT)/zlib
ZLIB_SRC := \
	adler32.c \
	compress.c \
	crc32.c \
	deflate.c \
	inffast.c \
	inflate.c \
	inftrees.c \
	trees.c \
	uncompr.c \
	zutil.c \
	gzlib.c \
	gzwrite.c \
	gzclose.c \
	gzread.c \

ZLIB_OBJ := $(addprefix $(ZLIB_OUT)/, $(ZLIB_SRC:%.c=%.o))

$(ZLIB_OUT):
	$(MKDIR_CMD)
$(ZLIB_OUT)/%.o: $(ZLIB_DIR)/%.c | $(ZLIB_OUT)
	$(CC_CMD) -Dverbose=-1 -DHAVE_UNISTD_H -DHAVE_STDARG_H

ZLIB_CFLAGS := -I$(ZLIB_DIR)
else
ZLIB_CFLAGS := $(SYS_ZLIB_CFLAGS)
ZLIB_LIBS := $(SYS_ZLIB_LIBS)
endif

# --- cURL ---

ifneq "$(wildcard $(CURL_DIR)/README)" ""

CURL_LIB := $(OUT)/libcurl.a
CURL_OUT := $(OUT)/curl
CURL_SRC := \
	amigaos.c \
	asyn-ares.c \
	asyn-thread.c \
	axtls.c \
	base64.c \
	bundles.c \
	conncache.c \
	connect.c \
	content_encoding.c \
	cookie.c \
	curl_addrinfo.c \
	curl_darwinssl.c \
	curl_fnmatch.c \
	curl_gethostname.c \
	curl_gssapi.c \
	curl_memrchr.c \
	curl_multibyte.c \
	curl_ntlm.c \
	curl_ntlm_core.c \
	curl_ntlm_msgs.c \
	curl_ntlm_wb.c \
	curl_rand.c \
	curl_rtmp.c \
	curl_sasl.c \
	curl_schannel.c \
	curl_sspi.c \
	curl_threads.c \
	cyassl.c \
	dict.c \
	easy.c \
	escape.c \
	file.c \
	fileinfo.c \
	formdata.c \
	ftp.c \
	ftplistparser.c \
	getenv.c \
	getinfo.c \
	gopher.c \
	gtls.c \
	hash.c \
	hmac.c \
	hostasyn.c \
	hostcheck.c \
	hostip.c \
	hostip4.c \
	hostip6.c \
	hostsyn.c \
	http.c \
	http_chunks.c \
	http_digest.c \
	http_negotiate.c \
	http_negotiate_sspi.c \
	http_proxy.c \
	idn_win32.c \
	if2ip.c \
	imap.c \
	inet_ntop.c \
	inet_pton.c \
	krb4.c \
	krb5.c \
	ldap.c \
	llist.c \
	md4.c \
	md5.c \
	memdebug.c \
	mprintf.c \
	multi.c \
	netrc.c \
	non-ascii.c \
	nonblock.c \
	nss.c \
	openldap.c \
	parsedate.c \
	pingpong.c \
	pipeline.c \
	polarssl.c \
	polarssl_threadlock.c \
	pop3.c \
	progress.c \
	qssl.c \
	rawstr.c \
	rtsp.c \
	security.c \
	select.c \
	sendf.c \
	share.c \
	slist.c \
	smtp.c \
	socks.c \
	socks_gssapi.c \
	speedcheck.c \
	splay.c \
	ssh.c \
	sslgen.c \
	ssluse.c \
	strdup.c \
	strequal.c \
	strerror.c \
	strtok.c \
	strtoofft.c \
	telnet.c \
	tftp.c \
	timeval.c \
	transfer.c \
	url.c \
	version.c \
	warnless.c \
	wildcard.c \

$(CURL_LIB): $(addprefix $(CURL_OUT)/, $(CURL_SRC:%.c=%.o))

$(CURL_OUT):
	$(MKDIR_CMD)

CRL_CFLAGS := -DHAVE_CONFIG_H -DBUILDING_LIBCURL -DCURL_STATICLIB \
	-DCURL_DISABLE_LDAP -I$(CURL_DIR)/include

$(CURL_OUT)/%.o: $(CURL_DIR)/lib/%.c	| $(CURL_OUT)
	$(CC_CMD) $(CRL_CFLAGS)

CURL_CFLAGS := -I$(CURL_DIR)/include
CURL_LIBS := $(SYS_CURL_DEPS)

HAVE_CURL := yes

else ifeq "$(HAVE_CURL)" "yes"
CURL_CFLAGS := $(SYS_CURL_CFLAGS)
CURL_LIBS := $(SYS_CURL_LIBS)
endif

# --- GLFW ---

ifneq "$(wildcard $(GLFW_DIR)/README.md)" ""

GLFW_LIB := $(OUT)/libglfw.a
GLFW_OUT := $(OUT)/glfw
GLFW_SRC := \
	context.c \
	glx_context.c \
	init.c \
	input.c \
	linux_joystick.c \
	monitor.c \
	posix_time.c \
	posix_tls.c \
	window.c \
	x11_init.c \
	x11_monitor.c \
	x11_window.c \
	xkb_unicode.c \

GLFW_SRC_UNUSED := \
	egl_context.c \
	mach_time.c \
	mir_init.c \
	mir_monitor.c \
	mir_window.c \
	win32_init.c \
	win32_monitor.c \
	win32_time.c \
	win32_tls.c \
	win32_window.c \
	winmm_joystick.c \
	wgl_context.c \
	wl_init.c \
	wl_monitor.c \
	wl_window.c \

$(GLFW_LIB): $(addprefix $(GLFW_OUT)/, $(GLFW_SRC:%.c=%.o))
$(GLFW_OUT):
	$(MKDIR_CMD)
$(GLFW_OUT)/%.o: $(GLFW_DIR)/src/%.c | $(GLFW_OUT)
	$(CC_CMD) -D_GLFW_X11 -D_GLFW_GLX -D_GLFW_USE_OPENGL -D_GLFW_HAS_GLXGETPROCADDRESS

GLFW_CFLAGS := -I$(GLFW_DIR)/include
GLFW_LIBS := -lGL -lX11 -lXcursor -lXrandr -lXinerama -lpthread

HAVE_GLFW := yes

else ifeq "$(HAVE_GLFW)" "yes"
GLFW_CFLAGS := $(SYS_GLFW_CFLAGS)
GLFW_LIBS := $(SYS_GLFW_LIBS)
endif

# --- X11 ---

ifeq "$(HAVE_X11)" "yes"
X11_CFLAGS := $(SYS_X11_CFLAGS)
X11_LIBS := $(SYS_X11_LIBS)
endif
