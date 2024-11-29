/*
 * idealweather.c
 *
 * version 0.1  2024-11-21
 *
 * copyright 2024  Mark J. Stock  markjstock@gmail.com
 *
 * Read many 16-bit grey png images and find the ideal climate
 *
 * Compile with
 *    gcc -Ofast -march=native -o idealweather idealweather.c -lpng
 */

#define TRUE 1
#define FALSE 0
#define MAXRES 100000

#define png_infopp_NULL (png_infopp)NULL
#define int_p_NULL (int*)NULL
#include <png.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>


/*
 * allocate memory for a two-dimensional array of float
 */
float** allocate_2d_array_f(int nx,int ny) {

   int i;
   float **array = (float **)malloc(nx * sizeof(float *));

   array[0] = (float *)malloc(nx * ny * sizeof(float));
   for (i=1; i<nx; i++)
      array[i] = array[0] + i * ny;

   return(array);
}

int free_2d_array_f(float** array){
   free(array[0]);
   free(array);
   return(0);
}

/*
 * allocate memory for a two-dimensional array of png_byte
 */
png_byte** allocate_2d_array_pb(int nx, int ny, int depth) {

   int i,bytesperpixel;
   png_byte **array;

   if (depth <= 8) bytesperpixel = 1;
   else bytesperpixel = 2;
   array = (png_byte **)malloc(ny * sizeof(png_byte *));
   array[0] = (png_byte *)malloc(bytesperpixel * nx * ny * sizeof(png_byte));

   for (i=1; i<ny; i++)
      array[i] = array[0] + i * bytesperpixel * nx;

   return(array);
}

png_byte** allocate_2d_rgb_array_pb(int nx, int ny, int depth) {

   int i,bytesperpixel;
   png_byte **array;

   if (depth <= 8) bytesperpixel = 3;
   else bytesperpixel = 6;
   array = (png_byte **)malloc(ny * sizeof(png_byte *));
   array[0] = (png_byte *)malloc(bytesperpixel * nx * ny * sizeof(png_byte));

   for (i=1; i<ny; i++)
      array[i] = array[0] + i * bytesperpixel * nx;

   return(array);
}

int free_2d_array_pb(png_byte** array){
   free(array[0]);
   free(array);
   return(0);
}


/*
 * print a frame using 1 or 3 channels to png - 2D
 */
