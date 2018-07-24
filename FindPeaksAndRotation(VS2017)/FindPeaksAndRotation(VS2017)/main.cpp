#include "FindPeaks.h"

void main()
{
	Mat input, temp, output;
	/*
	//add peaks
	input = imread("./image/dodo.png");
	temp = imread("./image/peaks_data.png");
	output = input | temp;
	imwrite("./image/dodo_peaks_added.png", output);
	*/
	/*
	//rotation
	input = imread("./image/dodo_peaks_added.png");
	Mat matrix = getRotationMatrix2D(Point2f(input.cols / 2, input.rows / 2), 20, 1);
	warpAffine(input, output, matrix, Size(input.cols, input.rows));
	imwrite("./image/dodo_peaks_added_rotate_20.png", output);
	*/

	input = imread("./image/dodo_peaks_added_rotate_20.png");
	FindPeaks fp(&input);
	Mat matrix = getRotationMatrix2D(Point2f(input.cols / 2, input.rows / 2), fp.angle, fp.scale);
	warpAffine(input, output, matrix, Size(input.cols, input.rows));
	imwrite("./image/output.png", output);	
}