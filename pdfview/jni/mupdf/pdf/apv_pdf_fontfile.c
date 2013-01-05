#include "fitz-internal.h"
#include "mupdf-internal.h"

#ifndef APV_ASSET_FONTS

#ifdef NOCJK
#define NOCJKFONT
#endif

#include "../generated/font_base14.h"

#ifndef NODROIDFONT
#include "../generated/font_droid.h"
#endif

#ifndef NOCJKFONT
#include "../generated/font_cjk.h"
#endif

unsigned char *
pdf_lookup_builtin_font(char *name, unsigned int *len)
{
	if (!strcmp("Courier", name)) {
		*len = sizeof pdf_font_NimbusMonL_Regu;
		return (unsigned char*) pdf_font_NimbusMonL_Regu;
	}
	if (!strcmp("Courier-Bold", name)) {
		*len = sizeof pdf_font_NimbusMonL_Bold;
		return (unsigned char*) pdf_font_NimbusMonL_Bold;
	}
	if (!strcmp("Courier-Oblique", name)) {
		*len = sizeof pdf_font_NimbusMonL_ReguObli;
		return (unsigned char*) pdf_font_NimbusMonL_ReguObli;
	}
	if (!strcmp("Courier-BoldOblique", name)) {
		*len = sizeof pdf_font_NimbusMonL_BoldObli;
		return (unsigned char*) pdf_font_NimbusMonL_BoldObli;
	}
	if (!strcmp("Helvetica", name)) {
		*len = sizeof pdf_font_NimbusSanL_Regu;
		return (unsigned char*) pdf_font_NimbusSanL_Regu;
	}
	if (!strcmp("Helvetica-Bold", name)) {
		*len = sizeof pdf_font_NimbusSanL_Bold;
		return (unsigned char*) pdf_font_NimbusSanL_Bold;
	}
	if (!strcmp("Helvetica-Oblique", name)) {
		*len = sizeof pdf_font_NimbusSanL_ReguItal;
		return (unsigned char*) pdf_font_NimbusSanL_ReguItal;
	}
	if (!strcmp("Helvetica-BoldOblique", name)) {
		*len = sizeof pdf_font_NimbusSanL_BoldItal;
		return (unsigned char*) pdf_font_NimbusSanL_BoldItal;
	}
	if (!strcmp("Times-Roman", name)) {
		*len = sizeof pdf_font_NimbusRomNo9L_Regu;
		return (unsigned char*) pdf_font_NimbusRomNo9L_Regu;
	}
	if (!strcmp("Times-Bold", name)) {
		*len = sizeof pdf_font_NimbusRomNo9L_Medi;
		return (unsigned char*) pdf_font_NimbusRomNo9L_Medi;
	}
	if (!strcmp("Times-Italic", name)) {
		*len = sizeof pdf_font_NimbusRomNo9L_ReguItal;
		return (unsigned char*) pdf_font_NimbusRomNo9L_ReguItal;
	}
	if (!strcmp("Times-BoldItalic", name)) {
		*len = sizeof pdf_font_NimbusRomNo9L_MediItal;
		return (unsigned char*) pdf_font_NimbusRomNo9L_MediItal;
	}
	if (!strcmp("Symbol", name)) {
		*len = sizeof pdf_font_StandardSymL;
		return (unsigned char*) pdf_font_StandardSymL;
	}
	if (!strcmp("ZapfDingbats", name)) {
		*len = sizeof pdf_font_Dingbats;
		return (unsigned char*) pdf_font_Dingbats;
	}
	*len = 0;
	return NULL;
}

unsigned char *
pdf_lookup_substitute_font(int mono, int serif, int bold, int italic, unsigned int *len)
{
#ifdef NODROIDFONT
	if (mono) {
		if (bold) {
			if (italic) return pdf_lookup_builtin_font("Courier-BoldOblique", len);
			else return pdf_lookup_builtin_font("Courier-Bold", len);
		} else {
			if (italic) return pdf_lookup_builtin_font("Courier-Oblique", len);
			else return pdf_lookup_builtin_font("Courier", len);
		}
	} else if (serif) {
		if (bold) {
			if (italic) return pdf_lookup_builtin_font("Times-BoldItalic", len);
			else return pdf_lookup_builtin_font("Times-Bold", len);
		} else {
			if (italic) return pdf_lookup_builtin_font("Times-Italic", len);
			else return pdf_lookup_builtin_font("Times-Roman", len);
		}
	} else {
		if (bold) {
			if (italic) return pdf_lookup_builtin_font("Helvetica-BoldOblique", len);
			else return pdf_lookup_builtin_font("Helvetica-Bold", len);
		} else {
			if (italic) return pdf_lookup_builtin_font("Helvetica-Oblique", len);
			else return pdf_lookup_builtin_font("Helvetica", len);
		}
	}
#else
	if (mono) {
		*len = sizeof pdf_font_DroidSansMono;
		return (unsigned char*) pdf_font_DroidSansMono;
	} else {
		*len = sizeof pdf_font_DroidSans;
		return (unsigned char*) pdf_font_DroidSans;
	}
#endif
}