int write_png (char *outfile, int nx, int ny,
   int three_channel, int high_depth,
   float **red, float redmin, float redrange,
   float **grn, float grnmin, float grnrange,
   float **blu, float blumin, float blurange) {

   int autorange = FALSE;
   int i,j,printval,bit_depth;
   float newminrange,newmaxrange;
   // gamma of 1.8 looks normal on most monitors...that display properly.
   //float gamma = 1.8;
   // must do 5/9 for stuff to look right on Macs....why? I dunno.
   float gamma = .55555;
   FILE *fp;
   png_uint_32 height,width;
   png_structp png_ptr;
   png_infop info_ptr;
   png_byte **img;
   int is_allocated = FALSE;
   png_byte **imgrgb;
   int rgb_is_allocated = FALSE;

   // set specific bit depth
   if (high_depth) bit_depth = 16;
   else bit_depth = 8;

   // allocate the space for the special array
   if (!is_allocated && !three_channel) {
      img = allocate_2d_array_pb(nx,ny,bit_depth);
      is_allocated = TRUE;
   }
   if (!rgb_is_allocated && three_channel) {
      imgrgb = allocate_2d_rgb_array_pb(nx,ny,bit_depth);
      rgb_is_allocated = TRUE;
   }

   // set the sizes in png-understandable format
   height=ny;
   width=nx;

   // auto-set the ranges
   if (autorange) {

      // first red
      newminrange = 9.9e+9;
      newmaxrange = -9.9e+9;
      for (i=0; i<nx; i++) {
         for (j=ny-1; j>=0; j--) {
            if (red[i][j]<newminrange) newminrange=red[i][j];
            if (red[i][j]>newmaxrange) newmaxrange=red[i][j];
         }
      }
      //printf("range %g %g\n",newminrange,newmaxrange);
      redmin = newminrange;
      redrange = newmaxrange-newminrange;

      // then the rest
      if (three_channel) {

         // then green
         newminrange = 9.9e+9;
         newmaxrange = -9.9e+9;
         for (i=0; i<nx; i++) {
            for (j=ny-1; j>=0; j--) {
               if (grn[i][j]<newminrange) newminrange=grn[i][j];
               if (grn[i][j]>newmaxrange) newmaxrange=grn[i][j];
            }
         }
         grnmin = newminrange;
         grnrange = newmaxrange-newminrange;

         // then blue
         newminrange = 9.9e+9;
         newmaxrange = -9.9e+9;
         for (i=0; i<nx; i++) {
            for (j=ny-1; j>=0; j--) {
               if (blu[i][j]<newminrange) newminrange=blu[i][j];
               if (blu[i][j]>newmaxrange) newmaxrange=blu[i][j];
            }
         }
         blumin = newminrange;
         blurange = newmaxrange-newminrange;
      }
   } else {
       // report the range
      newminrange = 9.9e+9;
      newmaxrange = -9.9e+9;
      for (i=0; i<nx; i++) {
         for (j=ny-1; j>=0; j--) {
            if (red[i][j]<newminrange) newminrange=red[i][j];
            if (red[i][j]>newmaxrange) newmaxrange=red[i][j];
         }
      }
      printf("  output range %g %g\n",newminrange,newmaxrange);
   }
 
   // write the file
   fp = fopen(outfile,"wb");
   if (fp==NULL) {
      fprintf(stderr,"Could not open output file %s\n",outfile);
      fflush(stderr);
      exit(0);
   }

   // now do the other two channels
   if (three_channel) {

     // no scaling, 16-bit per channel, RGB
     if (high_depth) {
       // am I looping these coordinates in the right memory order?
       for (j=ny-1; j>=0; j--) {
         for (i=0; i<nx; i++) {
           // red
           printval = (int)(0.5 + 65535*(red[i][j]-redmin)/redrange);
           if (printval<0) printval = 0;
           else if (printval>65535) printval = 65535;
           imgrgb[ny-1-j][6*i] = (png_byte)(printval/256);
           imgrgb[ny-1-j][6*i+1] = (png_byte)(printval%256);
           // green
           printval = (int)(0.5 + 65535*(grn[i][j]-grnmin)/grnrange);
           if (printval<0) printval = 0;
           else if (printval>65535) printval = 65535;
           imgrgb[ny-1-j][6*i+2] = (png_byte)(printval/256);
           imgrgb[ny-1-j][6*i+3] = (png_byte)(printval%256);
           // blue
           printval = (int)(0.5 + 65535*(blu[i][j]-blumin)/blurange);
           if (printval<0) printval = 0;
           else if (printval>65535) printval = 65535;
           imgrgb[ny-1-j][6*i+4] = (png_byte)(printval/256);
           imgrgb[ny-1-j][6*i+5] = (png_byte)(printval%256);
         }
       }

     // no scaling, 8-bit per channel, RGB
     } else {
       // am I looping these coordinates in the right memory order?
       for (j=ny-1; j>=0; j--) {
         for (i=0; i<nx; i++) {
           // red
           printval = (int)(0.5 + 256*(red[i][j]-redmin)/redrange);
           if (printval<0) printval = 0;
           else if (printval>255) printval = 255;
           imgrgb[ny-1-j][3*i] = (png_byte)printval;
           // green
           printval = (int)(0.5 + 256*(grn[i][j]-grnmin)/grnrange);
           if (printval<0) printval = 0;
           else if (printval>255) printval = 255;
           imgrgb[ny-1-j][3*i+1] = (png_byte)printval;
           // blue
           printval = (int)(0.5 + 256*(blu[i][j]-blumin)/blurange);
           if (printval<0) printval = 0;
           else if (printval>255) printval = 255;
           imgrgb[ny-1-j][3*i+2] = (png_byte)printval;
         }
       }
     }

   // monochrome image, read data from red array
   } else {

     // no scaling, 16-bit per channel
     if (high_depth) {
       // am I looping these coordinates in the right memory order?
       for (j=ny-1; j>=0; j--) {
         for (i=0; i<nx; i++) {
           printval = (int)(0.5 + 65534*(red[i][j]-redmin)/redrange);
           if (printval<0) printval = 0;
           else if (printval>65535) printval = 65535;
           img[ny-1-j][2*i] = (png_byte)(printval/256);
           img[ny-1-j][2*i+1] = (png_byte)(printval%256);
         }
       }

     // no scaling, 8-bit per channel
     } else {
       // am I looping these coordinates in the right memory order?
       for (j=ny-1; j>=0; j--) {
         for (i=0; i<nx; i++) {
           printval = (int)(0.5 + 254*(red[i][j]-redmin)/redrange);
           if (printval<0) printval = 0;
           else if (printval>255) printval = 255;
           img[ny-1-j][i] = (png_byte)printval;
         }
       }
     }

   }

   /* Create and initialize the png_struct with the desired error handler
    * functions.  If you want to use the default stderr and longjump method,
    * you can supply NULL for the last three parameters.  We also check that
    * the library version is compatible with the one used at compile time,
    * in case we are using dynamically linked libraries.  REQUIRED.
    */
   png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
      NULL, NULL, NULL);

   if (png_ptr == NULL) {
      fclose(fp);
      fprintf(stderr,"Could not create png struct\n");
      fflush(stderr);
      exit(0);
      return (-1);
   }

   /* Allocate/initialize the image information data.  REQUIRED */
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL) {
      fclose(fp);
      png_destroy_write_struct(&png_ptr,(png_infopp)NULL);
      return (-1);
   }

   /* Set error handling.  REQUIRED if you aren't supplying your own
    * error handling functions in the png_create_write_struct() call.
    */
   if (setjmp(png_jmpbuf(png_ptr))) {
      /* If we get here, we had a problem reading the file */
      fclose(fp);
      png_destroy_write_struct(&png_ptr, &info_ptr);
      return (-1);
   }

   /* set up the output control if you are using standard C streams */
   png_init_io(png_ptr, fp);

   /* Set the image information here.  Width and height are up to 2^31,
    * bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
    * the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
    * PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
    * or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
    * PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
    * currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
    */
   // png_set_IHDR(png_ptr, info_ptr, height, width, bit_depth,
   if (three_channel) {
      png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth,
         PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
         PNG_FILTER_TYPE_BASE);
   } else {
      png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth,
         PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
         PNG_FILTER_TYPE_BASE);
   }

   /* Optional gamma chunk is strongly suggested if you have any guess
    * as to the correct gamma of the image. */
   //png_set_gAMA(png_ptr, info_ptr, 2.2);
   png_set_gAMA(png_ptr, info_ptr, gamma);

   /* Write the file header information.  REQUIRED */
   png_write_info(png_ptr, info_ptr);

   /* One of the following output methods is REQUIRED */
   // png_write_image(png_ptr, row_pointers);
   if (three_channel) {
      png_write_image(png_ptr, imgrgb);
   } else {
      png_write_image(png_ptr, img);
   }

   /* It is REQUIRED to call this to finish writing the rest of the file */
   png_write_end(png_ptr, info_ptr);

   /* clean up after the write, and free any memory allocated */
   png_destroy_write_struct(&png_ptr, &info_ptr);

   // close file
   fclose(fp);

   // free the data array
   if (is_allocated) free_2d_array_pb(img);
   if (rgb_is_allocated) free_2d_array_pb(imgrgb);

   return(0);
}


