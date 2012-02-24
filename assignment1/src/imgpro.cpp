// COS 426, Spring 2006, Thomas Funkhouser
// Assignment 1: Image Processing
//
// main.c
// original by Wagner Correa, 1999
// modified by Robert Osada, 2000
// modified by Renato Werneck, 2003
// modified by Jason Lawrence, 2004
// modified by Jason Lawrence, 2005
// modified by Forrester Cole, 2006
// modified by Tom Funkhouser, 2007
// modified by Chris DeCoro, 2007
// modified by Martin Fuchs, 2010



// Include files

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "R2/R2.h"
#include "R2Pixel.h"
#include "R2Image.h"



// Program arguments

static char options[] =
	"  -help\n"
	"\n"
	"  -bilateral <real:domain> <real:range>\n"
	"  -blackandwhite \n"
	"  -blur <real:sigma>\n"
	"  -brightness <real:factor>\n"
	"  -composite <file:top_image> <file:bottom_mask> <file:top_mask> <int:operation(0=over)>\n"
	"  -contrast <real:factor>\n"
	"  -crop <int:x> <int:y> <int:width> <int:height>\n"
	"  -dither <int:method(0=random,1=ordered,2=FloydSteinberg)> <int:nbits>\n"
	"  -edge \n"
	"  -extract <int:channel (0=red,1=green,2=blue,3=alpha)>\n"
	"  -fun\n"
	"  -gamma <real:exponent>\n"
	"  -median <real:width>\n"
	"  -morph <file:target_image> <file:segment_correspondences> <real:t>\n"
	"  -motionblur <int:amount>  \n"
	"  -noise <real:factor>\n"
	"  -quantize <int:nbits>\n"
	"  -rotate <real:angle(in radians)> \n"
	"  -sampling <int:method (0=point,1=linear,2=Gaussian)>\n"
	"  -saturation <real:factor>\n"
	"  -scale <real:sx> <real:sy>\n"
	"  -sharpen \n"
	"  -sobel \n";


static void 
	ShowUsage(void)
{
	// Print usage message and exit
	fprintf(stderr, "Usage: imgpro input_image output_image [  -option [arg ...] ...]\n");
	fprintf(stderr, "%s", options);
	exit(EXIT_FAILURE);
}



static void 
	CheckOption(char *option, int argc, int minargc)
{
	// Check if there are enough remaining arguments for option
	if (argc < minargc)  {
		fprintf(stderr, "Too few arguments for %s\n", option);
		ShowUsage();
		exit(-1);
	}
}



static int 
	ReadCorrespondences(char *filename, R2Segment *&source_segments, R2Segment *&target_segments, int& nsegments)
{
	// Open file
	FILE *fp = fopen(filename, "r");
	if (!fp) {
		fprintf(stderr, "Unable to open correspondences file %s\n", filename);
		exit(-1);
	}

	// Read number of segments
	if (fscanf(fp, "%d", &nsegments) != 1) {
		fprintf(stderr, "Unable to read correspondences file %s\n", filename);
		exit(-1);
	}

	// Allocate arrays for segments
	source_segments = new R2Segment [ nsegments ];
	target_segments = new R2Segment [ nsegments ];
	if (!source_segments || !target_segments) {
		fprintf(stderr, "Unable to allocate correspondence segments for %s\n", filename);
		exit(-1);
	}

	// Read segments
	for (int i = 0; i <  nsegments; i++) {

		// Read source segment
		double sx1, sy1, sx2, sy2;
		if (fscanf(fp, "%lf%lf%lf%lf", &sx1, &sy1, &sx2, &sy2) != 4) { 
			fprintf(stderr, "Error reading correspondence %d out of %d\n", i, nsegments);
			exit(-1);
		}

		// Read target segment
		double tx1, ty1, tx2, ty2;
		if (fscanf(fp, "%lf%lf%lf%lf", &tx1, &ty1, &tx2, &ty2) != 4) { 
			fprintf(stderr, "Error reading correspondence %d out of %d\n", i, nsegments);
			exit(-1);
		}

		// Add segments to list
		source_segments[i] = R2Segment(sx1, sy1, sx2, sy2);
		target_segments[i] = R2Segment(tx1, ty1, tx2, ty2);
	}

	// Close file
	fclose(fp);

	// Return success
	return 1;
}



