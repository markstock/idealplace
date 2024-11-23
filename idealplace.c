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
   "   [-nn num]   number of relaxation iterations in the wall-normal dir (1)  ",
   "                                                                           ",
   "   [-nt num]   number of relaxation iterations in the wall-tangent dir (1) ",
   "                                                                           ",
   "   [-i file]   input file name                                             ",
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




int main (int argc, char **argv) {

  int xres, yres;
  int nn, nt;
  char inpng[255];
  char outpng[255];
  char progname[255];
  int i;

  // define default parameters
  xres = -1000;
  yres = -1000;
  nn = 1;
  nt = 1;
  sprintf(outpng,"out.png");

  // process command-line parameters
  (void) strcpy(progname,argv[0]);
  if (argc < 1) (void) Usage(progname,0);
  for (i=1; i<argc; i++) {
    if (strncmp(argv[i], "-nn", 3) == 0) {
      nn = atoi(argv[++i]);
    } else if (strncmp(argv[i], "-nt", 3) == 0) {
      nt = atoi(argv[++i]);
    } else if (strncmp(argv[i], "-i", 2) == 0) {
      strcpy(inpng,argv[++i]);
    } else if (strncmp(argv[i], "-o", 2) == 0) {
      strcpy(outpng,argv[++i]);
    } else {
      (void) Usage(progname,0);
    }
  }

  // check the command-line parameters
  if (nn < 0) { nn = 0; }
  if (nt < 0) { nt = 0; }

  // interrogate the header for resolution
  sprintf(inpng,"airtemp_m1.png");
  (void)read_png_res(inpng, &yres, &xres);

  // allocate and read temperature, full range is -30 to 40 C
  float** tempw = allocate_2d_array_f(xres,yres);
  sprintf(inpng,"airtemp_m1.png");
  (void)read_png(inpng,xres,yres,FALSE,FALSE,1.0,FALSE,tempw,-30.0,70.0,NULL,0.0,1.0,NULL,0.0,1.0);
  float** temps = allocate_2d_array_f(xres,yres);
  sprintf(inpng,"airtemp_m7.png");
  (void)read_png(inpng,xres,yres,FALSE,FALSE,1.0,FALSE,temps,-30.0,70.0,NULL,0.0,1.0,NULL,0.0,1.0);

  // allocate and read precipitation, full range is 0 to 1000mm per month
  float** rain = allocate_2d_array_f(xres,yres);
  sprintf(inpng,"precip_avg.png");
  (void)read_png(inpng,xres,yres,FALSE,FALSE,1.0,FALSE,rain,0.0,1000.0,NULL,0.0,1.0,NULL,0.0,1.0);

  // and clouds (0=sunny, 1=cloudy)
  sprintf(inpng,"clouds.png");
  float** clouds = allocate_2d_array_f(xres,yres);
  (void)read_png(inpng,xres,yres,FALSE,FALSE,1.0,FALSE,clouds,0.0,1.0,NULL,0.0,1.0,NULL,0.0,1.0);

  // human development index
  sprintf(inpng,"hdi.png");
  float** hdi = allocate_2d_array_f(xres,yres);
  (void)read_png(inpng,xres,yres,FALSE,FALSE,1.0,FALSE,hdi,0.0,1.0,NULL,0.0,1.0,NULL,0.0,1.0);

  // proximity to mountains
  sprintf(inpng,"dem_variance_area.png");
  float** mtn = allocate_2d_array_f(xres,yres);
  (void)read_png(inpng,xres,yres,FALSE,FALSE,1.0,FALSE,mtn,0.0,1.0,NULL,0.0,1.0,NULL,0.0,1.0);


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
  const float hdi_penalty = 5.0f;
  const float mtn_penalty = 5.0f;

  // running sums
  float total_temp = 0.f;
  float total_rain = 0.f;
  float total_cloud = 0.f;
  float total_hdi = 0.f;
  float total_mtn = 0.f;

  // Lauren's ideal
  float ideal_tempw = 25.0;	// C
  float ideal_temps = 25.0;	// C
  float ideal_rain = 30.0;	// mm / month
  float ideal_cloud = 0.0;	// none, ever
  float ideal_hdi = 0.7;	// normalized
  float ideal_mtn = 0.1;	// normalized
  //printf("Lau's ideal temp %g and rain %g\n", ideal_temp, ideal_rain);

  // accumulate penalties
  for (int row=0; row<yres; ++row) {
    for (int col=0; col<xres; ++col) {
      if (tempw[col][row] > -29.9f) {

        float tempcost = temp_penalty * fabs(temps[col][row]-ideal_temps);
        outval[col][row] += tempcost;
        total_temp += tempcost;
        tempcost = temp_penalty * fabs(tempw[col][row]-ideal_tempw);
        outval[col][row] += tempcost;
        total_temp += tempcost;

        const float raincost = rain_penalty * fabs(logf((1.0+rain[col][row])/ideal_rain));
        outval[col][row] += raincost;
        total_rain += raincost;

        const float cloudcost = cloud_penalty * fabs(clouds[col][row]-ideal_cloud);
        outval[col][row] += cloudcost;
        total_cloud += cloudcost;

        const float hdicost = hdi_penalty * fabs(hdi[col][row]-ideal_hdi);
        outval[col][row] += hdicost;
        total_hdi += hdicost;
      }
    }
  }

  // Mark's ideal

  if (0==0) {
    // what is ideal temperature and rainfall for this month?
    ideal_temps = 7.f;	// deg C
    ideal_tempw = 25.f;	// deg C
    ideal_rain = 50.0;	// mm / month
    ideal_cloud = 0.2;	// sometimes
    ideal_hdi = 0.9;	// very civilized
    ideal_mtn = 0.5;	// some mountains
    //printf("Mark's ideal temp %g and rain %g for month %d\n", ideal_temp, ideal_rain, month);

    // accumulate penalties
    for (int row=0; row<yres; ++row) {
      for (int col=0; col<xres; ++col) {
        if (tempw[col][row] > -29.9f) {

          float tempcost = temp_penalty * fabs(temps[col][row]-ideal_temps);
          outval[col][row] += tempcost;
          total_temp += tempcost;
          tempcost = temp_penalty * fabs(tempw[col][row]-ideal_tempw);
          outval[col][row] += tempcost;
          total_temp += tempcost;

          const float raincost = rain_penalty * fabs(logf((1.0+rain[col][row])/ideal_rain));
          outval[col][row] += raincost;
          total_rain += raincost;

          const float cloudcost = cloud_penalty * fabs(clouds[col][row]-ideal_cloud);
          outval[col][row] += cloudcost;
          total_cloud += cloudcost;

          const float hdicost = hdi_penalty * fabs(hdi[col][row]-ideal_hdi);
          outval[col][row] += hdicost;
          total_hdi += hdicost;

          const float mtncost = mtn_penalty * fabs(mtn[col][row]-ideal_mtn);
          outval[col][row] += mtncost;
          total_mtn += mtncost;
        }
      }
    }
  }
  printf("total costs, temp %g, rain %g, cloud %g, hdi %g, mtn %g\n", total_temp, total_rain, total_cloud, total_hdi, total_mtn);

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
  printf("%g percent tallied\n", 100.f*npix/(xres*yres));
  printf("min and max range: %g %g\n", loval, hival);

  // flip, to positive is better
  // and zero out the ocean
  for (int row=0; row<yres; ++row) {
    for (int col=0; col<xres; ++col) {
      if (tempw[col][row] < -29.9f) {
        outval[col][row] = 0.0f;
      } else {
        outval[col][row] = 1.0f - (outval[col][row]-loval)/(hival-loval);
      }
    }
  }

  // write the image
  (void)write_png(outpng,xres,yres,FALSE,TRUE, outval,0.f,1.f, NULL,0.0,1.0, NULL,0.0,1.0);

  exit(0);
}