/*
 * read a PNG header and return width and height
 */
int read_png_res (char *infile, int *hgt, int *wdt) {

   FILE *fp;
   unsigned char header[8];
   png_uint_32 height,width;
   int bit_depth,color_type,interlace_type;
   png_structp png_ptr;
   png_infop info_ptr;


   // check the file
   fp = fopen(infile,"rb");
   if (fp==NULL) {
      fprintf(stderr,"Could not open input file %s\n",infile);
      fflush(stderr);
      exit(0);
   }

   // check to see that it's a PNG
   fread (&header, 1, 8, fp);
   if (png_sig_cmp(header, 0, 8)) {
      fprintf(stderr,"File %s is not a PNG\n",infile);
      fflush(stderr);
      exit(0);
   }

   /* Create and initialize the png_struct with the desired error handler
    * functions.  If you want to use the default stderr and longjump method,
    * you can supply NULL for the last three parameters.  We also supply the
    * the compiler header file version, so that we know if the application
    * was compiled with a compatible version of the library.  REQUIRED
    */
   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
      NULL, NULL, NULL);

   /* Allocate/initialize the memory for image information.  REQUIRED. */
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL) {
      fclose(fp);
      png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
      exit(0);
   }

   /* Set error handling if you are using the setjmp/longjmp method (this is
    * the normal method of doing things with libpng).  REQUIRED unless you
    * set up your own error handlers in the png_create_read_struct() earlier.  */
   if (setjmp(png_jmpbuf(png_ptr))) {
      /* Free all of the memory associated with the png_ptr and info_ptr */
      png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
      fclose(fp);
      /* If we get here, we had a problem reading the file */
      exit(0);
   }

   /* One of the following I/O initialization methods is REQUIRED */
   /* Set up the input control if you are using standard C streams */
   png_init_io(png_ptr, fp);

   /* If we have already read some of the signature */
   png_set_sig_bytes(png_ptr, 8);

   /* The call to png_read_info() gives us all of the information from the
    * PNG file before the first IDAT (image data chunk).  REQUIRED */
   png_read_info(png_ptr, info_ptr);

   png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
       &interlace_type, int_p_NULL, int_p_NULL);

   // set the sizes so that we can understand them
   (*hgt) = height;
   (*wdt) = width;

   /* clean up after the read, and free any memory allocated - REQUIRED */
   png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

   /* close the file */
   fclose(fp);

   return(0);
}


/*
 * read a PNG, write it to 1 or 3 channels
 */