int 
	main(int argc, char **argv)
{
	// Look for help
	for (int i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-help")) {
			ShowUsage();
		}
	}

	// Read input and output image filenames
	if (argc < 3)  ShowUsage();
	argv++, argc--; // First argument is program name
	char *input_image_name = *argv; argv++, argc--; 
	char *output_image_name = *argv; argv++, argc--; 

	// Allocate image
	R2Image *image = new R2Image();
	if (!image) {
		fprintf(stderr, "Unable to allocate image\n");
		exit(-1);
	}

	// Read input image
	if (!image->Read(input_image_name)) {
		fprintf(stderr, "Unable to read image from %s\n", input_image_name);
		exit(-1);
	}

	// Initialize sampling method
	R2ImageSamplingMethod sampling_method = R2_IMAGE_POINT_SAMPLING;

	// Parse arguments and perform operations 
	while (argc > 0) {

		//Per-pixel operations
		if (!strcmp(*argv, "-brightness")) {
			CheckOption(*argv, argc, 2);
			double factor = atof(argv[1]);
			argv += 2, argc -=2;
			image->Brighten(factor);
		}
		else if (!strcmp(*argv, "-blackandwhite")) {
			argv++, argc--;
			image->BlackAndWhite();
		}
		else if (!strcmp(*argv, "-gamma")) {
			CheckOption(*argv, argc, 2);
			double factor = atof(argv[1]);
			argv += 2, argc -= 2;
			image->ApplyGamma(factor);
		}
		else if (!strcmp(*argv, "-saturation")) {
			CheckOption(*argv, argc, 2);
			double factor = atof(argv[1]);
			argv += 2, argc -= 2;
			image->ChangeSaturation(factor);
		}
		else if (!strcmp(*argv, "-contrast")) {
			CheckOption(*argv, argc, 2);
			double factor = atof(argv[1]);
			argv += 2, argc -= 2;
			image->ChangeContrast(factor);
		} 
		else if (!strcmp(*argv, "-extract")) {
			CheckOption(*argv, argc, 2);
			int channel = atoi(argv[1]);
			argv += 2, argc -= 2;
			image->ExtractChannel(channel);
		}

		// Linear Filtering Operations
		else if (!strcmp(*argv, "-blur")) {
			CheckOption(*argv, argc, 2);
			double sigma = atof(argv[1]);
			argv += 2, argc -= 2;
			image->Blur(sigma);
		}
		else if (!strcmp(*argv, "-sharpen")) {
			argv++, argc--;
			image->Sharpen();
		}
		else if (!strcmp(*argv, "-edge")) {
			argv++, argc--;
			image->EdgeDetect();
		}
		else if (!strcmp(*argv, "-motionblur")) {
			CheckOption(*argv, argc, 1);
			int amount = atoi(argv[1]);
			argv += 2, argc -= 2;
			image->MotionBlur(amount);
		}

		//Non-Linear Filtering Operations
		else if (!strcmp(*argv, "-median")) {
			CheckOption(*argv, argc, 2);
			double sigma = atof(argv[1]);
			argv += 2, argc -= 2;
			image->MedianFilter(sigma);
		}
		else if (!strcmp(*argv, "-bilateral")) {
			CheckOption(*argv, argc, 3);
			double sx = atof(argv[1]);
			double sy = atof(argv[2]);
			argv += 3, argc -= 3;
			image->BilateralFilter(sy, sx);
		}

		//Resampling Operations
		else if (!strcmp(*argv, "-sampling")) {
			CheckOption(*argv, argc, 2);
			sampling_method = (R2ImageSamplingMethod)atoi(argv[1]);
			argv += 2, argc -= 2;
		}
		else if (!strcmp(*argv, "-scale")) {
			CheckOption(*argv, argc, 3);
			double sx = atof(argv[1]);
			double sy = atof(argv[2]);
			argv += 3, argc -= 3;
			image->Scale(sx, sy, sampling_method);
		}
		else if (!strcmp(*argv, "-rotate")) {
			CheckOption(*argv, argc, 2);
			double angle = atof(argv[1]);
			argv += 2, argc -= 2;
			image->Rotate(angle, sampling_method);
		}
		else if (!strcmp(*argv, "-fun")) {
			image->Fun(sampling_method);
			argv++, argc--;
		}

		//Dithering Operations
		else if (!strcmp(*argv, "-quantize")) {
			CheckOption(*argv, argc, 2);
			int nbits = atoi(argv[1]);
			argv += 2, argc -= 2;
			image->Quantize(nbits);
		}
		else if (!strcmp(*argv, "-dither")) {
			CheckOption(*argv, argc, 3);
			int dither_method = atoi(argv[1]);
			int nbits = atoi(argv[2]);
			argv += 3, argc -= 3;
			if (dither_method == 0) image->RandomDither(nbits);
			else if (dither_method == 1) image->OrderedDither(nbits);
			else if (dither_method == 2) image->FloydSteinbergDither(nbits);
			else { fprintf(stderr, "Invalid dither method: %d\n", dither_method); exit(-1); }
		}

		//Miscellaneous
		else if (!strcmp(*argv, "-composite")) {
			CheckOption(*argv, argc, 5);
			R2Image *top_image = new R2Image(argv[1]);
			R2Image *bottom_mask = new R2Image(argv[2]);
			R2Image *top_mask = new R2Image(argv[3]);
			int operation = atoi(argv[4]);
			argv += 5, argc -= 5;
			image->CopyChannel(*bottom_mask, R2_IMAGE_BLUE_CHANNEL, R2_IMAGE_ALPHA_CHANNEL);
			top_image->CopyChannel(*top_mask, R2_IMAGE_BLUE_CHANNEL, R2_IMAGE_ALPHA_CHANNEL);
			image->Composite(*top_image, operation);
			delete top_image;
			delete bottom_mask;
			delete top_mask;
		}
		else if (!strcmp(*argv, "-morph")) {
			int nsegments = 0;
			R2Segment *source_segments = NULL;
			R2Segment *target_segments = NULL;
			CheckOption(*argv, argc, 4);
			R2Image *target_image = new R2Image(argv[1]);
			ReadCorrespondences(argv[2], source_segments, target_segments, nsegments);
			double t = atof(argv[3]);
			argv += 4, argc -= 4;
			image->Morph(*target_image, source_segments, target_segments, nsegments, t, sampling_method);
			delete target_image;
		}

		//Additional features 
		//To use, copy the else-if clause below and modify it to your needs
		else if (!strcmp(*argv, "-sobel")) {
			image->Sobel();
			argv++, argc--;
			//CheckOption(*argv, argc, N); //<-- checks that the number of arguments for the option is correct
			//Your commanf line arguments paring and method call goes here
			//argv += N; argc -= N; //<-- advance the command line arguments pointer and counter 
		}
		else {
			// Unrecognized program argument
			fprintf(stderr, "image: invalid option: %s\n", *argv);
			ShowUsage();
		}
	}

	// Write output image
	if (!image->Write(output_image_name)) {
		fprintf(stderr, "Unable to write image to %s\n", output_image_name);
		exit(-1);
	}

	// Delete image
	delete image;

	// Return success
	return EXIT_SUCCESS;
}



