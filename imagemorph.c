/*	Program to apply random elastic 'rubbersheet' 
        transforms to Netpbm color (.ppm, P6 raw binary) images for
        augmenting training sets in machine learning.

	The program reads an input pgm image from stdin and
	writes a ppm image to stdout.

	Original Author: Marius Bulacu (.pgm version for characters)
	Adapted for .ppm and color: Lambert Schomaker
	
Please cite:

M Bulacu, A Brink, T van der Zant, L Schomaker (2009).
Recognition of handwritten numerical fields in a 
large single-writer historical collection,
10th International Conference on Document Analysis and Recognition, 
pp. 808-812, DOI: 10.1109/ICDAR.2009.8 

Copyright M.Bulacu and L.Schomaker 2009,2016

*/

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <math.h>
# include <time.h>
# include <unistd.h>

typedef struct {
   unsigned char r;
   unsigned char g;
   unsigned char b;
} Pixel;


// Reads a gray-scale image in RAW PPM format from stdin
Pixel ** read_ppm_stdin( int * h, int * w )
{
	Pixel ** image;
	char buf[1024];
	char c;
	int i, j, l;


	// Read the PGM header and check it
	fscanf(stdin, "%s\n", buf);
	if ( strcmp(buf, "P6") != 0 )
	{
		fprintf(stderr, "\n! Error: The input is not in RAW PPM format (P6).\n");
		exit( 1 );
	}

	// Read the comment lines
	while( (c = fgetc(stdin)) == '#' )
	{
		fgets(buf, 1023, stdin);
	}
	ungetc(c, stdin);

	// Read image width and height
	fscanf(stdin, "%i", w);
	fscanf(stdin, "%i", h);
	fscanf(stdin, "%i", &l);
	c = fgetc(stdin); /* read newline */

	// Allocate the image
	image = (Pixel **) malloc( (*h) * sizeof(Pixel *) );
	for ( i = 0; i < (*h); i ++ )
	{
		image[i] = (Pixel *) malloc( (*w) * sizeof(Pixel) );
	}

	// Read the pixel values
	for ( i = 0; i < (*h); i ++ )
	{
		for ( j = 0; j < (*w); j ++ )
		{
			image[i][j].r = fgetc( stdin );
			image[i][j].g = fgetc( stdin );
			image[i][j].b = fgetc( stdin );
		}
	}

	return image;
}


// Writes a RGB image in RAW PPM format to stdout
void write_ppm_stdout( Pixel ** image, const int h, const int w )
{
	int i, j;

	// Write the PGM file header
	fprintf(stdout, "P6\n");
	fprintf(stdout, "# CREATOR: imagemorph.\n");
	fprintf(stdout, "%i %i\n", w, h);
	fprintf(stdout, "255\n");

	// Write the pixel data
	for ( i = 0; i < h; i ++ )
	{
		for ( j = 0; j < w; j ++ )
		{
			fputc((int) image[i][j].r, stdout);
			fputc((int) image[i][j].g, stdout);
			fputc((int) image[i][j].b, stdout);
		}
	}

	// Flush the data
	fflush( stdout );

	return;
}


// Computes the horizontal and vertical displacement fields
void compute_displacement_field( float ** d_x, float ** d_y, const int h, const int w,
				 const float amp, const float sigma )
{
	float ** da_x;
	float ** da_y;

	float * ker;
	int kws;
	int k;
	float sum_x, sum_y;
	float avg;

	int i, j;
	int u, v;


	// Allocate the auxiliary displacement fields
	da_x = (float **) malloc( h * sizeof(float *) );
	da_y = (float **) malloc( h * sizeof(float *) );
	for( i = 0; i < h; i ++ )
	{
		da_x[i] = (float *) malloc( w * sizeof(float) );
		da_y[i] = (float *) malloc( w * sizeof(float) );
	}

	// Allocate and prepare the gaussian smoothing kernel
	kws = (int)( 2.0 * sigma );
	ker = (float *) malloc( (kws + 1) * sizeof(float) );
	for( k = 0; k <= kws; k ++ )
	{
		ker[k] = exp( - (float)(k * k) / (sigma * sigma) );
	}

	// Generate the initial random displacement field
	for( i = 0; i < h; i ++ )
	{
		for( j = 0; j < w; j ++ )
		{
			d_x[i][j] = - 1.0 + 2.0 * drand48( );
			d_y[i][j] = - 1.0 + 2.0 * drand48( );
		}
	}

	// Smooth the random displacement field using the gaussian kernel
	for( i = 0; i < h; i ++ )
	{
		for( j = 0; j < w; j ++ )
		{
			sum_x = 0.0;
			sum_y = 0.0;
			for( k = - kws; k <= kws; k ++ )
			{
				v = j + k;

				if ( v < 0 )
				{
					v = - v;
				}
				if ( v >= w )
				{
					v = 2 * w - v - 1;
				}

				sum_x += d_x[i][v] * ker[ abs(k) ];
				sum_y += d_y[i][v] * ker[ abs(k) ];
			}

			da_x[i][j] = sum_x;
			da_y[i][j] = sum_y;
		}
	}
	for( j = 0; j < w; j ++ )
	{
		for( i = 0; i < h; i ++ )
		{
			sum_x = 0.0;
			sum_y = 0.0;
			for( k = - kws; k <= kws; k ++ )
			{
				u = i + k;

				if ( u < 0 )
				{
					u = - u;
				}
				if ( u >= h )
				{
					u = 2 * h - u - 1;
				}

				sum_x += da_x[u][j] * ker[ abs(k) ];
				sum_y += da_y[u][j] * ker[ abs(k) ];
			}

			d_x[i][j] = sum_x;
			d_y[i][j] = sum_y;
		}
	}

	// Normalize the field
	avg = 0.0;
	for( i = 0; i < h; i ++ )
	{
		for( j = 0; j < w; j ++ )
		{
			avg += sqrt( d_x[i][j] * d_x[i][j] + d_y[i][j] * d_y[i][j] );
		}
	}
	avg /= ( h * w );
	for( i = 0; i < h; i ++ )
	{
		for( j = 0; j < w; j ++ )
		{
			d_x[i][j] = amp * d_x[i][j] / avg;
			d_y[i][j] = amp * d_y[i][j] / avg;
		}
	}

	// Garbage collection
	for( i = 0; i < h; i ++ )
	{
		free( da_x[i] );
		free( da_y[i] );
	}
	free( da_x );
	free( da_y );
	free( ker );
}