int read_png (char *infile, int nx, int ny,
   int expect_three_channel,
   int overlay, float overlay_frac, int darkenonly,
   float **red, float redmin, float redrange,
   float **grn, float grnmin, float grnrange,
   float **blu, float blumin, float blurange) {

   int autorange = FALSE;
   int high_depth;
   int three_channel;
   int i,j,printval; //,bit_depth,color_type,interlace_type;
   float newminrange,newmaxrange;
   float overlay_divisor;
   FILE *fp;
   unsigned char header[8];
   png_uint_32 height,width;
   int bit_depth,color_type,interlace_type;
   png_structp png_ptr;
   png_infop info_ptr;
   png_byte **img;


   // set up overlay divisor
   if (overlay == TRUE) {
      // set divisor to blend original and incoming
      overlay_divisor = 1.0 + overlay_frac;
   } else if (overlay == 2) {
      // set divisor to add original and incoming
      overlay_divisor = 1.0;
   }

   // check the file
   fp = fopen(infile,"rb");
   if (fp==NULL) {
      fprintf(stderr,"Could not open input file %s\n",infile);
      fflush(stderr);
      exit(0);
   }

   // check to see that it's a PNG
   fread (&header, 1, 8, fp);
   if (png_sig_cmp(header, 0, 8)) {
      fprintf(stderr,"File %s is not a PNG\n",infile);
      fflush(stderr);
      exit(0);
   }

   /* Create and initialize the png_struct with the desired error handler
    * functions.  If you want to use the default stderr and longjump method,
    * you can supply NULL for the last three parameters.  We also supply the
    * the compiler header file version, so that we know if the application
    * was compiled with a compatible version of the library.  REQUIRED
    */
   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
      NULL, NULL, NULL);

   /* Allocate/initialize the memory for image information.  REQUIRED. */
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL) {
      fclose(fp);
      png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
      exit(0);
   }

   /* Set error handling if you are using the setjmp/longjmp method (this is
    * the normal method of doing things with libpng).  REQUIRED unless you
    * set up your own error handlers in the png_create_read_struct() earlier.  */
   if (setjmp(png_jmpbuf(png_ptr))) {
      /* Free all of the memory associated with the png_ptr and info_ptr */
      png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
      fclose(fp);
      /* If we get here, we had a problem reading the file */
      exit(0);
   }

   /* One of the following I/O initialization methods is REQUIRED */
   /* Set up the input control if you are using standard C streams */
   png_init_io(png_ptr, fp);

   /* If we have already read some of the signature */
   png_set_sig_bytes(png_ptr, 8);

   /* The call to png_read_info() gives us all of the information from the
    * PNG file before the first IDAT (image data chunk).  REQUIRED */
   png_read_info(png_ptr, info_ptr);

   png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
       &interlace_type, int_p_NULL, int_p_NULL);

   /* Set up the data transformations you want.  Note that these are all
    * optional.  Only call them if you want/need them.  Many of the
    * transformations only work on specific types of images, and many
    * are mutually exclusive.  */

   /* tell libpng to strip 16 bit/color files down to 8 bits/color */
   //png_set_strip_16(png_ptr);

   /* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
    * byte into separate bytes (useful for paletted and grayscale images).  */
   png_set_packing(png_ptr);

   /* Expand paletted colors into true RGB triplets */
   //if (color_type == PNG_COLOR_TYPE_PALETTE)
   //   png_set_palette_rgb(png_ptr);

   /* Expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel */
   if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
      png_set_expand_gray_1_2_4_to_8(png_ptr);

   /* Optional call to gamma correct and add the background to the palette
    * and update info structure.  REQUIRED if you are expecting libpng to
    * update the palette for you (ie you selected such a transform above).
    */
   //png_read_update_info(png_ptr, info_ptr);

   // check image type for applicability
   if (bit_depth != 8 && bit_depth != 16) {
     fprintf(stderr,"INCOMPLETE: read_png expect 8-bit or 16-bit images\n");
     fprintf(stderr,"   bit_depth: %d\n",bit_depth);
     fprintf(stderr,"   file: %s\n",infile);
     exit(0);
   }
   if (color_type != PNG_COLOR_TYPE_GRAY && color_type != PNG_COLOR_TYPE_RGB) {
     fprintf(stderr,"INCOMPLETE: read_png expect grayscale (%d) or RGB (%d) images\n",PNG_COLOR_TYPE_GRAY,PNG_COLOR_TYPE_RGB);
     fprintf(stderr,"   color_type: %d\n",color_type);
     fprintf(stderr,"   file: %s\n",infile);
     exit(0);
   }

   // set channels
   if (color_type == PNG_COLOR_TYPE_GRAY) three_channel = FALSE;
   else three_channel = TRUE;

   if (expect_three_channel && !three_channel) {
     fprintf(stderr,"ERROR: expecting 3-channel PNG, but input is 1-channel\n");
     fprintf(stderr,"  file (%s)",infile);
     fprintf(stderr,"  Convert file to color and try again.\n");
     exit(0);
   }

   if (!expect_three_channel && three_channel) {
     fprintf(stderr,"ERROR: not expecting 3-channel PNG, but input is 3-channel\n");
     fprintf(stderr,"  file (%s)",infile);
     fprintf(stderr,"  Convert file to grayscale and try again.\n");
     exit(0);
   }

   // set specific bit depth
   if (bit_depth == 16) high_depth = TRUE;
   else high_depth = FALSE;

   // make sure pixel sizes match exactly!
   if (ny != height || nx != width) {
     fprintf(stderr,"INCOMPLETE: read_png expects image resolution to match\n");
     fprintf(stderr,"  the simulation resolution.");
     fprintf(stderr,"  simulation %d x %d",nx,ny);
     fprintf(stderr,"  image %d x %d",width,height);
     fprintf(stderr,"  file (%s)",infile);
     exit(0);
   }

   // set the sizes so that we can understand them
   ny = height;
   nx = width;

   // allocate the space for the image array
   if (three_channel) {
      img = allocate_2d_rgb_array_pb(nx,ny,bit_depth);
   } else {
      img = allocate_2d_array_pb(nx,ny,bit_depth);
   }

   /* Now it's time to read the image.  One of these methods is REQUIRED */
   png_read_image(png_ptr, img);

   /* read rest of file, and get additional chunks in info_ptr - REQUIRED */
   png_read_end(png_ptr, info_ptr);

   /* At this point you have read the entire image */

   /* clean up after the read, and free any memory allocated - REQUIRED */
   png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

   /* close the file */
   fclose(fp);


   // now convert the data to stuff we can use
   if (three_channel) {

     // no scaling, 16-bit per channel, RGB
     if (high_depth) {
       if (overlay && !darkenonly) {
         for (j=ny-1; j>=0; j--) {
           for (i=0; i<nx; i++) {
             red[i][j] = (red[i][j] + overlay_frac*(redmin+redrange*(img[ny-1-j][6*i]*256+img[ny-1-j][6*i+1])/65535.)) / overlay_divisor;
             grn[i][j] = (grn[i][j] + overlay_frac*(grnmin+grnrange*(img[ny-1-j][6*i+2]*256+img[ny-1-j][6*i+3])/65535.)) / overlay_divisor;
             blu[i][j] = (blu[i][j] + overlay_frac*(blumin+blurange*(img[ny-1-j][6*i+4]*256+img[ny-1-j][6*i+5])/65535.)) / overlay_divisor;
           }
         }
       } else if (overlay && darkenonly) {
         for (j=ny-1; j>=0; j--) {
           for (i=0; i<nx; i++) {
             red[i][j] -= overlay_frac*(redmin+redrange*(1.-img[ny-1-j][3*i]/255.));
             red[i][j] -= overlay_frac*(redmin+redrange*(1.-(img[ny-1-j][6*i]*256+img[ny-1-j][6*i+1])/65535.));
             grn[i][j] -= overlay_frac*(grnmin+grnrange*(1.-(img[ny-1-j][6*i+2]*256+img[ny-1-j][6*i+3])/65535.));
             blu[i][j] -= overlay_frac*(blumin+blurange*(1.-(img[ny-1-j][6*i+4]*256+img[ny-1-j][6*i+5])/65535.));
           }
         }
       } else {
         for (j=ny-1; j>=0; j--) {
           for (i=0; i<nx; i++) {
             red[i][j] = redmin+redrange*(img[ny-1-j][6*i]*256+img[ny-1-j][6*i+1])/65535.;
             grn[i][j] = grnmin+grnrange*(img[ny-1-j][6*i+2]*256+img[ny-1-j][6*i+3])/65535.;
             blu[i][j] = blumin+blurange*(img[ny-1-j][6*i+4]*256+img[ny-1-j][6*i+5])/65535.;
           }
         }
       }

     // no scaling, 8-bit per channel, RGB
     } else {
       if (overlay && !darkenonly) {
         for (j=ny-1; j>=0; j--) {
           for (i=0; i<nx; i++) {
             red[i][j] = (red[i][j] + overlay_frac*(redmin+redrange*img[ny-1-j][3*i]/255.)) / overlay_divisor;
             grn[i][j] = (grn[i][j] + overlay_frac*(grnmin+grnrange*img[ny-1-j][3*i+1]/255.)) / overlay_divisor;
             blu[i][j] = (blu[i][j] + overlay_frac*(blumin+blurange*img[ny-1-j][3*i+2]/255.)) / overlay_divisor;
           }
         }
       } else if (overlay && darkenonly) {
         for (j=ny-1; j>=0; j--) {
           for (i=0; i<nx; i++) {
             red[i][j] -= overlay_frac*(redmin+redrange*(1.-img[ny-1-j][3*i]/255.));
             grn[i][j] -= overlay_frac*(grnmin+grnrange*(1.-img[ny-1-j][3*i+1]/255.));
             blu[i][j] -= overlay_frac*(blumin+blurange*(1.-img[ny-1-j][3*i+2]/255.));
           }
         }
       } else {
         for (j=ny-1; j>=0; j--) {
           for (i=0; i<nx; i++) {
             red[i][j] = redmin+redrange*img[ny-1-j][3*i]/255.;
             grn[i][j] = grnmin+grnrange*img[ny-1-j][3*i+1]/255.;
             blu[i][j] = blumin+blurange*img[ny-1-j][3*i+2]/255.;
           }
         }
       }
     }

   // monochrome image, read data from red array
   } else {

     // no scaling, 16-bit per channel
     if (high_depth) {
       if (overlay) {
         for (j=ny-1; j>=0; j--) {
           for (i=0; i<nx; i++) {
             red[i][j] = (red[i][j] + overlay_frac*(redmin+redrange*(img[ny-1-j][2*i]*256+img[ny-1-j][2*i+1])/65534.)) / overlay_divisor;
           }
         }
       } else {
         for (j=ny-1; j>=0; j--) {
           for (i=0; i<nx; i++) {
             red[i][j] = redmin+redrange*(img[ny-1-j][2*i]*256+img[ny-1-j][2*i+1])/65534.;
           }
         }
       }

     // no scaling, 8-bit per channel
     } else {
       if (overlay) {
         for (j=ny-1; j>=0; j--) {
           for (i=0; i<nx; i++) {
             red[i][j] = (red[i][j] + overlay_frac*(redmin+redrange*img[ny-1-j][i]/254.)) / overlay_divisor;
           }
         }
       } else {
         for (j=ny-1; j>=0; j--) {
           for (i=0; i<nx; i++) {
             red[i][j] = redmin+redrange*img[ny-1-j][i]/254.;
           }
         }
       }
     }

   }

   // free the data array
   free_2d_array_pb(img);

   return(0);
}


