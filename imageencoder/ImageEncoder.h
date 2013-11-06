#ifndef __LIBMIX_INTEL_IMAGE_ENCODER_H__
#define __LIBMIX_INTEL_IMAGE_ENCODER_H__

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <va/va.h>
#include <va/va_android.h>
#include <va/va_tpi.h>
#include <va/va_enc_jpeg.h>

#define INTEL_IMAGE_ENCODER_DEFAULT_QUALITY 90
#define INTEL_IMAGE_ENCODER_MAX_QUALITY 100
#define INTEL_IMAGE_ENCODER_MIN_QUALITY 1
#define INTEL_IMAGE_ENCODER_MAX_BUFFERS 64
#define INTEL_IMAGE_ENCODER_REQUIRED_STRIDE 64
#ifndef VA_FOURCC_YV16
#define VA_FOURCC_YV16 0x36315659
#endif
#define SURFACE_TYPE_USER_PTR 0x00000004
#define SURFACE_TYPE_GRALLOC 0x00100000

class IntelImageEncoder {
public:
	IntelImageEncoder(void);
	~IntelImageEncoder(void) {};
	int initializeEncoder(void);
	int createSourceSurface(int source_type, void *source_buffer,
				unsigned int width,unsigned int height,
				unsigned int stride, unsigned int fourcc,
				int *image_seqp);
	int createContext(int first_image_seq, unsigned int *max_coded_sizep);
	int createContext(unsigned int *max_coded_sizep)
	{
		return this->createContext(0, max_coded_sizep);
	}
	int setQuality(unsigned int new_quality);
	int encode(int image_seq, unsigned int new_quality);
	int encode(int image_seq)
	{
		return this->encode(image_seq, quality);
	}
	int encode(void)
	{
		return this->encode(0, quality);
	}
	int getCoded(void *user_coded_buf,
			unsigned int user_coded_buf_size,
			unsigned int *coded_data_sizep);
	int destroySourceSurface(int image_seq);
	int destroyContext(void);
	int deinitializeEncoder(void);

private:
	typedef enum {
		LIBVA_UNINITIALIZED = 0,
		LIBVA_INITIALIZED,
		LIBVA_CONTEXT_CREATED,
		LIBVA_ENCODING,
	}IntelImageEncoderStatus;

	/* Valid since LIBVA_UNINITIALIZED */
	IntelImageEncoderStatus encoder_status;
	unsigned int quality;

	/* Valid Since LIBVA_INITIALIZED */
	VADisplay va_dpy;
	VAConfigAttrib va_configattrib;

	/* Valid if a surface is created */
	unsigned int images_count;
	VASurfaceID va_surfaceid[INTEL_IMAGE_ENCODER_MAX_BUFFERS];
	unsigned int surface_width[INTEL_IMAGE_ENCODER_MAX_BUFFERS];
	unsigned int surface_height[INTEL_IMAGE_ENCODER_MAX_BUFFERS];
	unsigned int surface_fourcc[INTEL_IMAGE_ENCODER_MAX_BUFFERS];

	/* Valid since LIBVA_CONTEXT_CREATED */
	VAConfigID va_configid;
	VAContextID va_contextid;
	unsigned int context_width;
	unsigned int context_height;
	unsigned int context_fourcc;
	VABufferID va_codedbufferid;
	unsigned int coded_buf_size;

	/* Valid since LIBVA_ENCODING */
	int reserved_image_seq;
};

#endif /* __LIBMIX_INTEL_IMAGE_ENCODER_H__ */