// Applies the displacement field to an image: bilinear interpolation is used
void apply_displacement_field(Pixel ** input, Pixel ** output, const int h, const int w,
			       float ** d_x, float ** d_y )
{
	int i, j;
	int idx;
	int u = 0, v = 0;
	int u0, v0;

	float p1, p2;
	float f1, f2;
	float f = 0.0;
	int val;
	float sumr, sumg, sumb;


	// Bilinear interpolation
	for( i = 0; i < h; i ++ )
	{
		for( j = 0; j < w; j ++ )
		{
			p1 = i + d_y[i][j];
			p2 = j + d_x[i][j];

			u0 = (int)floorf( p1 );
			v0 = (int)floorf( p2 );

			f1 = p1 - u0;
			f2 = p2 - v0;

			sumr = 0.0;
			sumg = 0.0;
			sumb = 0.0;
			for( idx = 0; idx < 4; idx ++ )
			{
				switch( idx )
				{
					case 0:	u = u0;
						v = v0;
						f = (1.0 - f1) * (1.0 - f2);
						break;
					case 1:	u = u0 + 1;
						v = v0;
						f = f1 * (1.0 - f2);
						break;
					case 2:	u = u0;
						v = v0 + 1;
						f = (1.0 - f1) * f2;
						break;
					case 3:	u = u0 + 1;
						v = v0 + 1;
						f = f1 * f2;
						break;
				}

				if ( u < 0 )  u = 0;
				if ( u >= h ) u = h - 1;
				if ( v < 0 )  v = 0;
				if ( v >= w ) v = w - 1;

				val = input[u][v].r;
				sumr += f * val;

				val = input[u][v].g;
				sumg += f * val;

				val = input[u][v].b;
				sumb += f * val;
			}

			output[i][j].r = (unsigned char) sumr;
			output[i][j].g = (unsigned char) sumg;
			output[i][j].b = (unsigned char) sumb;
		}
	}
}


// Rubbersheet transform on a gray-scale image
// amp   = the amplitude of the deformation
// sigma = the local image area affected (the spread of the gaussian smoothing kernel)
void rubbersheet( Pixel ** input, Pixel ** output, const int h, const int w,
		  const float amp, const float sigma )
{
	float ** d_x;
	float ** d_y;

	int i;


	// Check that 'sigma' is not too large
	if( (sigma > h / 2.5) || (sigma > w / 2.5) )
	{
		fprintf( stderr, "- Warning: Gaussian smoothing kernel too large for the input image.\n" );
		return;
	}

	// Check that 'sigma' is not negative
	if( sigma < 1E-5 )
	{
		fprintf( stderr, "- Warning: Gaussian smoothing kernel with negative/zero spread.\n" );
		return;
	}

	// Allocate the displacement fields
	d_x  = (float **) malloc( h * sizeof(float *) );
	d_y  = (float **) malloc( h * sizeof(float *) );
	for( i = 0; i < h; i ++ )
	{
		d_x[i]  = (float *) malloc( w * sizeof(float) );
		d_y[i]  = (float *) malloc( w * sizeof(float) );
	}

	// Prepare the displacement fields
	compute_displacement_field( d_x, d_y, h, w, amp, sigma );

	// Apply the displacement fields
	apply_displacement_field( input, output, h, w, d_x, d_y );

	// Garbage collection
	for( i = 0; i < h; i ++ )
	{
		free( d_x[i] );
		free( d_y[i] );
	}
	free( d_x );
	free( d_y );
}


int main( int argc, char ** argv )
{
	int h, w;
	Pixel ** input;
	Pixel ** output;

	float amp;
	float sigma;
	int i;


// Seed the random number generator
	srand48( (unsigned int)getpid( ) + (unsigned int) time( NULL ) );

// Process the command line arguments
	if ( argc != 3 )
	{
		fprintf( stderr, "Program usage: cat [pgm image] | %s [displacement] [smoothing radius].\n", argv[0] );
		fprintf( stderr, "The morphed image is written to stdout.\n" );
		exit(1);
	}
	amp   = atof( argv[1] );
	sigma = atof( argv[2] );

// Read in the input image
	input = read_ppm_stdin( &h, &w );

// Allocate the output image
	output = (Pixel **) malloc( h * sizeof(Pixel *) );
	for ( i = 0; i < h; i ++ )
	{
		output[i] = (Pixel *) malloc( w * sizeof(Pixel) );
	}

// Morph the image
	rubbersheet( input, output, h, w, amp, sigma );

// Write the morphed image to stdout
	write_ppm_stdout( output, h, w );

// Garbage collection
	for ( i = 0; i < h; i ++ )
	{
		free( input[i] );
		free( output[i] );
	}
	free( input );
	free( output );

return 0;
}