/*
 * This function writes basic usage information to stderr,
 * and then quits. Too bad.
 */
int Usage(char progname[255],int status) {

   static char **cpp, *help_message[] = {
   "where [-options] are zero or more of the following:                        ",
   "                                                                           ",
   "   [-tf jan-low jan-high jul-low jul-high]    temperatures in deg F        ",
   "                                                                           ",
   "   [-tc jan-low jan-high jul-low jul-high]    temperatures in deg C        ",
   "                                                                           ",
   "   [-mr num]   average monthly precipitation, in mm/month                  ",
   "                                                                           ",
   "   [-ac num]   average cloudiness, in fraction (0..1, 1=always cloudy)     ",
   "                                                                           ",
   "   [-hdi num]  Human Development Index, local (0..1)                       ",
   "                                                                           ",
   "   [-mtn num]  proximity to and magnitude of mountains (0==1)              ",
   "                                                                           ",
   "   [-wmph num]   average wind speed in miles-per-hour                      ",
   "                                                                           ",
   "   [-wmps num]   average wind speed in meters-per-second                   ",
   "                                                                           ",
   "   [-ct lat lon]   prefer locations close to given lat-lon location (N, E) ",
   "                                                                           ",
   "   [-ff lat lon]   prefer locations far from given lat-lon location (N, E) ",
   "                                                                           ",
   "   [-boston]   set all preferences to that of Boston, Massachusetts, USA   ",
   "                                                                           ",
   "   [-new]      begin defining preferences for a second person (up to 8)    ",
   "                                                                           ",
   "   [-o file]   output file name                                            ",
   "                                                                           ",
   "   [-help]     returns this help information                               ",
   " ",
   "Options may be abbreviated to an unambiguous length.",
   "Output is to a series of PNM files.",
   NULL
   };

   fprintf(stderr, "usage:\n  %s [options]\n\n", progname);
   for (cpp = help_message; *cpp; cpp++) fprintf(stderr, "%s\n", *cpp);
   fflush(stderr);
   exit(status);
   return(0);
}


float ftoc (const float tempf) {
  return (tempf-32.f)/1.8f;
}

// return great circle route distance in radians (0..pi) given lat-lon
float gcr_dist (const float lat1, const float lon1, const float lat2, const float lon2) {

  // first convert to xyz vectors
  float v1[3];
  float v2[3];

  v1[0] = cosf(lon1)*cosf(lat1);
  v1[1] = sinf(lon1)*cosf(lat1);
  v1[2] = sinf(lat1);
  v2[0] = cosf(lon2)*cosf(lat2);
  v2[1] = sinf(lon2)*cosf(lat2);
  v2[2] = sinf(lat2);

  const float dp = v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
  const float theta = asinf(1.f) - asinf(dp);

  return theta;
}