unsigned char *
pdf_lookup_substitute_cjk_font(int ros, int serif, unsigned int *len)
{
#ifndef NOCJKFONT
	*len = sizeof pdf_font_DroidSansFallback;
	return (unsigned char*) pdf_font_DroidSansFallback;
#else
	*len = 0;
	return NULL;
#endif
}


#else /* APV_ASSET_FONTS is set */

#include <jni.h>

#include "hashmap.h"


static Hashmap *fonts = NULL;


typedef struct {
	int len;
	char *data;
} font_asset_t;

/* defined in apvandroid.c */
JavaVM *apv_get_cached_jvm();

static bool str_eq(void *key_a, void *key_b)
{
    return !strcmp((const char *)key_a, (const char *)key_b);
}

/* djb hash */
static int str_hash(void *str)
{
    uint32_t hash = 5381;
    char *p;

    for (p = str; p && *p; p++)
        hash = ((hash << 5) + hash) + *p;
    return (int)hash;
}

unsigned char *apv_get_font_data(char *name, unsigned int *len) {
	static jni_ids_cached = 0;
	static jmethodID getFontData_method_id;

	JavaVM *jvm = NULL;
	JNIEnv *jni_env = NULL;
	jclass pdf_class = NULL;
	jbyteArray jbytes = NULL;
	int jbytes_size = 0;
	jbyte *jbytes_internal = NULL;
	jstring jname = NULL;

	jvm = apv_get_cached_jvm();
	(*jvm)->GetEnv(jvm, (void**)&jni_env, JNI_VERSION_1_4);

	pdf_class = (*jni_env)->FindClass(jni_env, "cx/hell/android/lib/pdf/PDF");

	if (!jni_ids_cached) {
		getFontData_method_id = (*jni_env)->GetStaticMethodID(jni_env, pdf_class, "getFontData", "(Ljava/lang/String;)[B");
	}

	jname = (*jni_env)->NewStringUTF(jni_env, name);

	jbytes = (jbyteArray) (*jni_env)->CallStaticObjectMethod(jni_env, pdf_class, getFontData_method_id, jname);
	if (!jbytes) {
		*len = 0;
		return NULL;
	}
	jbytes_size = (*jni_env)->GetArrayLength(jni_env, jbytes);

	char *data = malloc(jbytes_size);
	jbytes_internal = (*jni_env)->GetByteArrayElements(jni_env, jbytes, NULL);
	memcpy(data, jbytes_internal, jbytes_size);
	(*jni_env)->ReleaseByteArrayElements(jni_env, jbytes, jbytes_internal, JNI_ABORT);

	*len = jbytes_size;
	return data;
}

unsigned char *apv_get_font_data_cached(char *name, unsigned int *len) {

	char *data = NULL;
	font_asset_t *font = NULL;

    if (fonts == NULL) {
    	fonts = hashmapCreate(32, str_hash, str_eq);
    }

    if (hashmapContainsKey(fonts, name)) {
    	font = (font_asset_t*)hashmapGet(fonts, name);
    	if (font) {
    		*len = font->len;
    		return font->data;
    	} else {
    		*len = 0;
    		return NULL;
    	}
    }

    data = apv_get_font_data(name, len);
    font = malloc(sizeof(font_asset_t));
    font->data = data;
    font->len = *len;
    hashmapPut(fonts, name, font);
    return data;
}


unsigned char *pdf_lookup_builtin_font(char *name, unsigned int *len) {
    return apv_get_font_data(name, len);
}

unsigned char *pdf_lookup_substitute_font(int mono, int serif, int bold, int italic, unsigned int *len) {
	if (mono) {
        /*
		*len = sizeof pdf_font_DroidSansMono;
		return (unsigned char*) pdf_font_DroidSansMono;
        */
        return apv_get_font_data("DroidSansMono", len);
	} else {
        /*
		*len = sizeof pdf_font_DroidSans;
		return (unsigned char*) pdf_font_DroidSans;
        */
        return apv_get_font_data("DroidSans", len);
	}
}

unsigned char *pdf_lookup_substitute_cjk_font(int ros, int serif, unsigned int *len) {
    *len = 0;
    return NULL;
}





#endif // APV_ASSET_FONTS