// return great circle route distance in radians (0..pi) given pixel coords
float gcr_dist_px (const int x1, const int y1, const int x2, const int y2, const int px, const int py) {

  const float lat1 = 90.f - 180.f * (0.5f+y1) / (float)py;
  const float lon1 = -180.f + 360.f * (0.5f+x1) / (float)px;
  const float lat2 = 90.f - 180.f * (0.5f+y2) / (float)py;
  const float lon2 = -180.f + 360.f * (0.5f+x2) / (float)px;
  //printf("\npixel %d,%d is %gN %gE\n", x1, y1, lat1, lon1);
  //printf("pixel %d,%d is %gN %gE\n", x2, y2, lat2, lon2);
  //printf("distance is %g\n", gcr_dist (lat1, lon1, lat2, lon2));
  const float degtorad = asinf(1.f) / 90.f;

  return gcr_dist (degtorad*lat1, degtorad*lon1, degtorad*lat2, degtorad*lon2);
}


int main (int argc, char **argv) {

  // array to hold up to 8 sets of preferences
  float ideal[8][11];
  int p = 1;	// start with 1 set of preferences
  // pre-set all
  for (int i=0; i<8; ++i) {
    ideal[i][0] = 10.f;
    ideal[i][1] = 20.f;
    // these -1 mean "don't use"
    ideal[i][2] = -1.f;
    ideal[i][3] = -1.f;
    ideal[i][4] = -1.f;
    ideal[i][5] = -1.f;
    ideal[i][6] = -1.f;
    // for lat-lon, need lower
    ideal[i][7] = -999.f;
    ideal[i][8] = -999.f;
    ideal[i][9] = -999.f;
    ideal[i][10] = -999.f;
  }

  // array to hold values for my hometown
  float boston[7];
  boston[0] = 1.1f;		// Jan mean temp (-30..40 C)
  boston[1] = 24.5f;	// July mean temp (-30..40 C)
  boston[2] = 101.f;	// Annual average rain (0..1000 mm/mo)
  boston[3] = 0.516f;	// Annual average cloud cover (0..1)
  boston[4] = 3.75f;	// Average wind speed at 10m (0..25 m/s)
  boston[5] = 0.985f;	// Human Development Index (0..1), negative means don't use
  boston[6] = -1.0f;	// Proximity to mountains (0..1), negative means don't use
  boston[7] = 42.35f;	// latitude (N degrees) - close to
  boston[8] = -71.05f;	// longitude (E degrees) - close to
  boston[9] = -999.f;	// latitude (N degrees) - far from
  boston[10] = -999.f;	// longitude (E degrees) - far from

  // define default parameters
  char outpng[255];
  sprintf(outpng,"out.png");

  // process command-line parameters
  char progname[255];
  (void) strcpy(progname,argv[0]);
  if (argc < 1) (void) Usage(progname,0);
  for (int i=1; i<argc; i++) {
    if (strncmp(argv[i], "-boston", 3) == 0) {
      // replace ideals for current person to Boston
      for (int i=0; i<6; ++i) ideal[p-1][i] = boston[i];
    } else if (strncmp(argv[i], "-new", 3) == 0) {
      if (p==8) {
        printf("No more than 8 sets of preferences allowed.\n");
      } else {
        // begin setting preferences for a new person
        ++p;
        printf("Setting ideals for person %d now\n", p);
      }
    } else if (strncmp(argv[i], "-tc", 3) == 0) {
      const float janlow = atof(argv[++i]);
      const float janhigh = atof(argv[++i]);
      const float julylow = atof(argv[++i]);
      const float julyhigh = atof(argv[++i]);
      ideal[p-1][0] = 0.5*(janlow+janhigh);
      ideal[p-1][1] = 0.5*(julylow+julyhigh);
      printf("  set ideal Jan, July temps to %g %g C\n", ideal[p-1][0], ideal[p-1][1]);
    } else if (strncmp(argv[i], "-tf", 3) == 0) {
      const float janlow = atof(argv[++i]);
      const float janhigh = atof(argv[++i]);
      const float julylow = atof(argv[++i]);
      const float julyhigh = atof(argv[++i]);
      ideal[p-1][0] = ftoc(0.5*(janlow+janhigh));
      ideal[p-1][1] = ftoc(0.5*(julylow+julyhigh));
      printf("  set ideal Jan, July temps to %g %g C\n", ideal[p-1][0], ideal[p-1][1]);
    } else if (strncmp(argv[i], "-mr", 3) == 0) {
      ideal[p-1][2] = atof(argv[++i]);
      printf("  set ideal monthly rain to %g mm/mo\n", ideal[p-1][2]);
    } else if (strncmp(argv[i], "-ac", 2) == 0) {
      ideal[p-1][3] = atof(argv[++i]);
      printf("  set ideal annual cloud cover to %g (1=100%)\n", ideal[p-1][3]);
    } else if (strncmp(argv[i], "-wmps", 5) == 0) {
      ideal[p-1][4] = atof(argv[++i]);
      printf("  set ideal wind speed to %g (m/s)\n", ideal[p-1][4]);
    } else if (strncmp(argv[i], "-wmph", 5) == 0) {
      ideal[p-1][4] = 0.44704f*atof(argv[++i]);
      printf("  set ideal wind speed to %g (m/s)\n", ideal[p-1][4]);
    } else if (strncmp(argv[i], "-hdi", 3) == 0) {
      ideal[p-1][5] = atof(argv[++i]);
      printf("  set ideal Human Development Index to %g (1=most)\n", ideal[p-1][5]);
    } else if (strncmp(argv[i], "-mtn", 3) == 0) {
      ideal[p-1][6] = atof(argv[++i]);
      printf("  set ideal mountain proximity to %g (1=closest)\n", ideal[p-1][6]);
    } else if (strncmp(argv[i], "-ct", 3) == 0) {
      ideal[p-1][7] = atof(argv[++i]);
      ideal[p-1][8] = atof(argv[++i]);
      printf("  prefer close to %g N %g E\n", ideal[p-1][7], ideal[p-1][8]);
    } else if (strncmp(argv[i], "-ff", 3) == 0) {
      ideal[p-1][9] = atof(argv[++i]);
      ideal[p-1][10] = atof(argv[++i]);
      printf("  prefer far from %g N %g E\n", ideal[p-1][9], ideal[p-1][10]);
    //} else if (strncmp(argv[i], "-nw", 3) == 0) {
      //ideal[p-1][6] = atof(argv[++i]);
      //printf("  set ideal water proximity to %g (1=closest)\n", ideal[p-1][6]);
    //} else if (strncmp(argv[i], "-no", 3) == 0) {
      //ideal[p-1][6] = atof(argv[++i]);
      //printf("  set ideal ocean proximity to %g (1=closest)\n", ideal[p-1][6]);
    } else if (strncmp(argv[i], "-o", 2) == 0) {
      strcpy(outpng,argv[++i]);
    } else if (strncmp(argv[i], "-h", 2) == 0) {
      (void) Usage(progname,0);
    } else {
      (void) Usage(progname,0);
    }
  }

  // interrogate the header for resolution
  int xres = -1000;
  int yres = -1000;
  (void)read_png_res("airtemp_m1.png", &yres, &xres);

  // allocate and read temperature, full range is -30 to 40 C
  float** tempw = allocate_2d_array_f(xres,yres);
  (void)read_png("airtemp_m1.png",xres,yres,FALSE,FALSE,1.0,FALSE,tempw,-30.0,70.0,NULL,0.0,1.0,NULL,0.0,1.0);
  float** temps = allocate_2d_array_f(xres,yres);
  (void)read_png("airtemp_m7.png",xres,yres,FALSE,FALSE,1.0,FALSE,temps,-30.0,70.0,NULL,0.0,1.0,NULL,0.0,1.0);

  // allocate and read precipitation, full range is 0 to 1000mm per month
  float** rain = allocate_2d_array_f(xres,yres);
  (void)read_png("precip_avg.png",xres,yres,FALSE,FALSE,1.0,FALSE,rain,0.0,1000.0,NULL,0.0,1.0,NULL,0.0,1.0);

  // and clouds (0=sunny, 1=cloudy)
  float** clouds = allocate_2d_array_f(xres,yres);
  (void)read_png("clouds.png",xres,yres,FALSE,FALSE,1.0,FALSE,clouds,0.0,1.0,NULL,0.0,1.0,NULL,0.0,1.0);

  // wind (0 to 25 m/s average at 10m above ground)
  float** wind = allocate_2d_array_f(xres,yres);
  (void)read_png("windspeed.png",xres,yres,FALSE,FALSE,1.0,FALSE,wind,0.0,25.0,NULL,0.0,1.0,NULL,0.0,1.0);

  // human development index (0..1)
  float** hdi = allocate_2d_array_f(xres,yres);
  (void)read_png("hdi.png",xres,yres,FALSE,FALSE,1.0,FALSE,hdi,0.0,1.0,NULL,0.0,1.0,NULL,0.0,1.0);

  // proximity to mountains (0..1)
  float** mtn = allocate_2d_array_f(xres,yres);
  (void)read_png("dem_variance_area.png",xres,yres,FALSE,FALSE,1.0,FALSE,mtn,0.0,1.0,NULL,0.0,1.0,NULL,0.0,1.0);


  // allocate space for the output and set to 0
  float** outval = allocate_2d_array_f(xres,yres);
  for (int row=0; row<yres; ++row) {
    for (int col=0; col<xres; ++col) {
      outval[col][row] = 0.0f;
    }
  }

  // penalty weight for being off
  const float temp_penalty = 0.1f;
  const float rain_penalty = 1.5f;
  const float cloud_penalty = 5.0f;
  const float wind_penalty = 1.0f;
  const float hdi_penalty = 5.0f;
  const float mtn_penalty = 5.0f;
  const float dist_penalty = 2.5f;

  // running sums
  float total_temp = 0.f;
  float total_rain = 0.f;
  float total_cloud = 0.f;
  float total_wind = 0.f;
  float total_hdi = 0.f;
  float total_mtn = 0.f;
  float total_dist = 0.f;

  // accumulate penalties

  for (int ip=0; ip<p; ++ip) {

  // always consider temperature
  float ideal_tempw = ideal[ip][0];
  float ideal_temps = ideal[ip][1];
  for (int row=0; row<yres; ++row) {
    for (int col=0; col<xres; ++col) {
      if (tempw[col][row] > -29.9f) {
        float tempcost = temp_penalty * fabs(temps[col][row]-ideal_temps);
        outval[col][row] += tempcost;
        total_temp += tempcost;
        tempcost = temp_penalty * fabs(tempw[col][row]-ideal_tempw);
        outval[col][row] += tempcost;
        total_temp += tempcost;
      }
    }
  }

  // all others are optional
  float ideal_rain = ideal[ip][2];
  if (ideal_rain >= 0.f) {
    for (int row=0; row<yres; ++row) {
      for (int col=0; col<xres; ++col) {
        if (tempw[col][row] > -29.9f) {
          const float raincost = rain_penalty * fabs(logf((0.1f+rain[col][row])/(0.1f+ideal_rain)));
          outval[col][row] += raincost;
          total_rain += raincost;
        }
      }
    }
  }

  float ideal_cloud = ideal[ip][3];
  if (ideal_cloud >= 0.f) {
    for (int row=0; row<yres; ++row) {
      for (int col=0; col<xres; ++col) {
        if (tempw[col][row] > -29.9f) {
          const float cloudcost = cloud_penalty * fabs(clouds[col][row]-ideal_cloud);
          outval[col][row] += cloudcost;
          total_cloud += cloudcost;
        }
      }
    }
  }

  float ideal_wind = ideal[ip][4];
  if (ideal_wind >= 0.f) {
    for (int row=0; row<yres; ++row) {
      for (int col=0; col<xres; ++col) {
        if (tempw[col][row] > -29.9f) {
          const float windcost = wind_penalty * fabs(wind[col][row]-ideal_wind);
          outval[col][row] += windcost;
          total_wind += windcost;
        }
      }
    }
  }

  float ideal_hdi = ideal[ip][5];
  if (ideal_hdi >= 0.f) {
    for (int row=0; row<yres; ++row) {
      for (int col=0; col<xres; ++col) {
        if (tempw[col][row] > -29.9f) {
          const float hdicost = hdi_penalty * fabs(hdi[col][row]-ideal_hdi);
          outval[col][row] += hdicost;
          total_hdi += hdicost;
        }
      }
    }
  }

  float ideal_mtn = ideal[ip][6];
  if (ideal_mtn >= 0.f) {
    for (int row=0; row<yres; ++row) {
      for (int col=0; col<xres; ++col) {
        if (tempw[col][row] > -29.9f) {
          const float mtncost = mtn_penalty * fabs(mtn[col][row]-ideal_mtn);
          outval[col][row] += mtncost;
          total_mtn += mtncost;
        }
      }
    }
  }

  // want close to, so penalize far from
  int ideal_px = 0.5f + xres * (180.f + ideal[p-1][8]) / 360.f;
  int ideal_py = 0.5f + yres * ( 90.f + ideal[p-1][7]) / 180.f;
  if (ideal[p-1][7] > -500.f) {
    for (int row=0; row<yres; ++row) {
      for (int col=0; col<xres; ++col) {
        if (tempw[col][row] > -29.9f) {
          const float distcost = dist_penalty * gcr_dist_px(ideal_px, ideal_py, col, row, xres, yres);
          outval[col][row] += distcost;
          total_dist += distcost;
        }
      }
    }
  }

  // want far from given point, so penalize close to
  ideal_px = 0.5f + xres * (180.f + ideal[p-1][10]) / 360.f;
  ideal_py = 0.5f + yres * ( 90.f + ideal[p-1][9]) / 180.f;
  if (ideal[p-1][9] > -500.f) {
    for (int row=0; row<yres; ++row) {
      for (int col=0; col<xres; ++col) {
        if (tempw[col][row] > -29.9f) {
          const float distcost = dist_penalty * (3.1416f-gcr_dist_px(ideal_px, ideal_py, col, row, xres, yres));
          outval[col][row] += distcost;
          total_dist += distcost;
        }
      }
    }
  }


  }

  printf("total costs: temp %g, rain %g, cloud %g, wind %g, hdi %g, mtn %g\n", total_temp, total_rain, total_cloud, total_wind, total_hdi, total_mtn);

  // find the min and max values
  float loval = 9.9e+9;
  float hival = -9.9e+9;
  int npix = 0;
  for (int row=0; row<yres; ++row) {
    for (int col=0; col<xres; ++col) {
      if (tempw[col][row] > -29.9f) {
        if (outval[col][row] < loval) loval = outval[col][row];
        if (outval[col][row] > hival) hival = outval[col][row];
        ++npix;
      }
    }
  }
  //printf("%g percent tallied\n", 100.f*npix/(xres*yres));
  printf("min and max range: %g %g\n", loval, hival);

  // flip, to positive is better
  // and zero out the ocean
  for (int row=0; row<yres; ++row) {
    for (int col=0; col<xres; ++col) {
      if (tempw[col][row] < -29.9f) {
        // zero out the ocean
        outval[col][row] = 0.0f;
      } else {
        // flip to 0=bad, 1=best
        outval[col][row] = 1.0f - (outval[col][row]-loval)/(hival-loval);
        // apply power to accentuate the best
        outval[col][row] = powf(outval[col][row], 8.f);
      }
    }
  }

  // find the "best" place
  float bestval = 0.f;
  int bestrow = -1;
  int bestcol = -1;
  for (int row=0; row<yres; ++row) {
    for (int col=0; col<xres; ++col) {
      if (tempw[col][row] > -29.9f) {
        if (outval[col][row] > bestval) {
          bestval = outval[col][row];
          bestrow = row;
          bestcol = col;
        }
      }
    }
  }
  //printf("Best pixel is %d %d\n", bestcol, bestrow);
  printf("Best place on Earth is");
  const float nlat = 0.1f*(0.5f+bestrow-900.f);
  const float elong = 0.1f*(0.5f+bestcol-1800.f);
  if (nlat>0.f) printf(" %g N", nlat);
  else printf(" %g S", -nlat);
  if (elong>0.f) printf(" %g E", elong);
  else printf(" %g W", -elong);
  printf("\n");

  // optionally add national boundary lines (overwrite the mountain map)
  if (1==1) {
    (void)read_png("natl_bdry.png",xres,yres,FALSE,FALSE,1.0,FALSE,mtn,0.0,1.0,NULL,0.0,1.0,NULL,0.0,1.0);
    // and include only where it makes the pixel brighter
    for (int row=0; row<yres; ++row) {
      for (int col=0; col<xres; ++col) {
        if (mtn[col][row] > outval[col][row]) outval[col][row] = mtn[col][row];
      }
    }
  }

  // write the image
  (void)write_png(outpng,xres,yres,FALSE,TRUE, outval,0.f,1.f, NULL,0.0,1.0, NULL,0.0,1.0);

  exit(0);
}

